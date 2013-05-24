#define _CRT_SECURE_NO_WARNINGS
#define STRICT
#define WIN32_LEAN_AND_MEAN

#include <windows.h>
#include <shlobj.h>
#include <stdio.h>
#include <stdlib.h>
#include <varargs.h>
//#include "ddoutput.h"

CRITICAL_SECTION gCS;

bool _stdcall SetWindowPosHook(HWND, HWND, int, int, int, int, int) { return true; }
bool _stdcall SetWindowLongHook(HWND, int, int) { return true; }
bool _stdcall ShowWindowHook(HWND, int) { return true; }

bool _stdcall DllMain(HANDLE, DWORD dwReason, LPVOID) {
	if(dwReason==DLL_PROCESS_ATTACH) {
		InitializeCriticalSection(&gCS);
	} else if(dwReason==DLL_PROCESS_DETACH) {
		//d3d9Exit();
		DeleteCriticalSection(&gCS);
	}
	return true;
}

//#define DISABLE_LOGGING
#define MAX_TABDEPTH 100

int gLoglines = 0;
int gTabStops = 0;
int gLastTick = 0;

void pushtab()
{
	gTabStops++;
}

void poptab()
{
	gTabStops--;
	if (gTabStops < 0)
		gTabStops = 0;
}

long long milliseconds_now() {
    static LARGE_INTEGER s_frequency;
    static BOOL s_use_qpc = QueryPerformanceFrequency(&s_frequency);
    if (s_use_qpc) {
        LARGE_INTEGER now;
        QueryPerformanceCounter(&now);
        return (1000LL * now.QuadPart) / s_frequency.QuadPart;
    } else {
        return GetTickCount();
    }
}

FILE * openlog()
{
	char fname[]="wrapper0.log";
	fname[7] = '0' + (gLoglines >> 20); // new file every ~1024k lines

	return fopen(fname, "a");
}

void logfc(char * fmt, ...)
{
#ifndef DISABLE_LOGGING
	if (gTabStops > MAX_TABDEPTH)
		return;
	
	va_list ap;
	va_start(ap, fmt);
	FILE *f = openlog();
	if (f)
	{
		vfprintf(f, fmt, ap);
		fclose(f);
	}
	va_end(ap);
#endif
}

void logf(char * fmt, ...)
{
#ifndef DISABLE_LOGGING
	if (gTabStops > MAX_TABDEPTH)
		return;

	gLoglines++;
	int tick = (int)milliseconds_now();
	logfc("[%+8dms] ", tick - gLastTick);
	gLastTick = tick;
	va_list ap;
	va_start(ap, fmt);
	FILE *f = openlog();
	if (f)
	{
		int i;
		for (i = 0; i < gTabStops; i++) fputc('\t', f);
		vfprintf(f, fmt, ap);
		fclose(f);
	}
	va_end(ap);
#endif
}

struct WrapPair
{
	void * mOriginal;
	void * mWrapper;
};

#define MAX_PAIRS 65536
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
				gWrapPair[i].mWrapper = aWrapper;
				return;
			}
		}
	}

	if (gWrapPairs >= MAX_PAIRS)
	{
		logf("**** Max number of wrappers exceeded - adjust and recompile\n");
	}
}

void * wrapfetch(void * aOriginal)
{
	void * ret = do_wrapfetch(aOriginal);

/*	if (ret == NULL)
	{
		logf("Pre-wrapped object not found - returning null\n");
	}
*/
	return ret;
}