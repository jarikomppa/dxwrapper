#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <vector>
#include <string>

#define VERSION "ddwrappergen 1.130524 (c)2013 Jari Komppa http://iki.fi/sol/"

//#define DISABLE_LOGGING
//#define DISABLE_CRITICAL_SECTION
//#define DISABLE_PASSTHROUGH

using namespace std;

struct Method
{
	string mRetType;
	string mFuncName;
	vector<string> mParmType;
	vector<string> mParmName;
};

struct Iface
{
	string mName;
	string mLPName;
	string mWrapperName;
	vector<Method *> mMethod;
};

vector<Iface *>gIface;

char * loadfile(const char *aFilename)
{
	FILE * f;
	f = fopen(aFilename, "rb");
	fseek(f, 0, SEEK_END);
	int len = ftell(f);
	fseek(f, 0, SEEK_SET);
	char * buf = new char[len+1];
	buf[len] = 0;
	fread(buf,1,len,f);
	return buf;
}

int is_whitespace(char c)
{
	if (c == ' ') return 1;
	if (c == '\t') return 1;
//	if (c == '\n') return 1;
	if (c == '\r') return 1;
	return 0;
}

int is_alphanumeric(char c)
{
	if (c >= '0' && c <= '9')
		return 1;
	if (c >= 'a' && c <= 'z')
		return 1;
	if (c >= 'A' && c <= 'Z')
		return 1;
	if (c == '_')
		return 1;
	return 0;
}

string token(char * buf, int &ofs)
{
	string s = "";

	while (is_whitespace(buf[ofs])) ofs++;
	
	if (is_alphanumeric(buf[ofs]))
	{
		while (is_alphanumeric(buf[ofs]))
		{
			s += buf[ofs];
			ofs++;
		}
	}
	else
	{
		s += buf[ofs];
		ofs++;
		if ((buf[ofs-1] == '/' && buf[ofs] == '/') ||
			(buf[ofs-1] == '/' && buf[ofs] == '*') ||
			(buf[ofs-1] == '*' && buf[ofs] == '/') ||
			(buf[ofs-1] == '=' && buf[ofs] == '=') ||
			(buf[ofs-1] == '!' && buf[ofs] == '=') ||
			(buf[ofs-1] == '<' && buf[ofs] == '=') ||
			(buf[ofs-1] == '>' && buf[ofs] == '=') ||
			(buf[ofs-1] == '-' && buf[ofs] == '=') ||
			(buf[ofs-1] == '+' && buf[ofs] == '=') ||
			(buf[ofs-1] == '+' && buf[ofs] == '+') ||
			(buf[ofs-1] == '-' && buf[ofs] == '-') ||
			(buf[ofs-1] == '/' && buf[ofs] == '=') ||
			(buf[ofs-1] == '*' && buf[ofs] == '=') ||
			(buf[ofs-1] == '%' && buf[ofs] == '='))
		{
			s += buf[ofs];
			ofs++;
		}		
	}
	return s;
}

#define PARSEERROR { printf("Parse error near \"%s\"", s.c_str()); exit(0); }
#define EXPECT(x) if (token(b,ofs) != x) PARSEERROR
#define IGNORE token(b,ofs);
#define NEXTTOKEN s = token(b,ofs);

void parse(const char *aFilename, int aPrintProgress = 0)
{
	printf("Parsing %s..\n", aFilename);
	char *b = loadfile(aFilename);
	int ofs = 0;
	string s;
	int parsestate = 0;
	int i = 0;
	int newline = 0;
	int parnameidx = 0;
	Method * method;
	Iface * iface;

	while (b[ofs])
	{
		NEXTTOKEN;
		if (s == "#" && newline)
		{
			while (token(b,ofs) != "\n") {}
			newline = 1;
		}
		else
		if (s == "//")
		{
			while (token(b,ofs) != "\n") {}
			newline = 1;
		}
		else
		if (s == "/*")
		{
			while (token(b,ofs) != "*/") {}
			newline = 0;
		}
		else
		if (s == "\"")
		{
			while (token(b,ofs) != "\"") {}
		}
		else
		if (s == "\n")
		{
			newline = 1;
		}
		else
		{
			newline = 0;
			switch (parsestate)
			{
			case 0:
				if (s == "DECLARE_INTERFACE_")
				{
					parsestate++;
					EXPECT("(");
					NEXTTOKEN;
					if (aPrintProgress) printf("Start interface %s\n", s.c_str());
					iface = new Iface;
					iface->mName = s;
					iface->mWrapperName = "my" + s;
					iface->mLPName = "lp" + s.substr(1, s.length());
					int i;
					for (i = 0; i < (signed)iface->mLPName.length(); i++)
						iface->mLPName[i] = toupper(iface->mLPName[i]);
//					printf("'%s' '%s' '%s'\n", iface->mName.c_str(), iface->mWrapperName.c_str(), iface->mLPName.c_str());
					EXPECT(",");
					IGNORE;
					EXPECT(")");
					EXPECT("\n");
					EXPECT("{");
				}			
				break;
			case 1:
				parnameidx = 0;
				if (s == "STDMETHOD")
				{
					EXPECT("(");
					NEXTTOKEN;
					if (aPrintProgress) printf("\tMethod %s, returns HRESULT\n", s.c_str());
					method = new Method;
					method->mFuncName = s;
					method->mRetType = "HRESULT";
					EXPECT(")");
					EXPECT("(");
					NEXTTOKEN;
					if (s == "THIS")
					{
						EXPECT(")");
					}
					else
					if (s == "THIS_")
					{
						do
						{
							string vartype;
							string varname = "-";
							NEXTTOKEN;
							vartype = s;
							NEXTTOKEN;
							while (s != "," && s != ")")
							{
								if (s == "*" || s == "FAR" || s == "&" || vartype == "const")
								{
									vartype = vartype + " " + s;
								}
								else
								{
									varname = s;
								}
								NEXTTOKEN;
							}
							if (varname == "-")
							{
								varname = "a";
								varname[0] += parnameidx;
								parnameidx++;
							}
							if (aPrintProgress) printf("\t\t%s (%s)\n", vartype.c_str(), varname.c_str());
							method->mParmName.push_back(varname);
							method->mParmType.push_back(vartype);
						} 
						while (s == ",");
					}
					else
					{
						PARSEERROR;
					}
					EXPECT("PURE");
					EXPECT(";");
					EXPECT("\n");
					iface->mMethod.push_back(method);
				}
				else
				if (s == "STDMETHOD_")
				{
					EXPECT("(");
					NEXTTOKEN;
					string ret = s;
					EXPECT(",");
					NEXTTOKEN;
					if (aPrintProgress) printf("\tMethod %s, returns %s\n", s.c_str(), ret.c_str());
					method = new Method;
					method->mFuncName = s;
					method->mRetType = ret;
					EXPECT(")");
					EXPECT("(");
					NEXTTOKEN;
					if (s == "THIS")
					{
						EXPECT(")");
					}
					else
					if (s == "THIS_")
					{
						do
						{
							string vartype;
							string varname = "-";
							NEXTTOKEN;
							vartype = s;
							NEXTTOKEN;
							while (s != "," && s != ")")
							{
								if (s == "*" || s == "FAR" || s == "&")
								{
									vartype = vartype + " " + s;
								}
								else
								{
									varname = s;
								}
								NEXTTOKEN;
							}
							if (varname == "-")
							{
								varname = "a";
								varname[0] += parnameidx;
								parnameidx++;
							}
							if (aPrintProgress) printf("\t\t%s (%s)\n", vartype.c_str(), varname.c_str());
							method->mParmName.push_back(varname);
							method->mParmType.push_back(vartype);
						} 
						while (s == ",");
					}
					else
					{
						PARSEERROR;
					}
					EXPECT("PURE");
					EXPECT(";");
					EXPECT("\n");
					iface->mMethod.push_back(method);
				}
				else
				if (s == "}")
				{
					parsestate = 0;
					if (aPrintProgress) printf("End interface\n");
					gIface.push_back(iface);
				}
				else
				if (s == "\n")
				{
				}
				else
				{
					PARSEERROR;
				}
			}
		}
	}
}

void banner(FILE * f)
{
	fprintf(f, 
"// Generated with:\n"
"// " VERSION "\n"
"//\n"
"// If you wish to use the generator, don't do manual changes to this file\n"
"// This is your first and only warning.\n"
"//\n"
"// This software is provided 'as-is', without any express or implied\n"
"// warranty. In no event will the authors be held liable for any damages\n"
"// arising from the use of this software.\n"
"//\n"
"// Permission is granted to anyone to use this software for any purpose,\n"
"// including commercial applications, and to alter it and redistribute it\n"
"// freely, subject to the following restrictions:\n"
"// \n"
"// 1. The origin of this software must not be misrepresented; you must not\n"
"// claim that you wrote the original software. If you use this software\n"
"// in a product, an acknowledgment in the product documentation would be\n"
"// appreciated but is not required.\n"
"//\n"
"// 2. Altered source versions must be plainly marked as such, and must not be\n"
"// misrepresented as being the original software.\n"
"//\n"
"// 3. This notice may not be removed or altered from any source\n"
"// distribution.\n\n");
}

void printTemplate(FILE * f, char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	char *s[10];
	int i = 0;
	do
	{
		s[i] = va_arg(ap, char*);
		i++;
	}
	while (s[i-1]);

	while (*fmt)
	{
		if (*fmt == '@')
		{
			fmt++;
			int idx = *fmt - '0';
			fprintf(f, "%s", s[idx]);
		}
		else
		{
			fputc(*fmt, f);
		}
		fmt++;
	}
	va_end(ap);
}

void printH(int aIfaceNo)
{
	Iface * iface = gIface[aIfaceNo];
	FILE * f;
	char temp[256];
	sprintf(temp, "../wrapper/my%s.h", iface->mName.c_str());
	f = fopen(temp, "w");

	banner(f);
	fprintf(f, "#pragma once\n");
	fprintf(f, "\n");
	fprintf(f, "class my%s : public %s\n", iface->mName.c_str(), iface->mName.c_str());
	fprintf(f, "{\n"
		"public:\n");
#ifdef DISABLE_PASSTHROUGH
	fprintf(f, "  my%s();\n", iface->mName.c_str(), iface->mName.c_str());
#else
	fprintf(f, "  my%s(%s * aOriginal);\n", iface->mName.c_str(), iface->mName.c_str());
#endif
	fprintf(f, "  ~my%s();\n", iface->mName.c_str());
	fprintf(f, "\n");
	int i;
	for (i = 0; i < (signed)iface->mMethod.size(); i++)
	{
		fprintf(f, "  %s __stdcall %s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
		int j;
		for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
			fprintf(f, "%s%s %s",j?", ":"", iface->mMethod[i]->mParmType[j].c_str(), iface->mMethod[i]->mParmName[j].c_str());
		fprintf(f, ");\n");
	}
#ifdef DISABLE_PASSTHROUGH
	fprintf(f, "\n"
		       " int mRef;\n");
#else
	fprintf(f, "\n"
		   "  %s * mOriginal;\n", iface->mName.c_str());
#endif
	fprintf(f, "};\n\n");
	fclose(f);
}

void printIfacePtrHandler(FILE * f, const char * aDataType, const char * aVariable, const char * aWrapper, const char * aPtrCaster = 0)
{
	if (aPtrCaster)
	{
#ifdef DISABLE_PASSTHROUGH
		printTemplate(f,
					"  @0 n = (@0)new @2();\n"
#ifndef DISABLE_LOGGING
					"  logf(\"Wrapped.\\n\");\n"
#endif
					"  *@1 = n;\n", 
					/* 0 */ aDataType,
					/* 1 */ aVariable,
					/* 2 */ aWrapper,
					/* 3 */ aPtrCaster,
					0);
#else
		printTemplate(f,
					"  @0 n = (@0)wrapfetch(*@1);\n"
					"  if (n == NULL && *@1 != NULL)\n"
					"  {\n"
					"    n = (@0)new @2((@3)*@1);\n"
					"    wrapstore(n, *@1);\n"
#ifndef DISABLE_LOGGING
					"    logf(\"Wrapped.\\n\");\n"
#endif
					"  }\n"
					"  *@1 = n;\n", 
					/* 0 */ aDataType,
					/* 1 */ aVariable,
					/* 2 */ aWrapper,
					/* 3 */ aPtrCaster,
					0);
#endif
	}
	else
	{
#ifdef DISABLE_PASSTHROUGH
		printTemplate(f,
					"  @0 n = (@0)new @2();\n"
#ifndef DISABLE_LOGGING
					"  logf(\"Wrapped.\\n\");\n"
#endif
					"  *@1 = n;\n", 
					/* 0 */ aDataType,
					/* 1 */ aVariable,
					/* 2 */ aWrapper,
					0);
#else
		printTemplate(f,
					"  @0 n = (@0)wrapfetch(*@1);\n"
					"  if (n == NULL && *@1 != NULL)\n"
					"  {\n"
					"    n = (@0)new @2(*@1);\n"
					"    wrapstore(n, *@1);\n"
#ifndef DISABLE_LOGGING
					"    logf(\"Wrapped.\\n\");\n"
#endif
					"  }\n"
					"  *@1 = n;\n", 
					/* 0 */ aDataType,
					/* 1 */ aVariable,
					/* 2 */ aWrapper,
					0);
#endif
	}
}



void printCpp(int aIfaceNo)
{
	Iface * iface = gIface[aIfaceNo];
	FILE * f;
	char temp[256];
	sprintf(temp, "../wrapper/my%s.cpp", iface->mName.c_str());
	f = fopen(temp, "w");

	banner(f);

	fprintf(f, "#include \"wrapper.h\"\n");	
	fprintf(f, "#include \"my%s.h\"\n\n", iface->mName.c_str());

#ifdef DISABLE_PASSTHROUGH
	fprintf(f, "my%s::my%s()\n", iface->mName.c_str(), iface->mName.c_str());
#else
	fprintf(f, "my%s::my%s(%s * aOriginal)\n", iface->mName.c_str(), iface->mName.c_str(), iface->mName.c_str());
#endif
	fprintf(f, "{\n");
#ifndef DISABLE_LOGGING
	fprintf(f, "  logf(\"my%s ctor\\n\");\n", iface->mName.c_str());
#endif
#ifdef DISABLE_PASSTHROUGH
	fprintf(f, "  mRef = 1;\n");
#else
	fprintf(f, "  mOriginal = aOriginal;\n");
#endif
    fprintf(f,"}\n"
	     	   "\n");

	fprintf(f, "my%s::~my%s()\n", iface->mName.c_str(), iface->mName.c_str());
	fprintf(f, "{\n");
#ifndef DISABLE_LOGGING
	fprintf(f, "  logf(\"my%s dtor\\n\");\n", iface->mName.c_str());
#endif
	fprintf(f, "}\n"
		   "\n");

	int i;
	for (i = 0; i < (signed)iface->mMethod.size(); i++)
	{
		fprintf(f, "%s __stdcall my%s::%s(", iface->mMethod[i]->mRetType.c_str(), iface->mName.c_str(), iface->mMethod[i]->mFuncName.c_str());
		int j;
		for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
			fprintf(f, "%s%s %s",j?", ":"", iface->mMethod[i]->mParmType[j].c_str(), iface->mMethod[i]->mParmName[j].c_str());
		fprintf(f, ")\n"
			"{\n");

#ifndef DISABLE_CRITICAL_SECTION
		fprintf(f, "  EnterCriticalSection(&gCS);\n");
#endif

#ifndef DISABLE_LOGGING
		fprintf(f, "  logf(\"my%s::%s(", iface->mName.c_str(), iface->mMethod[i]->mFuncName.c_str());
		for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
		{
			fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmType[j].c_str());
			if (iface->mMethod[i]->mParmType[j] == "DWORD") fprintf(f, " %%d");
			else
			if (iface->mMethod[i]->mParmType[j] == "ULONG") fprintf(f, " %%d");
			else
			if (iface->mMethod[i]->mParmType[j] == "HWND") fprintf(f, " 0x%%x");			
			else
			if (iface->mMethod[i]->mParmType[j][0] == 'L' && iface->mMethod[i]->mParmType[j][1] == 'P') fprintf(f, " 0x%%x");			
		}
		fprintf(f, ");\"");
		for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
		{
			if ((iface->mMethod[i]->mParmType[j] == "DWORD") ||
			   (iface->mMethod[i]->mParmType[j] == "ULONG") ||
			   (iface->mMethod[i]->mParmType[j] == "HWND") ||
			   (iface->mMethod[i]->mParmType[j][0] == 'L' && iface->mMethod[i]->mParmType[j][1] == 'P'))
			 fprintf(f, ", %s", iface->mMethod[i]->mParmName[j].c_str());
		}
		fprintf(f, ");\n");
#endif

		char * d3dtype = "$Undefined$"; 
		char * d3dwrapper = "$Undefined$"; 
		char * ddsurfacetype = "$Undefined$";
		char * ddsurfacewrapper = "$Undefined$"; 
		char * ddtype = "$Undefined$"; 
		char * ddwrapper = "$Undefined$"; 
		char * d3ddevicetype = "$Undefined$"; 
		char * d3ddevicewrapper = "$Undefined$"; 
		char * d3dmaterialtype = "$Undefined$"; 
		char * d3dmaterialwrapper = "$Undefined$"; 
		char * d3dtexturetype = "$Undefined$"; 
		char * d3dtexturewrapper = "$Undefined$"; 
		char * d3dvertexbuffertype = "$Undefined$";
		char * d3dvertexbufferwrapper = "$Undefined$";
		char * d3dviewporttype = "$Undefined$";
		char * d3dviewportwrapper = "$Undefined$";

		if (iface->mName == "IDirect3D") {  d3dmaterialwrapper = "myIDirect3DMaterial";  d3dmaterialtype = "myIDirect3DMaterial *";  d3dviewportwrapper = "myIDirect3DViewport";  d3dviewporttype = "myIDirect3DViewport *";  d3dvertexbufferwrapper = "myIDirect3DVertexBuffer";  d3dvertexbuffertype = "myIDirect3DVertexBuffer *"; }
		if (iface->mName == "IDirect3D2") { d3dmaterialwrapper = "myIDirect3DMaterial2"; d3dmaterialtype = "myIDirect3DMaterial2 *"; d3dviewportwrapper = "myIDirect3DViewport2"; d3dviewporttype = "myIDirect3DViewport2 *"; d3dvertexbufferwrapper = "myIDirect3DVertexBuffer";  d3dvertexbuffertype = "myIDirect3DVertexBuffer *";  ddsurfacewrapper = "myIDirectDrawSurface";  d3ddevicewrapper = "myIDirect3DDevice2"; d3ddevicetype = "myIDirect3DDevice2 *"; }
		if (iface->mName == "IDirect3D3") { d3dmaterialwrapper = "myIDirect3DMaterial3"; d3dmaterialtype = "myIDirect3DMaterial3 *"; d3dviewportwrapper = "myIDirect3DViewport3"; d3dviewporttype = "myIDirect3DViewport3 *"; d3dvertexbufferwrapper = "myIDirect3DVertexBuffer";  d3dvertexbuffertype = "myIDirect3DVertexBuffer *";  ddsurfacewrapper = "myIDirectDrawSurface4"; d3ddevicewrapper = "myIDirect3DDevice3"; d3ddevicetype = "myIDirect3DDevice3 *"; }
		if (iface->mName == "IDirect3D7") { d3dvertexbufferwrapper = "myIDirect3DVertexBuffer7"; d3dvertexbuffertype = "myIDirect3DVertexBuffer7 *"; ddsurfacewrapper = "myIDirectDrawSurface7"; d3ddevicewrapper = "myIDirect3DDevice7"; d3ddevicetype = "myIDirect3DDevice7 *"; }
		if (iface->mName == "IDirect3DDevice") {  ddsurfacewrapper = "myIDirectDrawSurface";  d3dtexturewrapper = "myIDirect3DTexture";    d3dtype = "LPDIRECT3D";  d3dwrapper = "myIDirect3D";   ddsurfacetype = "LPDIRECTDRAWSURFACE";  d3dviewportwrapper = "myIDirect3DViewport";  d3dviewporttype = "myIDirect3DViewport *"; }
		if (iface->mName == "IDirect3DDevice2") { ddsurfacewrapper = "myIDirectDrawSurface";  d3dtexturewrapper = "myIDirect3DTexture";    d3dtype = "LPDIRECT3D2"; d3dwrapper = "myIDirect3D2";  ddsurfacetype = "LPDIRECTDRAWSURFACE";  d3dviewportwrapper = "myIDirect3DViewport2"; d3dviewporttype = "myIDirect3DViewport2 *"; }
		if (iface->mName == "IDirect3DDevice3") { ddsurfacewrapper = "myIDirectDrawSurface4"; d3dtexturewrapper = "myIDirect3DTexture2";   d3dvertexbufferwrapper = "myIDirect3DVertexBuffer";  d3dtype = "LPDIRECT3D3"; d3dwrapper = "myIDirect3D3";  ddsurfacetype = "LPDIRECTDRAWSURFACE4"; d3dviewportwrapper = "myIDirect3DViewport3"; d3dviewporttype = "myIDirect3DViewport3 *"; d3dtexturetype = "LPDIRECT3DTEXTURE2";   }
		if (iface->mName == "IDirect3DDevice7") { ddsurfacewrapper = "myIDirectDrawSurface7"; d3dtexturewrapper = "myIDirectDrawSurface7"; d3dvertexbufferwrapper = "myIDirect3DVertexBuffer7"; d3dtype = "LPDIRECT3D7"; d3dwrapper = "myIDirect3D7";  ddsurfacetype = "LPDIRECTDRAWSURFACE7"; d3dviewportwrapper = "myIDirect3DViewport3"; d3dviewporttype = "myIDirect3DViewport3 *"; d3dtexturetype = "LPDIRECTDRAWSURFACE7"; }
		if (iface->mName == "IDirect3DMaterial")  d3ddevicewrapper = "myIDirect3DDevice";
		if (iface->mName == "IDirect3DMaterial2") d3ddevicewrapper = "myIDirect3DDevice2";
		if (iface->mName == "IDirect3DMaterial3") d3ddevicewrapper = "myIDirect3DDevice3";
		if (iface->mName == "IDirect3DTexture") {  d3ddevicewrapper = "myIDirect3DDevice";  d3dtexturewrapper = "myIDirect3DTexture"; }
		if (iface->mName == "IDirect3DTexture2") { d3ddevicewrapper = "myIDirect3DDevice2"; d3dtexturewrapper = "myIDirect3DTexture2"; }
		if (iface->mName == "IDirect3DVertexBuffer") {  d3dvertexbufferwrapper = "myIDirect3DVertexBuffer";  d3ddevicewrapper = "myIDirect3DDevice3"; }
		if (iface->mName == "IDirect3DVertexBuffer7") { d3dvertexbufferwrapper = "myIDirect3DVertexBuffer7"; d3ddevicewrapper = "myIDirect3DDevice7"; }
		if (iface->mName == "IDirectDraw")  { ddsurfacewrapper = "myIDirectDrawSurface";  ddsurfacetype = "IDirectDrawSurface *"; }
		if (iface->mName == "IDirectDraw2") { ddsurfacewrapper = "myIDirectDrawSurface";  ddsurfacetype = "IDirectDrawSurface *"; }
		if (iface->mName == "IDirectDraw3") { ddsurfacewrapper = "myIDirectDrawSurface";  ddsurfacetype = "IDirectDrawSurface *"; }
		if (iface->mName == "IDirectDraw4") { ddsurfacewrapper = "myIDirectDrawSurface4"; ddsurfacetype = "IDirectDrawSurface4 *"; }
		if (iface->mName == "IDirectDraw7") { ddsurfacewrapper = "myIDirectDrawSurface7"; ddsurfacetype = "IDirectDrawSurface7 *"; }
		if (iface->mName == "IDirectDrawSurface")  { ddsurfacewrapper = "myIDirectDrawSurface";  ddsurfacetype = "myIDirectDrawSurface *"; }
		if (iface->mName == "IDirectDrawSurface2") { ddsurfacewrapper = "myIDirectDrawSurface2"; ddsurfacetype = "myIDirectDrawSurface2 *"; ddwrapper = "myIDirectDraw2"; ddtype = "IDirectDraw2 *";}
		if (iface->mName == "IDirectDrawSurface3") { ddsurfacewrapper = "myIDirectDrawSurface3"; ddsurfacetype = "myIDirectDrawSurface3 *"; ddwrapper = "myIDirectDraw3"; ddtype = "IDirectDraw3 *"; }
		if (iface->mName == "IDirectDrawSurface4") { ddsurfacewrapper = "myIDirectDrawSurface4"; ddsurfacetype = "myIDirectDrawSurface4 *"; ddwrapper = "myIDirectDraw4"; ddtype = "IDirectDraw4 *"; }
		if (iface->mName == "IDirectDrawSurface7") { ddsurfacewrapper = "myIDirectDrawSurface7"; ddsurfacetype = "myIDirectDrawSurface7 *"; ddwrapper = "myIDirectDraw7"; ddtype = "IDirectDraw7 *"; }
		if (iface->mName == "IDirect3DViewport3") { ddsurfacewrapper = "myIDirectDrawSurface4"; ddsurfacetype = "myIDirectDrawSurface4 *"; }
#ifdef DISABLE_PASSTHROUGH
		fprintf(f, "  %s x = 0;\n", iface->mMethod[i]->mRetType.c_str());
#else
		fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
		for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
		{
			int k;
			int found = 0;
			for (k = 0; !found && k < (signed)gIface.size(); k++)
			{
				if (gIface[k]->mLPName == iface->mMethod[i]->mParmType[j])
				{
					found = 1;
					printTemplate(f, "@0(@1)?((@2 *)@1)->mOriginal:0", 
						/* 0 */ j ? ", " : "", 
						/* 1 */ iface->mMethod[i]->mParmName[j].c_str(), 
						/* 2 */ gIface[k]->mWrapperName.c_str(), 
						        0);
				}
			}
			if (!found)
			{
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			}
		}
		fprintf(f, ");\n");
#endif
#ifndef DISABLE_LOGGING
		fprintf(f,	"  logfc(\" -> return %%d\\n\", x);\n"
		            "  pushtab();\n");
#endif
					

		if (iface->mMethod[i]->mFuncName == "Release")
		{
#ifdef DISABLE_PASSTHROUGH
			fprintf(f, "  x = --mRef;\n"
				       "  if (mRef <= 0)\n"
				       "  {\n"
					   "    delete this;\n"
					   "  }\n");
#else
			fprintf(f, "  if (x == 0)\n"
				       "  {\n"
					   "    wrapstore(mOriginal, 0);\n"
					   "    mOriginal = NULL;\n"
					   "    delete this;\n"
					   "  }\n");
#endif
		}
		else
#ifdef DISABLE_PASSTHROUGH
		if (iface->mMethod[i]->mFuncName == "AddRef")
		{
			fprintf(f, "  x = ++mRef;\n");
		}
		else
#endif
		if (iface->mMethod[i]->mFuncName == "QueryInterface")
		{
            fprintf(f, "  if (x == 0) genericQueryInterface(%s, %s);\n", iface->mMethod[i]->mParmName[0].c_str(), iface->mMethod[i]->mParmName[1].c_str());
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateClipper")
		{
			printIfacePtrHandler(f, "myIDirectDrawClipper *", "b", "myIDirectDrawClipper");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreatePalette")
		{
			printIfacePtrHandler(f, "myIDirectDrawPalette *", "c", "myIDirectDrawPalette");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateSurface" ||
		    iface->mMethod[i]->mFuncName == "DuplicateSurface" || 
			iface->mMethod[i]->mFuncName == "GetSurfaceFromDC" ||
		    iface->mMethod[i]->mFuncName == "GetAttachedSurface")
		{		
			printIfacePtrHandler(f, ddsurfacetype, iface->mMethod[i]->mParmName[1].c_str(), ddsurfacewrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetGDISurface" ||
		    iface->mMethod[i]->mFuncName == "GetRenderTarget" ||
			iface->mMethod[i]->mFuncName == "GetBackgroundDepth2")
		{
			printIfacePtrHandler(f, ddsurfacetype, iface->mMethod[i]->mParmName[0].c_str(), ddsurfacewrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetBackgroundDepth")
		{
			printIfacePtrHandler(f, "myIDirectDrawSurface *", "a", "myIDirectDrawSurface");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateDevice")
		{
			printIfacePtrHandler(f, d3ddevicetype, "c", d3ddevicewrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateDirectDraw")
		{
			// May require more thought, since the dd interface returned may be something else than just plain dd1.
			printIfacePtrHandler(f, "myIDirectDraw *", "ppDirectDraw", "myIDirectDraw");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateVertexBuffer")
		{
			printIfacePtrHandler(f, d3dvertexbuffertype, "b", d3dvertexbufferwrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateLight")
		{
			printIfacePtrHandler(f, "myIDirect3DLight *", "a", "myIDirect3DLight");
		}
		else
		if (iface->mMethod[i]->mFuncName == "NextLight")
		{
			printIfacePtrHandler(f, "myIDirect3DLight *", "b", "myIDirect3DLight");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateExecuteBuffer")
		{
			printIfacePtrHandler(f, "myIDirect3DExecuteBuffer *", "b", "myIDirect3DExecuteBuffer");
		}
		else			
		if (iface->mMethod[i]->mFuncName == "CreateMaterial")
		{
			printIfacePtrHandler(f, d3dmaterialtype, "a", d3dmaterialwrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateViewport" ||
			iface->mMethod[i]->mFuncName == "GetCurrentViewport")
		{
			printIfacePtrHandler(f, d3dviewporttype, "a", d3dviewportwrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "NextViewport")
		{
			printIfacePtrHandler(f, d3dviewporttype, "b", d3dviewportwrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetPalette")
		{
			printIfacePtrHandler(f, "myIDirectDrawPalette *", iface->mMethod[i]->mParmName[0].c_str(), "myIDirectDrawPalette");
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetClipper")
		{
			printIfacePtrHandler(f, "myIDirectDrawClipper *", iface->mMethod[i]->mParmName[0].c_str(), "myIDirectDrawClipper");
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetDDInterface")
		{
			printIfacePtrHandler(f, "LPVOID", iface->mMethod[i]->mParmName[0].c_str(), ddwrapper, ddtype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetDirect3D")
		{
			printIfacePtrHandler(f, d3dtype, iface->mMethod[i]->mParmName[0].c_str(), d3dwrapper);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetTexture")
		{
			printIfacePtrHandler(f, d3dtexturetype, iface->mMethod[i]->mParmName[1].c_str(), d3dtexturewrapper);
		}
		else
		if (0)
		{
			// If there are some specifically non-implemented methods
#ifndef DISABLE_LOGGING
			fprintf(f, "  logf(\"\\n**** NOT IMPLEMENTED\\n\");\n");
#endif
			fprintf(f, "  return E_NOTIMPL;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "EvaluateMode" ||
			iface->mMethod[i]->mFuncName == "Lock")
		{
			// Just to skip the warnings generated below..
		}
		else
		{
			// generic function, doesn't do anything strange. 
			// Let's do some heuristics to see if this function should get special handing..
			for (j = 0; j < (signed)iface->mMethod[i]->mParmType.size(); j++)
			{
				if (iface->mMethod[i]->mParmType[j].find('*') != std::string::npos &&
					iface->mMethod[i]->mParmType[j].find("HDC") == std::string::npos &&
					iface->mMethod[i]->mParmType[j].find("GUID") == std::string::npos &&
					iface->mMethod[i]->mParmType[j].find("HWND") == std::string::npos &&
					iface->mMethod[i]->mParmType[j].find("D3DVALUE") == std::string::npos &&
					iface->mMethod[i]->mParmType[j].find("BOOL") == std::string::npos)
				{
					printf("\n%s::%s may need special care due to %s", iface->mName.c_str(), iface->mMethod[i]->mFuncName.c_str(), iface->mMethod[i]->mParmType[j].c_str());
				}
			}
		}
		
		fprintf(f, 
#ifndef DISABLE_LOGGING
				   "  poptab();\n" 
#endif
#ifndef DISABLE_CRITICAL_SECTION
				   "  LeaveCriticalSection(&gCS);\n"
#endif
				   "  return x;\n"
		           "}\n"
			       "\n");
	}
	fclose(f);
}

void main(void)
{
	parse("E:\\dx1dx7\\dx8sdk\\include\\ddraw.h");
	parse("E:\\dx1dx7\\dx8sdk\\include\\ddrawex.h");
	parse("E:\\dx1dx7\\dx8sdk\\include\\d3d.h");
	printf("Generating");
	int i;
	for (i = 0; i < (signed)gIface.size(); i++)
	{
		printf(".");
		printH(i);
		printCpp(i);
	}
	printf("\nDone.\n");
}