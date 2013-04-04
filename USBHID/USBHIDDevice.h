//
//  USBHIDDevice.h
//  USBHID
//
//  Created by Christopher Stawarz on 4/3/13.
//  Copyright (c) 2013 The MWorks Project. All rights reserved.
//

#ifndef __USBHID__USBHIDDevice__
#define __USBHID__USBHIDDevice__

#include "CFTypeHelpers.h"


BEGIN_NAMESPACE_MW


class USBHIDDevice : public IODevice, boost::noncopyable {
    
public:
    static const std::string USAGE_PAGE;
    static const std::string USAGE;
    static const std::string LOG_ALL_INPUT_VALUES;
    
    static void describeComponent(ComponentInfo &info);
    
    explicit USBHIDDevice(const ParameterValueMap &parameters);
    ~USBHIDDevice();
    
    bool initialize() MW_OVERRIDE;
    
private:
    CFDictionaryPtr createDeviceMatchingDictionary() const;
    
    const long usagePage;
    const long usage;
    const bool logAllInputValues;
    
    const IOHIDManagerPtr hidManager;
    IOHIDDeviceRef hidDevice;
    
};


END_NAMESPACE_MW


#endif // !defined(__USBHID__USBHIDDevice__)


























