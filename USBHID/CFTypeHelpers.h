//
//  CFTypeHelpers.h
//  USBHID
//
//  Created by Christopher Stawarz on 4/3/13.
//  Copyright (c) 2013 The MWorks Project. All rights reserved.
//

#ifndef USBHID_CFTypeHelpers_h
#define USBHID_CFTypeHelpers_h


inline void intrusive_ptr_add_ref(CFDictionaryRef ref) { CFRetain (ref); }
inline void intrusive_ptr_release(CFDictionaryRef ref) { CFRelease(ref); }

inline void intrusive_ptr_add_ref(CFNumberRef ref) { CFRetain (ref); }
inline void intrusive_ptr_release(CFNumberRef ref) { CFRelease(ref); }

inline void intrusive_ptr_add_ref(CFSetRef ref) { CFRetain (ref); }
inline void intrusive_ptr_release(CFSetRef ref) { CFRelease(ref); }

inline void intrusive_ptr_add_ref(IOHIDManagerRef ref) { CFRetain (ref); }
inline void intrusive_ptr_release(IOHIDManagerRef ref) { CFRelease(ref); }


BEGIN_NAMESPACE_MW


template <typename T>
struct CFObjectPtr {
    typedef boost::intrusive_ptr<typename boost::remove_pointer<T>::type> type;
};


typedef CFObjectPtr<CFDictionaryRef>::type CFDictionaryPtr;
typedef CFObjectPtr<CFNumberRef>::type CFNumberPtr;
typedef CFObjectPtr<CFSetRef>::type CFSetPtr;
typedef CFObjectPtr<IOHIDManagerRef>::type IOHIDManagerPtr;


template <typename T>
inline typename CFObjectPtr<T>::type manageCFRef(T ref) {
    return typename CFObjectPtr<T>::type(ref, false);
}


END_NAMESPACE_MW


#endif // !defined(USBHID_CFTypeHelpers_h)


























