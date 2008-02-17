// pxCamera Copyright 2007-2008 John Robinson
// pxCameraNative.h

#include <windows.h>
#include <qedit.h>
#include <dshow.h>

#include "rtRefPtr.h"

class pxCamerasNative
{
protected:
	rtRefPtr<ICreateDevEnum>    mDvEnum;
    rtRefPtr<IEnumMoniker>      mMkEnum;
};

class pxCameraNative
{
protected:
    char* mId;
    char* mName;
    rtRefPtr<IBaseFilter> mCamera;
    rtRefPtr<IGraphBuilder>  graph;
};