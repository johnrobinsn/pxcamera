// Histogram Example CopyRight 2007 John Robinson
// Demonstrates capturing frames from the pxCamera and drawing a grayscale histogram
// based on the captured frames

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
            sprintf(buffer, "Grayscale Histogram - %s", mCamera.name());
        else
            strcpy(buffer, "Grayscale Histogram - Please attach a camera and then press <TAB>");

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

        // Let's sync up with the windows thread
        // so that we can access the background offscreen
        // safely
        sendSynchronizedMessage("drawHistogram", &frame);
    }

    void onSynchronizedMessage(char* messageName, void *p1)
    {
        if (!strcmp(messageName, "drawHistogram"))
        {
            drawBackground(mTexture);

            pxBuffer* b = (pxBuffer*)p1;

            long histogram[256];
            memset(histogram, 0, sizeof(long)*256);

            // calc grayscale value per pixel and inc histogram appropriately
            for (int y = 0; y < b->height(); y++)
            {
                pxPixel *p = b->scanline(y);
                pxPixel *pe = p + b->width();
                while(p < pe)
                {
                    int gray = (p->r + p->g + p->b)/3;
                    histogram[gray]++;
                    p++;
                }
            }

            // find the max value so we can scale
            int maxValue = 0;
            for (int j = 0; j < 256; j++)
                if (histogram[j] > maxValue) maxValue = histogram[j];

            // render the histogram into the back buffer
            pxRect r;
            const int graphBottom = 440;
            const int graphHeight = 400;
            const int graphBarWidth = 2;

            for (int i = 0; i < 256; i++)
            {
                int barLeft = (255-i) * graphBarWidth;
                int barRight = barLeft + graphBarWidth;

                r.setLeft(barLeft);
                r.setRight(barRight);
                r.setBottom(graphBottom);

                double scaled = (double)histogram[i]/(double)maxValue;
                int h = (int)((double)graphHeight * scaled);
                r.setTop(graphBottom-h);

                pxColor c;
                c.r = c.g = c.b = i;
                mTexture.fill(r, c);
            }

            // blit the backbuffer to the screen
            pxSurfaceNative s;
            if (PX_OK == beginNativeDrawing(s))
            {
                mTexture.blit(s);
                endNativeDrawing(s);
            }
        }
    }

    void onKeyDown(int keycode, unsigned long flags)
    {
        if (keycode == PX_KEY_TAB)
        {
            getACamera();
        }
    }

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


