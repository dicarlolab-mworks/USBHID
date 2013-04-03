//
//  USBHIDPlugin.cpp
//  USBHID
//
//  Created by Christopher Stawarz on 4/3/13.
//  Copyright (c) 2013 The MWorks Project. All rights reserved.
//


BEGIN_NAMESPACE_MW


class USBHIDPlugin : public Plugin {
    void registerComponents(boost::shared_ptr<ComponentRegistry> registry) MW_OVERRIDE {
    }
};


extern "C" Plugin* getPlugin() {
    return new USBHIDPlugin();
}


END_NAMESPACE_MW
