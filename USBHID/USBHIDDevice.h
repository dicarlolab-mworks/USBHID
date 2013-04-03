//
//  USBHIDDevice.h
//  USBHID
//
//  Created by Christopher Stawarz on 4/3/13.
//  Copyright (c) 2013 The MWorks Project. All rights reserved.
//

#ifndef __USBHID__USBHIDDevice__
#define __USBHID__USBHIDDevice__

#include <boost/intrusive_ptr.hpp>
#include <boost/noncopyable.hpp>
#include <boost/type_traits/remove_pointer.hpp>


inline void intrusive_ptr_add_ref(IOHIDManagerRef hidManager) {
    CFRetain(hidManager);
}


inline void intrusive_ptr_release(IOHIDManagerRef hidManager) {
    CFRelease(hidManager);
}


BEGIN_NAMESPACE_MW


class USBHIDDevice : public IODevice, boost::noncopyable {
    
public:
    static void describeComponent(ComponentInfo &info);
    
    explicit USBHIDDevice(const ParameterValueMap &parameters);
    ~USBHIDDevice();
    
private:
    boost::intrusive_ptr<boost::remove_pointer<IOHIDManagerRef>::type> hidManager;
    
};


END_NAMESPACE_MW


#endif // !defined(__USBHID__USBHIDDevice__)


























