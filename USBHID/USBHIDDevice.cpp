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
    hidManager(IOHIDManagerPtr(IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone), CFObjectRef::owned))
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
    boost::shared_ptr<USBHIDInputChannel> newInputChannel = boost::dynamic_pointer_cast<USBHIDInputChannel>(child);
    if (newInputChannel) {
        boost::shared_ptr<USBHIDInputChannel> &channel = inputChannels[std::make_pair(newInputChannel->getUsagePage(),
                                                                                      newInputChannel->getUsage())];
        if (channel) {
            throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN,
                                  "Cannot create more than one USBHID channel for a given usage page and usage");
        }
        channel = newInputChannel;
        return;
    }
    
    throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN, "Invalid channel type for USBHID device");
}


bool USBHIDDevice::initialize() {
    if (inputChannels.empty() && !logAllInputValues) {
        throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN,
                              "USBHID device must have at least one channel or have value logging enabled");
    }
    
    const CFDictionaryPtr deviceMatchingDictionary = createMatchingDictionary(usagePage, usage);
    IOHIDManagerSetDeviceMatching(hidManager.get(), deviceMatchingDictionary.get());
    
    const IOReturn status = IOHIDManagerOpen(hidManager.get(), kIOHIDOptionsTypeNone);
    if (status != kIOReturnSuccess) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Unable to open HID manager (status = %d)", status);
        return false;
    }
    
    const CFSetPtr matchingDevices = CFSetPtr(IOHIDManagerCopyDevices(hidManager.get()), CFObjectRef::owned);
    const CFIndex numMatchingDevices = (matchingDevices ? CFSetGetCount(matchingDevices.get()) : 0);
    if (!matchingDevices || (numMatchingDevices < 1)) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "No matching HID devices found");
        return false;
    }
    
    const void *values[numMatchingDevices];
    CFSetGetValues(matchingDevices.get(), values);
    hidDevice = IOHIDDevicePtr((IOHIDDeviceRef)(values[0]), CFObjectRef::borrowed);
    
    if (!prepareInputChannels()) {
        return false;
    }
    
    IOHIDDeviceRegisterInputValueCallback(hidDevice.get(), &inputValueCallback, this);
    
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


CFDictionaryPtr USBHIDDevice::createMatchingDictionary(long usagePage, long usage) {
    const CFNumberPtr usagePageCFNumber = CFNumberPtr(CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &usagePage),
                                                      CFObjectRef::owned);
    const CFNumberPtr usageCFNumber = CFNumberPtr(CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &usage),
                                                  CFObjectRef::owned);
    
    if (!usagePageCFNumber || !usageCFNumber) {
        throw std::bad_alloc();
    }
    
    const CFIndex numValues = 2;
    const void *keys[numValues] = {CFSTR(kIOHIDDeviceUsagePageKey), CFSTR(kIOHIDDeviceUsageKey)};
    const void *values[numValues] = {usagePageCFNumber.get(), usageCFNumber.get()};
    
    CFDictionaryPtr matchingDictionary = CFDictionaryPtr(CFDictionaryCreate(kCFAllocatorDefault,
                                                                            keys,
                                                                            values,
                                                                            numValues,
                                                                            &kCFTypeDictionaryKeyCallBacks,
                                                                            &kCFTypeDictionaryValueCallBacks),
                                                         CFObjectRef::owned);
    
    if (!matchingDictionary) {
        throw std::bad_alloc();
    }
    
    return matchingDictionary;
}


void USBHIDDevice::inputValueCallback(void *context, IOReturn result, void *sender, IOHIDValueRef value) {
    static_cast<USBHIDDevice *>(context)->handleInputValue(value);
}


bool USBHIDDevice::prepareInputChannels() {
    std::vector<CFDictionaryPtr> matchingDicts;
    std::vector<const void *> matchingArrayItems;
    
    BOOST_FOREACH(const InputChannelMap::value_type &value, inputChannels) {
        const UsagePair &usagePair = value.first;
        
        const CFDictionaryPtr dict = createMatchingDictionary(usagePair.first, usagePair.second);
        matchingDicts.push_back(dict);
        matchingArrayItems.push_back(dict.get());
        
        CFArrayPtr matchingElements = CFArrayPtr(IOHIDDeviceCopyMatchingElements(hidDevice.get(),
                                                                                 dict.get(),
                                                                                 kIOHIDOptionsTypeNone),
                                                 CFObjectRef::owned);
        if (!matchingElements || (CFArrayGetCount(matchingElements.get()) < 1)) {
            merror(M_IODEVICE_MESSAGE_DOMAIN,
                   "No matching HID elements for usage page %ld, usage %ld",
                   usagePair.first,
                   usagePair.second);
            return false;
        }
        
        hidElements[usagePair] = IOHIDElementPtr((IOHIDElementRef)CFArrayGetValueAtIndex(matchingElements.get(), 0),
                                                 CFObjectRef::borrowed);
    }
    
    if (!logAllInputValues) {
        CFArrayPtr inputValueMatchingArray = CFArrayPtr(CFArrayCreate(kCFAllocatorDefault,
                                                                      &(matchingArrayItems.front()),
                                                                      matchingArrayItems.size(),
                                                                      &kCFTypeArrayCallBacks),
                                                        CFObjectRef::owned);
        if (!inputValueMatchingArray) {
            throw std::bad_alloc();
        }
        
        IOHIDDeviceSetInputValueMatchingMultiple(hidDevice.get(), inputValueMatchingArray.get());
    }
    
    return true;
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


























