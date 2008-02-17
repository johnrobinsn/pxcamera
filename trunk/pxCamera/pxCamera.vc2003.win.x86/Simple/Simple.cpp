// Simple Example CopyRight 2007-2008 John Robinson
// Demonstrates capturing frames from the pxCamera class

// Keys
// <TAB> - Switch cameras

#include "pxCore.h"
#include "pxEventLoop.h"
#include "pxWindow.h"
#include "pxOffscreen.h"

#include "pxCamera.h"

#include <stdio.h>

pxEventLoop eventLoop;

void drawBackground(pxBuffer& b)
{
    // Fill the buffer with a simple pattern as a function of f(x,y)
    int w = b.width();
    int h = b.height();

    for (int y = 0; y < h; y++)
    {
        pxPixel* p = b.scanline(y);
        for (int x = 0; x < w; x++)
        {
            p->r = pxClamp<int>(x+y, 255);
            p->g = pxClamp<int>(y,   255);
            p->b = pxClamp<int>(x,   255);
            p++;
        }
    }
}

class myWindow: public pxWindow, public pxICameraCapture
{
public:

    myWindow()
    {
        mVideoWidth = mVideoHeight = 0;
    }

private:

    // Using the enumeration APIs from pxCameras try to find a camera
    // If one is found start capturing frames from it
    void getACamera()
    {
        
        if (mCamera)
            mCamera.stopCapture();

        bool cameraFound = false;

        if (mCameras.next(mCamera))
            cameraFound = true;
        else
        {
            mCameras.reset();
            cameraFound = mCameras.next(mCamera);
        }

        char buffer[512];
        if (cameraFound)
            sprintf(buffer, "Simple - %s", mCamera.name());
        else
            strcpy(buffer, "Simple - Please attach a camera and then press <TAB>");

        setTitle(buffer);

        mCamera.startCapture(this);

        // Let's repaint the background whilst we wait
        // for the camera to start
        invalidateRect();
    }

    // Event Handlers - Look in pxWindow.h for more

    void onCreate()
    {
        mCameras.init();
        getACamera();
    }

    void onCloseRequest()
    {
        // When someone clicks the close box no policy is predefined.
        // so we need to explicitly tell the event loop to exit
        eventLoop.exit();
    }

    void onSize(int newWidth, int newHeight)
    {
        mTexture.init(newWidth, newHeight);
        drawBackground(mTexture);

        invalidateRect();

#if PX_PLATFORM_WIN        
        // Mark the area where the video is being displayed as
        // valid to eliminate flicker on resize
        RECT r;
        SetRect(&r, 0, 0, mVideoWidth, mVideoHeight);
        ::ValidateRect(mWindow, &r);
#endif
    }

    void onDraw(pxSurfaceNative s)
    {
        mTexture.blit(s);
    }

    void onCameraCapture(pxBuffer& frame)
    {
        // Please beware that this method is called back on another thread
        // Synchronizing with the main thread if necessary is left as an excercise for the
        // you

        mVideoWidth = frame.width();
        mVideoHeight = frame.height();

        // Draw it directly to the window bypassing the paint loop
        pxSurfaceNative s;
        if (beginNativeDrawing(s) == PX_OK)
        {
            frame.blit(s);
            endNativeDrawing(s);
        }
    }

    void onKeyDown(int keycode, unsigned long flags)
    {
        if (keycode == PX_KEY_TAB)
        {
            getACamera();
        }
    }

    int mVideoWidth;
    int mVideoHeight;
    pxOffscreen mTexture;
    pxCameras mCameras;
    pxCamera mCamera;
};

int pxMain()
{

// pxCamera uses COM on Windows
#ifdef PX_PLATFORM_WIN
    ::CoInitialize(NULL);
#endif

    {
        // The nesting inside the braces is important since pxCamera uses
        // COM and COM doesn't like it's objects to be destroyed after
        // CoUninitialize
        myWindow win;

        win.init(10, 64, 640, 480);
        win.setTitle("Simple");
        win.setVisibility(true);

        eventLoop.run();
    }

#ifdef PX_PLATFORM_WIN
    ::CoUninitialize();
#endif

    return 0;
}


