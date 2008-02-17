// pxCamera Copyright 2007-2008 John Robinson
// pxCamera.h

#include "pxCore.h"
#include "pxBuffer.h"
#include "pxOffscreen.h"

#if defined(PX_PLATFORM_WIN)
#include "win/pxCameraNative.h"
#endif

class pxCameras;
class pxCamera;
class pxICameraCapture;

// Use this class to enumerate all available video cameras
class pxCameras: public pxCamerasNative
{
public:
    pxCameras();
    ~pxCameras();

    pxError init();
    pxError term();

    // This method will reset the enumeration so that the next
    // call to the next method will return the first attached
    // video camera.
    pxError reset();
    
    // This will init-ialize the provided pxCamera object with the next available camera
    bool next(pxCamera& camera);
};

class pxCamera: public pxCameraNative
{
public:
    pxCamera();
    ~pxCamera();

    // id ia intended to be an opaque non-readable string
    // that uniquely identiifes the camera.  It can be discovered
    // via the pxCameras class
    pxError init(char* id);
    pxError term();

    operator bool ();

    // Returns an opaque unique identifier for a camera
    char* id();
    // Returns a human readable name for the camera
    char* name();

    // This method will start capturing and callback the
    // provided callback object for each frame
    // NOTE: THE callback method onCameraCapture will be invoked on another thread
    pxError startCapture(pxICameraCapture* callback);
    pxError stopCapture();
};

// Callback Interface
class pxICameraCapture
{
public:
    // Gets called for every frame captured by the camera
    // NOTE: This will get called on a different thread
    // NOTE: The frame data will not survive past the duration of this call.
    virtual void onCameraCapture(pxBuffer& frame) = (0);
};

