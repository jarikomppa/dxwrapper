#pragma once
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "ddraw.h"
#include "ddrawex.h"
#include "d3d.h"

#include "myIDirect3D.h"
#include "myIDirect3D2.h"
#include "myIDirect3D3.h"
#include "myIDirect3D7.h"
#include "myIDirect3DDevice.h"
#include "myIDirect3DDevice2.h"
#include "myIDirect3DDevice3.h"
#include "myIDirect3DDevice7.h"
#include "myIDirect3DExecuteBuffer.h"
#include "myIDirect3DLight.h"
#include "myIDirect3DMaterial.h"
#include "myIDirect3DMaterial2.h"
#include "myIDirect3DMaterial3.h"
#include "myIDirect3DTexture.h"
#include "myIDirect3DTexture2.h"
#include "myIDirect3DVertexBuffer.h"
#include "myIDirect3DVertexBuffer7.h"
#include "myIDirect3DViewport.h"
#include "myIDirect3DViewport2.h"
#include "myIDirect3DViewport3.h"
#include "myIDirectDraw.h"
#include "myIDirectDraw2.h"
#include "myIDirectDraw3.h"
#include "myIDirectDraw4.h"
#include "myIDirectDraw7.h"
#include "myIDirectDrawClipper.h"
#include "myIDirectDrawColorControl.h"
#include "myIDirectDrawFactory.h"
#include "myIDirectDrawGammaControl.h"
#include "myIDirectDrawPalette.h"
#include "myIDirectDrawSurface.h"
#include "myIDirectDrawSurface2.h"
#include "myIDirectDrawSurface3.h"
#include "myIDirectDrawSurface4.h"
#include "myIDirectDrawSurface7.h"

extern CRITICAL_SECTION gCS;

void logf(char * format, ...);
void logfc(char * format, ...);
void genericQueryInterface(REFIID riid, LPVOID * ppvObj);
void wrapstore(void * aOriginal, void * aWrapper);
void *wrapfetch(void * aOriginal);
void pushtab();
void poptab();

#ifdef _DEBUG
#define DEBUGMESS(a) OutputDebugStringA(a)
#else
#define DEBUGMESS(a)
#endif


#define STRINGIFY(x) STRINGIFY_INNER(x)
#define STRINGIFY_INNER(x) #x

//#define WARNMODE_NONE
//#define WARNMODE_MESSAGE
#define WARNMODE_BREAK

#if defined(WARNMODE_NONE)
#define UNDEFINED return E_FAIL;
#define UNDEFINED_(x) return (x);
#define UNDEFINED_void return;
#define UNDEFINED_WARN
#elif defined(WARNMODE_BREAK)
#define UNDEFINED_WARN DEBUGMESS(__FILE__ "(" STRINGIFY(__LINE__) "): Undefined function called: " __FUNCTION__ "\r\n"); DebugBreak(); 
#define UNDEFINED { UNDEFINED_WARN; return 0; }
#define UNDEFINED_(x) { UNDEFINED_WARN; return (x); }
#define UNDEFINED_void { UNDEFINED_WARN; return; }
#else // WARNMODE_MESSAGE
#define UNDEFINED_WARN DEBUGMESS(__FILE__ "(" STRINGIFY(__LINE__) "): Undefined function called: " __FUNCTION__ "\r\n");
#define UNDEFINED { UNDEFINED_WARN; return 0; }
#define UNDEFINED_(x) { UNDEFINED_WARN; return (x); }
#define UNDEFINED_void { UNDEFINED_WARN; return; }
#endif

#define DEFINED_NOTE DEBUGMESS(__FILE__ "(" STRINGIFY(__LINE__) "): Defined function called: " __FUNCTION__ "\r\n");
