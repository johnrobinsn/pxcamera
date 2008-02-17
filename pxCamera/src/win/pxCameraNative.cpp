// pxCamera Copyright 2007-2008 John Robinson
// pxCameraNative.cpp

#include "pxCamera.h"

#include <stdio.h>
#include <atlconv.h>

// finalize mediatype selection

//  Free an existing media type (ie free resources it holds)
void WINAPI FreeMediaType(AM_MEDIA_TYPE& mt)
{
    if (mt.cbFormat != 0) {
        CoTaskMemFree((PVOID)mt.pbFormat);

        // Strictly unnecessary but tidier
        mt.cbFormat = 0;
        mt.pbFormat = NULL;
    }
    if (mt.pUnk != NULL) {
        mt.pUnk->Release();
        mt.pUnk = NULL;
    }
}

// general purpose function to delete a heap allocated AM_MEDIA_TYPE structure
// which is useful when calling IEnumMediaTypes::Next as the interface
// implementation allocates the structures which you must later delete
// the format block may also be a pointer to an interface to release
void WINAPI DeleteMediaType(AM_MEDIA_TYPE *pmt)
{
    // allow NULL pointers for coding simplicity

    if (pmt == NULL) {
        return;
    }

    FreeMediaType(*pmt);
    CoTaskMemFree((PVOID)pmt);
}


HRESULT getBestMediaType(IPin* pPin, AM_MEDIA_TYPE** t)
{
    HRESULT hr = E_FAIL;

    rtRefPtr<IEnumMediaTypes> e;
    if (SUCCEEDED(pPin->EnumMediaTypes(e.ref())))
    {
        ULONG fetched;
        AM_MEDIA_TYPE *pt;
        int curBitCount = 0;
        int curWidth = 0;
        e->Reset();
        while (S_OK == (e->Next(1, &pt, &fetched)))
        {
            if( pt->formattype == FORMAT_VideoInfo && pt->majortype == MEDIATYPE_Video && pt->subtype == MEDIASUBTYPE_RGB24)
            {
                VIDEOINFOHEADER *vih = (VIDEOINFOHEADER *)pt->pbFormat;
                if (vih->bmiHeader.biWidth >= 640 && vih->bmiHeader.biBitCount >= 24)
                {
                    // Good Enough
                    *t = pt;
                    hr = S_OK;
                    break;
                }

            }
            DeleteMediaType(pt);
        }
    }
    return hr;
}

HRESULT getPin(IBaseFilter * pFilter, PIN_DIRECTION dirrequired, IPin **ppPin)
{
    *ppPin = NULL;

    bool found = false;
    rtRefPtr<IEnumPins> pEnum;
    
    HRESULT hr = pFilter->EnumPins(pEnum.ref());
    if(FAILED(hr)) 
        return hr;

    ULONG ulFound;
    IPin *pPin;
    hr = E_FAIL;

    hr = pEnum->Reset();
    while(S_OK == pEnum->Next(1, &pPin, &ulFound))
    {
        PIN_DIRECTION pindir = (PIN_DIRECTION)3;

        pPin->QueryDirection(&pindir);
        if(pindir == dirrequired)
        {
            if (!found)
            {
                *ppPin = pPin;  // Return the pin's interface
                (*ppPin)->AddRef();
                found = true;
            }

            hr = S_OK;
            break;
        } 

        pPin->Release();
    } 

    return hr;
}


void getInPin( IBaseFilter * pFilter, IPin** pin)
{
    getPin(pFilter, PINDIR_INPUT, pin);
}


void getOutPin( IBaseFilter * pFilter, IPin **pin )
{
    getPin(pFilter, PINDIR_OUTPUT, pin);
}


class grabberCB : public ISampleGrabberCB 
{
public:

    grabberCB(pxICameraCapture* capture, int width, int height)
    {
        mRefCount = 0;
        mCapture = capture;
        mWidth = width;
        mHeight = height;
    }

    STDMETHODIMP_(ULONG) AddRef() 
    { 
        mRefCount++;
        return mRefCount;
    }

    STDMETHODIMP_(ULONG) Release() 
    { 
        mRefCount--;
        if (mRefCount == 0)
        {
            delete this;
        }
        return mRefCount;
    }

    STDMETHODIMP QueryInterface(REFIID riid, void ** ppv)
    {
        if( riid == IID_ISampleGrabberCB || riid == IID_IUnknown ) 
        {
            *ppv = (void *) static_cast<ISampleGrabberCB*> ( this );
            return S_OK;
        }    

        if (*ppv)
        {
            ((IUnknown*)*ppv)->AddRef();
        }
        return E_NOINTERFACE;
    }

    STDMETHODIMP SampleCB( double SampleTime, IMediaSample * pSample )
    {
        return 0;
    }

	STDMETHODIMP BufferCB( double dblSampleTime, BYTE * pBuffer, long lBufferSize )
    {
        pxBuffer b;
        b.setBase(pBuffer);
        b.setWidth(mWidth);
        b.setHeight(mHeight);
        b.setStride(mWidth*4);
        b.setUpsideDown(true);
        
        mCapture->onCameraCapture(b);

        return S_OK;
    }

public:
    int mWidth;
    int mHeight;
    ULONG mRefCount;
    pxICameraCapture* mCapture;
};

pxCameras::pxCameras()
{
    // Nothing to do
}

pxCameras::~pxCameras()
{
    term();
}

pxError pxCameras::init()
{
    return reset();
}

pxError pxCameras::term()
{
    mMkEnum = NULL;
    mDvEnum = NULL;
    return PX_OK;
}

pxError pxCameras::reset()
{
    pxError e = PX_FAIL;

    term();
    // enumerate all video capture devices
    if (SUCCEEDED(CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC_SERVER,
			  IID_ICreateDevEnum, (void**)mDvEnum.ref())))
    {
        if (SUCCEEDED(mDvEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory,
								mMkEnum.ref(), 0)))
        {            
            mMkEnum->Reset();
            e = PX_OK;
        }
    }

    return e;
}

bool pxCameras::next(pxCamera& camera)
{
    USES_CONVERSION;
    bool foundCamera = false;
    ULONG cFetched;
    rtRefPtr<IMoniker> moniker;
    HRESULT hr = E_FAIL;
    if (S_OK == mMkEnum->Next(1, moniker.ref(), &cFetched))
    {
        rtRefPtr<IBindCtx> bc;
        CreateBindCtx(NULL, bc.ref());
        LPOLESTR n;
        moniker->GetDisplayName(bc, NULL, &n);
		rtRefPtr<IPropertyBag> pBag;
		hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)&pBag);
		if(SUCCEEDED(hr)) 
		{
			VARIANT var;
			var.vt = VT_BSTR;
			hr = pBag->Read(L"FriendlyName", &var, NULL);
			if (hr == NOERROR) 
			{

                //moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)pFilter);

                if (PX_OK == camera.init(W2A(n)))
                {
                    foundCamera = true;
                }

				SysFreeString(var.bstrVal);
			}
		}
    }
    return foundCamera;
}


pxCamera::pxCamera()
{
    mName = NULL;
    mId = NULL;
}

pxCamera::~pxCamera()
{
    term();
}

pxError pxCamera::init(char* id)
{
    USES_CONVERSION;

    pxError e = PX_FAIL;

    term();

    rtRefPtr<IBindCtx> bindCtx;
    if (SUCCEEDED(CreateBindCtx(NULL, bindCtx.ref())))
    {
        rtRefPtr<IMoniker> moniker;
        ULONG eaten;
        if (SUCCEEDED(MkParseDisplayName(bindCtx, A2W((char*)id), &eaten, moniker.ref())))
        {
            rtRefPtr<IPropertyBag> pBag;
		    HRESULT hr = moniker->BindToStorage(0, 0, IID_IPropertyBag, (void **)pBag.ref());
		    if(SUCCEEDED(hr)) 
		    {
			    VARIANT var;
			    var.vt = VT_BSTR;
			    hr = pBag->Read(L"FriendlyName", &var, NULL);
			    if (hr == NOERROR) 
			    {
					if (SUCCEEDED(moniker->BindToObject(0, 0, IID_IBaseFilter, (void**)mCamera.ref())))
                    {
                        if (mCamera)
                        {
                            mName = strdup(W2A(var.bstrVal));
                            mId = strdup(id);
                            e = PX_OK;
                        }
                    }
				    SysFreeString(var.bstrVal);
			    }
		    }
        }
    }

    return e;
}

pxError pxCamera::term()
{
    stopCapture();
    if (mName)
    {
        free(mName);
        mName = NULL;
    }
    if (mId)
    {
        free(mId);
        mId = NULL;
    }

    return PX_OK;
}

pxCamera::operator bool()
{
    return mCamera?true:false;
}

// Returns an opaque unique identifier for a camera
char* pxCamera::id()
{
    return mId;
}

// Returns a human readable name for the camera
char* pxCamera::name()
{
    return mName;
}

// This method will start capturing and callback the
// provided callback object for each frame
pxError pxCamera::startCapture(pxICameraCapture* callback)
{
    pxError e = PX_FAIL;

    HRESULT hr;

    if (mCamera)
    {
        stopCapture();

        rtRefPtr<ISampleGrabber> grabber;
        rtRefPtr<IBaseFilter> grabberBase;

        if (FAILED(CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
                IID_ISampleGrabber, (void**)grabber.ref() )))
            return PX_FAIL;

        if (FAILED(grabber->SetBufferSamples( FALSE )))
            return PX_FAIL;

        if (FAILED(grabber->QueryInterface(IID_IBaseFilter, (void**)grabberBase.ref())))
            return PX_FAIL;

        if (FAILED(CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC_SERVER,
			    IID_IGraphBuilder, (void**)graph.ref())))
            return PX_FAIL;
        
        hr = graph->AddFilter( mCamera, L"camera");
        hr = graph->AddFilter( grabberBase, L"grabber");

        rtRefPtr<IPin> sourcePin;
        rtRefPtr<IPin> grabPin;

        getOutPin( mCamera, sourcePin.ref() );
        getInPin( grabberBase, grabPin.ref() );

        if (!sourcePin || !grabPin)
        {
            return PX_FAIL;
        }

        rtRefPtr<IAMStreamConfig> config;
        if (SUCCEEDED(sourcePin->QueryInterface( IID_IAMStreamConfig, (void **) config.ref() )))
        {
            AM_MEDIA_TYPE *mt;
            if (SUCCEEDED(getBestMediaType(sourcePin, &mt)))
            {
                mt->subtype = MEDIASUBTYPE_RGB24;
                hr=config->SetFormat( mt );
                DeleteMediaType(mt);
            }
        }

        AM_MEDIA_TYPE mt;
        ZeroMemory(&mt, sizeof(AM_MEDIA_TYPE));
        mt.majortype = MEDIATYPE_Video;
        mt.subtype = MEDIASUBTYPE_RGB32;  
        hr = grabber->SetMediaType(&mt);    
        FreeMediaType(mt);

        if (FAILED(hr))
            return PX_FAIL;

        if (FAILED(graph->Connect(sourcePin, grabPin)))
            return PX_FAIL;

        int vWidth, vHeight;

        {
            AM_MEDIA_TYPE mt;
            if (SUCCEEDED(grabber->GetConnectedMediaType(&mt)))
            {
                VIDEOINFOHEADER * vih = (VIDEOINFOHEADER*) mt.pbFormat;
                vWidth  = vih->bmiHeader.biWidth;
                vHeight = vih->bmiHeader.biHeight;
                FreeMediaType( mt );

            }
            else 
                return PX_FAIL;
        }

        rtRefPtr<grabberCB> cb = new grabberCB(callback, vWidth, vHeight);
        if (cb)
        {
            if (FAILED(grabber->SetCallback( cb, 1 )))
                return PX_FAIL;
        }

        rtRefPtr<IPin> grabOutPin;
        getOutPin(grabberBase, grabOutPin.ref());
        if (FAILED(graph->Render(grabOutPin)))
            return PX_FAIL;

        // Disable the video capture window
        rtRefPtr<IVideoWindow> videoWindow;
        graph->QueryInterface(IID_IVideoWindow, (void**)videoWindow.ref());
        if (videoWindow)
        {
            hr = videoWindow->put_AutoShow(OAFALSE);
        }

        rtRefPtr<IMediaControl> control;
        graph->QueryInterface(IID_IMediaControl, (void**)control.ref());

        hr = control->Run( );
        
        e = PX_OK;
    }
    return e;
}

pxError pxCamera::stopCapture()
{
    if (graph)
    {
        rtRefPtr<IMediaControl> control;
        if (SUCCEEDED(graph->QueryInterface(IID_IMediaControl, (void**)control.ref())))
        {
            control->Stop();
        }
        graph = NULL;
    }
    return PX_OK;
}
