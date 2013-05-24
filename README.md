dxwrapper
=========

DirectX 1-7 wrapper project for making old games run on new hardware. This code is only a "pass-through" wrapper, which can be used as boilerplate for actual fixes (or a dx->opengl wrapper or whatnot).

Requires DirectX 8.1 SDK headers (although the parser should(tm) be able to handle just about any version), which the wrapper generator will parse to generate the interface wrappers for the various DirectX interfaces.

The wrapper is a full pass-through wrapper; it doesn't do anything special, just takes in calls from the application, writes the event to a log, and passes the arguments to the real DirectX.

downloads
=========

A binary package can be found at http://iki.fi/sol/zip/dxwrapper20130524.zip

Source to the binary (w/generated files) http://iki.fi/sol/zip/dxwrapper20130524_src.zip

license
=======

The whole thing, and the generated code, is released under zlib license:

-- 8< -- 8< -- 8< --

Copyright (c) 2013 Jari Komppa

This software is provided 'as-is', without any express or implied
warranty. In no event will the authors be held liable for any damages
arising from the use of this software.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

   1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment in the product documentation would be
   appreciated but is not required.

   2. Altered source versions must be plainly marked as such, and must not be
   misrepresented as being the original software.

   3. This notice may not be removed or altered from any source
   distribution.

-- 8< -- 8< -- 8< --

description
===========

The **wrappergen** directory contains the wrapper generator. This is used to turn the directx headers into classes like myIDirect3DDevice3.

The **wrapper** directory contains some files that the wrapper itself uses in addition to the generated files. The generated files are currently in order of 400kB, and change heavily whenever the wrapper generator is changed, so there's really no point in putting the generated files in git.

The basic idea is to extend DirectX COM interface classes, and then pass the extended classes to the application. The application doesn't have any clue that it's not talking directly to DirectX (usually. I'm pretty sure punkbuster or some such does have checks against this). Then, when the application calls a function, we can muck around with the parameters before passing it to DirectX, and/or muck around with the returning value. It's also entirely possible to just skip on talking to DirectX and make an OpenGL back-end, like I did with ddhack for Wing Commander games, but that's a lot of work.

Now, every time the application calls a function with a parameter that happens to be one of our wrapped classes, we also have to change these pointers to point at the original objects. This can get a bit tricky, and I'm sure I've missed several places (especially on interfaces I haven't seen in use yet). Just something to keep in mind when playing with it.

Sometimes the application calls some function that returns another wrappable object. Sometimes we've already wrapped this. For this, I've added a simple (slow, and stupid) database of pointer pairs that is checked whenever one of those functions is called.

It's entirely possible that other interfaces (like directmedia/directshow) eat or output directx objects, in which case we're either in trouble, or have to wrap more interfaces.

examples
========

As an example of a generated wrapper function, here's myIDirect3DDevice3::DrawPrimitiveVB from the current version:

```C++
HRESULT __stdcall myIDirect3DDevice3::DrawPrimitiveVB(D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFER b, DWORD c, DWORD d, DWORD e)
{
  EnterCriticalSection(&gCS);
  logf("myIDirect3DDevice3::DrawPrimitiveVB(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER 0x%x, DWORD %d, DWORD %d, DWORD %d);", b, c, d, e);
  HRESULT x = mOriginal->DrawPrimitiveVB(a, (b)?((myIDirect3DVertexBuffer *)b)->mOriginal:0, c, d, e);
  logfc(" -> return %d\n", x);
  pushtab();
  poptab();
  LeaveCriticalSection(&gCS);
  return x;
}
```

As you note, not everything is logged (as of yet) - it's possible to add code to produce better logs if some specific thing (like, in this case, the primitve type) is of interest. There is also no logging of modified data or complex structures as of yet. Then again, more logging, slower execution..

Sample clippet of the log created..
```C++
[      +7ms] myIDirect3DDevice3::SetTextureStageState(DWORD 0, D3DTEXTURESTAGESTATETYPE, DWORD 1); -> return 0
[      +7ms] myIDirect3DDevice3::SetTextureStageState(DWORD 0, D3DTEXTURESTAGESTATETYPE, DWORD 1); -> return 0
[      +7ms] myIDirect3DViewport3::SetViewport2(LPD3DVIEWPORT2 0xa17504); -> return 0
[      +7ms] myIDirectDrawSurface4::Lock(LPRECT 0x0, LPDDSURFACEDESC2 0x18df48, DWORD 1, HANDLE); -> return 0
[      +7ms] myIDirectDrawSurface4::Unlock(LPRECT 0x0); -> return 0
[      +7ms] myIDirectDrawSurface4::QueryInterface(REFIID, LPVOID FAR * 0x18dfe0); -> return 0
[      +7ms]    Interface Query: {93281502-8CF8-11D0-89AB-00A0C9054129}
[      +5ms] 		myIDirect3DTexture2 ctor
[      +5ms] 		Wrapped: IDirect3DTexture2
[      +5ms] myIDirect3DTexture2::Load(LPDIRECT3DTEXTURE2 0x2a9b5f0); -> return 0
[      +7ms] myIDirect3DTexture2::Release(); -> return 1
[      +7ms] myIDirect3DDevice3::SetRenderState(D3DRENDERSTATETYPE, DWORD 1); -> return 0
[      +7ms] myIDirect3DDevice3::SetRenderState(D3DRENDERSTATETYPE, DWORD 0); -> return 0
[      +7ms] myIDirect3DDevice3::SetTexture(DWORD 0, LPDIRECT3DTEXTURE2 0x2a99828); -> return 0
[      +7ms] myIDirect3DDevice3::SetTextureStageState(DWORD 0, D3DTEXTURESTAGESTATETYPE, DWORD 3); -> return 0
[      +7ms] myIDirect3DDevice3::SetTextureStageState(DWORD 0, D3DTEXTURESTAGESTATETYPE, DWORD 3); -> return 0
[      +7ms] myIDirect3DDevice3::DrawPrimitive(D3DPRIMITIVETYPE, DWORD 452, LPVOID 0x54a0020, DWORD 4, DWORD 24); -> return 0
```

status
======

Unless bugs are found, this pass-through wrapper project is now complete. The biggest issue in my mind is that printing to the log is rather slow, and this is apparently primarily due to printf being slow (outputting the log to a ramdisk didn't have much of an effect, and disabling logging makes everything rather fast, so the wrapping itself isn't an issue).

If you play with this code, toss me a note, I'm always interested in hearing about it, but I'm most likely too busy to actually help you (much) =)

Cheers,
   Jari
