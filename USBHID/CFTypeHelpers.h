//
//  CFTypeHelpers.h
//  USBHID
//
//  Created by Christopher Stawarz on 4/3/13.
//  Copyright (c) 2013 The MWorks Project. All rights reserved.
//

#ifndef USBHID_CFTypeHelpers_h
#define USBHID_CFTypeHelpers_h


BEGIN_NAMESPACE_MW


BEGIN_NAMESPACE(cf)


template <typename T>
class ObjectPtr {
    
    BOOST_COPYABLE_AND_MOVABLE(ObjectPtr)
    
public:
    static ObjectPtr created(T ref) {
        if (!ref) throw std::bad_alloc();
        return owned(ref);
    }
    
    static ObjectPtr owned(T ref) {
        return ObjectPtr(ref, false);  // Already owned, so don't retain
    }
    
    static ObjectPtr borrowed(T ref) {
        return ObjectPtr(ref, true);   // Borrowed, so retain to establish ownership
    }
    
    ~ObjectPtr() {
        if (ref) CFRelease(ref);
    }
    
    ObjectPtr() : ref(NULL) { }
    
    // Copy constructor
    ObjectPtr(const ObjectPtr& other) :
        ref(other.ref)
    {
        if (ref) CFRetain(ref);
    }
    
    // Copy assignment
    ObjectPtr& operator=(BOOST_COPY_ASSIGN_REF(ObjectPtr) other) {
        if (this != &other) {
            if (ref) CFRelease(ref);
            ref = other.ref;
            if (ref) CFRetain(ref);
        }
        return (*this);
    }
    
    // Move constructor
    ObjectPtr(BOOST_RV_REF(ObjectPtr) other) :
        ref(other.ref)
    {
        other.ref = NULL;
    }
    
    // Move assignment
    ObjectPtr& operator=(BOOST_RV_REF(ObjectPtr) other) {
        if (this != &other) {
            if (ref) CFRelease(ref);
            ref = other.ref;
            other.ref = NULL;
        }
        return (*this);
    }
    
    T get() const MW_NOEXCEPT {
        return ref;
    }
    
    explicit operator bool() const MW_NOEXCEPT {
        return ref;
    }
    
private:
    ObjectPtr(T ref, bool retain) :
        ref(ref)
    {
        if (ref && retain) CFRetain(ref);
    }
    
    T ref;
    
};


typedef ObjectPtr<CFArrayRef> ArrayPtr;
typedef ObjectPtr<CFDictionaryRef> DictionaryPtr;
typedef ObjectPtr<CFNumberRef> NumberPtr;
typedef ObjectPtr<CFSetRef> SetPtr;
typedef ObjectPtr<CFStringRef> StringPtr;


END_NAMESPACE(cf)


BEGIN_NAMESPACE(iohid)


typedef cf::ObjectPtr<IOHIDDeviceRef> DevicePtr;
typedef cf::ObjectPtr<IOHIDElementRef> ElementPtr;
typedef cf::ObjectPtr<IOHIDManagerRef> ManagerPtr;


END_NAMESPACE(iohid)


END_NAMESPACE_MW


#endif // !defined(USBHID_CFTypeHelpers_h)


























