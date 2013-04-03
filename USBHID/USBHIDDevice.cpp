//
//  USBHIDDevice.cpp
//  USBHID
//
//  Created by Christopher Stawarz on 4/3/13.
//  Copyright (c) 2013 The MWorks Project. All rights reserved.
//

#include "USBHIDDevice.h"


BEGIN_NAMESPACE_MW


void USBHIDDevice::describeComponent(ComponentInfo &info) {
    IODevice::describeComponent(info);
}


USBHIDDevice::USBHIDDevice(const ParameterValueMap &parameters) :
    IODevice(parameters),
    hidManager(IOHIDManagerCreate(kCFAllocatorDefault, kIOHIDManagerOptionNone), false)
{
    if (!hidManager) {
        throw SimpleException(M_IODEVICE_MESSAGE_DOMAIN, "Unable to create HID manager");
    }
}


USBHIDDevice::~USBHIDDevice() {
}


END_NAMESPACE_MW
