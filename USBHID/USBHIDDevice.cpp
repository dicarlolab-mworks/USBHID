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


bool USBHIDDevice::initialize() {
    CFDictionaryPtr matching = createDeviceMatchingDictionary();
    if (!matching) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Unable to create device matching dictionary");
        return false;
    }
    
    IOHIDManagerSetDeviceMatching(hidManager.get(), matching.get());
    
    const IOReturn status = IOHIDManagerOpen(hidManager.get(), kIOHIDOptionsTypeNone);
    if (status != kIOReturnSuccess) {
        merror(M_IODEVICE_MESSAGE_DOMAIN, "Unable to open HID manager (status = %d)", status);
        return false;
    }
    
    return true;
}


CFDictionaryPtr USBHIDDevice::createDeviceMatchingDictionary() const {
    CFNumberPtr usagePageCFNumber = manageCFRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &usagePage));
    CFNumberPtr usageCFNumber = manageCFRef(CFNumberCreate(kCFAllocatorDefault, kCFNumberLongType, &usage));
    
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


END_NAMESPACE_MW


























