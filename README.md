dxwrapper
=========

DirectX 1-7 wrapper project for making old games run on new hardware.

Requires DirectX 8.1 SDK headers (although the parser should(tm) be able to handle just about any version), which the wrapper generator will parse to generate the interface wrappers for the various DirectX interfaces.

At its current form, the wrapper is a full pass-through wrapper; it doesn't do anything special, just takes in calls from the application, writes the event to a log, and passes the arguments to the real DirectX.

There's bound to be plenty of bugs left, but I've managed to get a 100k-line log out of Crimson Skies so far..

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

The wrappergen directory contains the wrapper generator. This is used to turn the directx headers into classes like myIDirect3DDevice3.

The wrapper directory contains some files that the wrapper itself uses in addition to the generated files. The generated files are currently in order of 330kB, and change heavily whenever the wrapper generator is changed, so there's really no point in putting the generated files in git.

The basic idea is to extend DirectX COM interface classes, and then pass the extended classes to the application. The application doesn't have any clue that it's not talking directly to DirectX (usually. I'm pretty sure punkbuster or some such does have checks against this). Then, when the application calls a function, we can muck around with the parameters before passing it to DirectX, and/or muck around with the returning value. It's also entirely possible to just skip on talking to DirectX and make an OpenGL back-end, like I did with ddhack for Wing Commander games, but that's a lot of work.

Now, every time the application calls a function with a parameter that happens to be one of our wrapped classes, we also have to change these pointers to point at the original objects. This can get a bit tricky, and I'm sure I've missed several places (especially on interfaces I haven't seen in use yet). Just something to keep in mind when playing with it.

Sometimes the application calls some function that returns another wrappable object. Sometimes we've already wrapped this. For this, I've added a simple (slow, and stupid) database of pointer pairs that is checked whenever one of those functions is called. Sometimes this can cause a false warning in the log, since the framebuffer, for example, doesn't need to be created implicitly, but is an attached surface. Or something. I've added code to wrap it when it's queried for the first time.

It's entirely possible that other interfaces (like directmedia/directshow) eat or output directx objects, in which case we're either in trouble, or have to wrap more interfaces.

As an example of a generated wrapper function, here's myIDirect3DDevice3::DrawPrimitiveVB from the current version:

```c++
HRESULT __stdcall myIDirect3DDevice3::DrawPrimitiveVB(D3DPRIMITIVETYPE a, LPDIRECT3DVERTEXBUFFER b, DWORD c, DWORD d, DWORD e)
{
  logf("myIDirect3DDevice3::DrawPrimitiveVB(D3DPRIMITIVETYPE, LPDIRECT3DVERTEXBUFFER[%08x], DWORD[%d], DWORD[%d], DWORD[%d]);", b, c, d, e);
  pushtab();
  HRESULT x = mOriginal->DrawPrimitiveVB(a, (b)?((myIDirect3DVertexBuffer *)b)->mOriginal:0, c, d, e);
  logfc(" -> return [%d]\n", x);
  poptab();
  return x;
}
```

As you note, not everything is logged (as of yet) - it's possible to add code to produce better logs if some specific thing (like, in this case, the primitve type) is of interest. There is also no logging of modified data or complex structures as of yet. Then again, more logging, slower execution..

Sample clippet of the log created..
```c++
[     +15ms] myIDirect3DDevice3::SetTexture(DWORD[0], LPDIRECT3DTEXTURE2[03418488]); -> return [0]
[     +16ms] myIDirect3DDevice3::DrawPrimitive(D3DPRIMITIVETYPE, DWORD[452], LPVOID[059c0020], DWORD[4], DWORD[24]); -> return [0]
[      +0ms] myIDirectDraw4::CreateSurface(LPDDSURFACEDESC2[0018f7d8], LPDIRECTDRAWSURFACE4 FAR *[0018f7ac], IUnknown FAR *); -> return [0]
[     +15ms] 	myIDirectDrawSurface4 ctor
[      +0ms] 	Wrapped surface.
[     +16ms] myIDirectDrawSurface4::Lock(LPRECT[00000000], LPDDSURFACEDESC2[0018f70c], DWORD[1], HANDLE); -> return [0]
[      +0ms] myIDirectDrawSurface4::Unlock(LPRECT[00000000]); -> return [0]
[     +16ms] myIDirectDraw4::CreateSurface(LPDDSURFACEDESC2[0018f7d8], LPDIRECTDRAWSURFACE4 FAR *[0018f7b0], IUnknown FAR *); -> return [0]
[     +15ms] 	myIDirectDrawSurface4 ctor
[      +0ms] 	Wrapped surface.
```
If you play with this code, toss me a note, I'm always interested in hearing about it, but I'm most likely too busy to actually help you (much) =)

Cheers,
   Jari