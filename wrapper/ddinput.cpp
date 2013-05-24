#define STRICT
#define WIN32_LEAN_AND_MEAN
#define INITGUID
#define DIRECTDRAW_VERSION 0x0700
#define DIRECT3D_VERSION   0x0700

#include <windows.h>
#include <ddraw.h>
#include "d3d.h"
#include "ddoutput.h"
#include "wrapper.h"

#ifdef _DEBUG
#define DEBUGMESS(a) OutputDebugStringA(a)
#else
#define DEBUGMESS(a)
#endif


typedef HRESULT (_stdcall *DDrawCreateProc)(void* a, void* b, void* c);
typedef HRESULT (_stdcall *DDrawEnumerateProc)(void* callback, void* context);
typedef void (_stdcall *DDrawMiscProc)();
typedef HRESULT (_stdcall *DDrawCreateExProc)(GUID FAR *lpGUID, LPVOID *lplpDD, REFIID iid, IUnknown FAR *pUnkOuter);

static DDrawCreateProc DDrawCreate=0;
static DDrawEnumerateProc DDrawEnumerate=0;
static DDrawMiscProc AcquireLock;
static DDrawMiscProc ParseUnknown;
static DDrawMiscProc InternalLock;
static DDrawMiscProc InternalUnlock;
static DDrawMiscProc ReleaseLock;
static DDrawCreateExProc DDrawCreateEx;

static void LoadDLL() {
	char path[MAX_PATH];
	GetSystemDirectoryA(path,MAX_PATH);
	strcat_s(path, "\\ddraw.dll");	
	HMODULE ddrawdll=LoadLibraryA(path);
	//HMODULE ddrawdll=LoadLibraryA("csfix.dll");
	DDrawCreate=(DDrawCreateProc)GetProcAddress(ddrawdll, "DirectDrawCreate");
	DDrawEnumerate=(DDrawEnumerateProc)GetProcAddress(ddrawdll, "DirectDrawEnumerateA");
	DDrawCreateEx=(DDrawCreateExProc)GetProcAddress(ddrawdll, "DirectDrawCreateEx");

	AcquireLock=(DDrawMiscProc)GetProcAddress(ddrawdll, "AcquireDDThreadLock");
	ParseUnknown=(DDrawMiscProc)GetProcAddress(ddrawdll, "D3DParseUnknownCommand");
	InternalLock=(DDrawMiscProc)GetProcAddress(ddrawdll, "DDInternalLock");
	InternalUnlock=(DDrawMiscProc)GetProcAddress(ddrawdll, "DDInternalUnlock");
	ReleaseLock=(DDrawMiscProc)GetProcAddress(ddrawdll, "ReleaseDDThreadLock");
}

extern "C" void __declspec(naked) myAcquireLock() { 
	logf(__FUNCTION__ "\n");
	_asm jmp AcquireLock;
}
extern "C" void __declspec(naked) myParseUnknown() { 
	logf(__FUNCTION__ "\n");
	_asm jmp ParseUnknown;
}
extern "C" void __declspec(naked) myInternalLock() {
	logf(__FUNCTION__ "\n");
	_asm jmp InternalLock;
}
extern "C" void __declspec(naked) myInternalUnlock() {
	logf(__FUNCTION__ "\n");
	_asm jmp InternalUnlock;
}
extern "C" void __declspec(naked) myReleaseLock() {
	logf(__FUNCTION__ "\n");
	_asm jmp ReleaseLock;
}

extern "C" HRESULT _stdcall myDirectDrawCreate(GUID* a, IDirectDraw** b, IUnknown* c) {
	logf(__FUNCTION__ "\n");
	if(!DDrawCreate) LoadDLL();
	HRESULT hr=DDrawCreate(a,b,c);
	if(FAILED(hr)) return hr;
	pushtab();
	*b=(IDirectDraw*)new myIDirectDraw(*b);
	poptab();
	return 0;
}

extern "C" HRESULT _stdcall myDirectDrawCreateEx(
  GUID FAR *lpGUID,
  LPVOID *lplpDD,
  REFIID iid,
  IUnknown FAR *pUnkOuter
)
{
	logf(__FUNCTION__ "\n");
	if(!DDrawCreate) LoadDLL();
	HRESULT hr = DDrawCreateEx(lpGUID, lplpDD, iid, pUnkOuter);
	pushtab();
	genericQueryInterface(iid, lplpDD);
	poptab();
	return hr;
}

extern "C" HRESULT _stdcall myDirectDrawEnumerate(void* lpCallback, void* lpContext) {
	logf(__FUNCTION__ "\n");
	if(!DDrawEnumerate) LoadDLL();
	return DDrawEnumerate(lpCallback, lpContext);
}

