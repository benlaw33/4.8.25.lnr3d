// MetalBridge.mm
// Objective-C++ bridge for Metal integration with SDL

#import <AppKit/AppKit.h>
#import <QuartzCore/CAMetalLayer.h>

extern "C" {
    // Function to set Metal layer for SDL window
    void SetMetalLayerForSDLWindow(void* nsWindowPtr, void* metalLayerPtr) {
        // Cast the void pointers to their Objective-C types
        NSWindow* nsWindow = (__bridge NSWindow*)nsWindowPtr;
        CAMetalLayer* metalLayer = (__bridge CAMetalLayer*)metalLayerPtr;
        
        // Set the metal layer on the content view
        NSView* contentView = [nsWindow contentView];
        [contentView setWantsLayer:YES];
        [contentView setLayer:metalLayer];
    }
    
    // Function to create a CAMetalLayer and return it as a void pointer
    void* CreateCAMetalLayer() {
        CAMetalLayer* layer = [CAMetalLayer layer];
        return (__bridge_retained void*)layer;
    }
    
    // Function to properly release the layer
    void ReleaseCAMetalLayer(void* layerPtr) {
        if (layerPtr) {
            CAMetalLayer* layer = (__bridge_transfer CAMetalLayer*)layerPtr;
            // The layer will be released when this function exits
            // due to the __bridge_transfer cast
        }
    }
}