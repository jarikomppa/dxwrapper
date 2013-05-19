#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <varargs.h>
//#include "ddoutput.h"

bool _stdcall SetWindowPosHook(HWND, HWND, int, int, int, int, int) { return true; }
bool _stdcall SetWindowLongHook(HWND, int, int) { return true; }
bool _stdcall ShowWindowHook(HWND, int) { return true; }

bool _stdcall DllMain(HANDLE, DWORD dwReason, LPVOID) {
	if(dwReason==DLL_PROCESS_ATTACH) {
	} else if(dwReason==DLL_PROCESS_DETACH) {
		//d3d9Exit();
	}
	return true;
}

void logf(char * fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	FILE *f = fopen("wrapper.log", "a");
	if (f)
	{
		vfprintf(f, fmt, ap);
		fclose(f);
	}
	va_end(ap);
}

struct WrapPair
{
	void * mOriginal;
	void * mWrapper;
};

#define MAX_PAIRS 4096
WrapPair gWrapPair[MAX_PAIRS];
int gWrapPairs = 0;

void * do_wrapfetch(void * aOriginal)
{
	int i;
	for (i = 0; i < gWrapPairs; i++)
	{
		if (gWrapPair[i].mOriginal == aOriginal)
		{
			return gWrapPair[i].mWrapper;
		}
	}
	return NULL;
}

void wrapstore(void * aOriginal, void * aWrapper)
{
	if (do_wrapfetch(aOriginal) == NULL)
	{
		gWrapPair[gWrapPairs].mOriginal = aOriginal;
		gWrapPair[gWrapPairs].mWrapper = aWrapper;
		gWrapPairs++;
	}
	else
	{
		int i;
		for (i = 0; i < gWrapPairs; i++)
		{
			if (gWrapPair[i].mOriginal == aOriginal)
			{
				gWrapPair[i].mWrapper == aWrapper;
				return;
			}
		}
	}

	if (gWrapPairs >= MAX_PAIRS)
	{
		logf("\n\t**** Max number of wrappers exceeded - adjust and recompile\n");
	}
}

void * wrapfetch(void * aOriginal)
{
	void * ret = do_wrapfetch(aOriginal);

	if (ret == NULL)
	{
		logf("\t**** Wrapped object not found - returning null - brace yourself\n");
	}
	return ret;
}