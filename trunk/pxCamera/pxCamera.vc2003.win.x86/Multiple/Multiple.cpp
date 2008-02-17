// Multiple Example CopyRight 2007-2008 John Robinson
// Demonstrates capturing frames from the pxCamera class from multiple attached cameras

#include "pxCore.h"
#include "pxEventLoop.h"
#include "pxWindow.h"
#include "pxOffscreen.h"

#include "pxCamera.h"

#include <stdio.h>

pxEventLoop eventLoop;

int windowPos = 0;
const int windowOffset = 64;

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

class captureWindow: public pxWindow, public pxICameraCapture
{
public:

    captureWindow()
    {
        mVideoWidth = mVideoHeight = 0;
    }

    pxError init(char* id, char* name)
    {
        pxError e = pxWindow::init(windowPos, windowPos, 640, 480);
        if (PX_OK == e)
        {
            // Stagger the next one
            windowPos += windowOffset;

            setTitle(name);

            e = mCamera.init(id);
            if (PX_OK == e)
            {
                mCamera.startCapture(this);
            }
        }
        return e;
    }

private:

    // Event Handlers - Look in pxWindow.h for more

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

    int mVideoWidth;
    int mVideoHeight;
    pxOffscreen mTexture;
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

        const int MAXCAMERAS = 2;

        pxCameras c;
        int camerasFound = 0;
        captureWindow win[MAXCAMERAS];

        if (PX_OK == c.init())
        {
            pxCamera camera;
            while (c.next(camera))
            {
                // Create a capture window per camera
                // up to 2...  Change the number and let me know if it works with more :-)

                win[camerasFound].init(camera.id(), camera.name());
                win[camerasFound].setVisibility(true);

                camerasFound++;
                if (camerasFound >= MAXCAMERAS) break;
            }
        }     

        eventLoop.run();
    }

#ifdef PX_PLATFORM_WIN
    ::CoUninitialize();
#endif

    return 0;
}


