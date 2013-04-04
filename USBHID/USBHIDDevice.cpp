//
//  USBHIDDevice.cpp
//  USBHID
//
//  Created by Christopher Stawarz on 4/3/13.
//  Copyright (c) 2013 The MWorks Project. All rights reserved.
//

#include "USBHIDDevice.h"


BEGIN_NAMESPACE_MW


const std::string USBHIDDevice::USAGE_PAGE("usage_page");
const std::string USBHIDDevice::USAGE("usage");
const std::string USBHIDDevice::LOG_ALL_INPUT_VALUES("log_all_input_values");


void USBHIDDevice::describeComponent(ComponentInfo &info) {
    IODevice::describeComponent(info);
    
    info.setSignature("iodevice/usbhid_generic");
    
    info.addParameter(USAGE_PAGE);
    info.addParameter(USAGE);
    info.addParameter(LOG_ALL_INPUT_VALUES, "NO");
}


USBHIDDevice::USBHIDDevice(const ParameterValueMap &parameters) :
    IODevice(parameters),
    usagePage(parameters[USAGE_PAGE]),
    usage(parameters[USAGE]),
    logAllInputValues(parameters[LOG_ALL_INPUT_VALUES]),
    hidManager(manageCFRef(IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone)))
{
    if (usagePage <= kHIDPage_Undefined) {
        throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN, "Invalid HID usage page");
    }
    if (usage <= kHIDUsage_Undefined) {
        throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN, "Invalid HID usage");
    }
    if (!hidManager) {
        throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN, "Unable to create HID manager");
    }
}


USBHIDDevice::~USBHIDDevice() {
    (void)IOHIDManagerClose(hidManager.get(), kIOHIDOptionsTypeNone);
}


void USBHIDDevice::addChild(std::map<std::string, std::string> parameters,
                            ComponentRegistryPtr reg,
                            boost::shared_ptr<Component> child)
{
    boost::shared_ptr<USBHIDInputChannel> channel = boost::dynamic_pointer_cast<USBHIDInputChannel>(child);
    if (channel) {
        inputChannels[std::make_pair(channel->getUsagePage(), channel->getUsage())] = channel;
        return;
    }
    
    throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN, "Invalid channel type for USBHID device");
}


bool USBHIDDevice::initialize() {
    const CFDictionaryPtr matchingDictionary = createDeviceMatchingDictionary();
    if (!matchingDictionary) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Unable to create HID device matching dictionary");
        return false;
    }
    
    IOHIDManagerSetDeviceMatching(hidManager.get(), matchingDictionary.get());
    
    const IOReturn status = IOHIDManagerOpen(hidManager.get(), kIOHIDOptionsTypeNone);
    if (status != kIOReturnSuccess) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Unable to open HID manager (status = %d)", status);
        return false;
    }
    
    const CFSetPtr matchingDevices = manageCFRef(IOHIDManagerCopyDevices(hidManager.get()));
    if (!matchingDevices) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "No matching HID devices found");
        return false;
    }
    
    const CFIndex numMatchingDevices = CFSetGetCount(matchingDevices.get());
    if (numMatchingDevices < 1) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "No matching HID devices found");
        return false;
    }
    
    const void *values[numMatchingDevices];
    CFSetGetValues(matchingDevices.get(), values);
    hidDevice = (IOHIDDeviceRef)(values[0]);
    
    IOHIDDeviceRegisterInputValueCallback(hidDevice, &inputValueCallback, this);
    
    return true;
}


bool USBHIDDevice::startDeviceIO() {
    if (!isRunning()) {
        try {
            runLoopThread = boost::thread(boost::bind(&USBHIDDevice::runLoop,
                                                      component_shared_from_this<USBHIDDevice>()));
        } catch (const boost::thread_resource_error &e) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Unable to start HID device: %s", e.what());
            return false;
        }
    }
    
    return true;
}


bool USBHIDDevice::stopDeviceIO() {
    if (isRunning()) {
        runLoopThread.interrupt();
        try {
            runLoopThread.join();
        } catch (const boost::system::system_error &e) {
            merror(M_IODEVICE_MESSAGE_DOMAIN, "Unable to stop HID device: %s", e.what());
            return false;
        }
    }
    
    return true;
}


void USBHIDDevice::inputValueCallback(void *context, IOReturn result, void *sender, IOHIDValueRef value) {
    static_cast<USBHIDDevice *>(context)->handleInputValue(value);
}


CFDictionaryPtr USBHIDDevice::createDeviceMatchingDictionary() const {
    const CFNumberPtr usagePageCFNumber = manageCFRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &usagePage));
    const CFNumberPtr usageCFNumber = manageCFRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &usage));
    
    if (!usagePageCFNumber || !usageCFNumber) {
        return CFDictionaryPtr();
    }
    
    const CFIndex numValues = 2;
    const void *keys[numValues] = {CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey)};
    const void *values[numValues] = {usagePageCFNumber.get(), usageCFNumber.get()};
    
    return manageCFRef(CFDictionaryCreate(kCFAllocatorDefault,
                                          keys,
                                          values,
                                          numValues,
                                          &kCFTypeDictionaryKeyCallBacks,
                                          &kCFTypeDictionaryValueCallBacks));
}


void USBHIDDevice::runLoop() {
    IOHIDManagerScheduleWithRunLoop(hidManager.get(), CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    
    BOOST_SCOPE_EXIT(&hidManager) {
        IOHIDManagerUnscheduleFromRunLoop(hidManager.get(), CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);
    } BOOST_SCOPE_EXIT_END
    
    while (true) {
        // Run the CFRunLoop for 500ms
        (void)CFRunLoopRunInMode(kCFRunLoopDefaultMode, 0.5, false);
        
        // Give another thread a chance to terminate this one
        boost::this_thread::interruption_point();
    }
}


void USBHIDDevice::handleInputValue(IOHIDValueRef value) {
    IOHIDElementRef element = IOHIDValueGetElement(value);
    const uint32_t elementUsagePage = IOHIDElementGetUsagePage(element);
    const uint32_t elementUsage = IOHIDElementGetUsage(element);
    const CFIndex integerValue = IOHIDValueGetIntegerValue(value);
    
    const boost::shared_ptr<USBHIDInputChannel> &channel = inputChannels[std::make_pair(long(elementUsagePage),
                                                                                        long(elementUsage))];
    if (channel) {
        // Convert OS absolute time stamp to nanoseconds, subtract MWorks base time, and convert to microseconds
        MWTime valueTime = ((MWTime(AudioConvertHostTimeToNanos(IOHIDValueGetTimeStamp(value)))
                                    - Clock::instance()->getSystemBaseTimeNS())
                            / MWTime(1000));
        
        channel->postValue(integerValue, valueTime);
    }
    
    if (logAllInputValues) {
        mprintf("HID input on device \"%s\":\n"
                "\tUsage page:\t%u\t(0x%02X)\n"
                "\tUsage:\t\t%u\t(0x%02X)\n"
                "\tValue:\t\t%ld",
                getTag().c_str(),
                elementUsagePage, elementUsagePage,
                elementUsage, elementUsage,
                integerValue);
    }
}


END_NAMESPACE_MW


























