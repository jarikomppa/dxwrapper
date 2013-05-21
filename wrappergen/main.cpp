#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include <stdarg.h>
#include <vector>
#include <string>

#define VERSION "ddwrappergen 0.130521 (c)2013 Jari Komppa http://iki.fi/sol/"

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
	fprintf(f, "  my%s(%s * aOriginal);\n", iface->mName.c_str(), iface->mName.c_str());
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
	fprintf(f, "\n"
		   "  %s * mOriginal;\n", iface->mName.c_str());
	fprintf(f, "};\n\n");
	fclose(f);
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

	fprintf(f, "my%s::my%s(%s * aOriginal)\n", iface->mName.c_str(), iface->mName.c_str(), iface->mName.c_str());
	fprintf(f, "{\n");
	fprintf(f, "  logf(\"my%s ctor\\n\");\n", iface->mName.c_str());
	fprintf(f, "  mOriginal = aOriginal;\n"
		   "}\n"
		   "\n");

	fprintf(f, "my%s::~my%s()\n", iface->mName.c_str(), iface->mName.c_str());
	fprintf(f, "{\n");
	fprintf(f, "  logf(\"my%s dtor\\n\");\n", iface->mName.c_str());
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

		fprintf(f, "  EnterCriticalSection(&gCS);\n");

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

		fprintf(f, "  pushtab();\n");

#define PROLOGUE \
		"  poptab();\n" \
		"  LeaveCriticalSection(&gCS);\n"


		if (iface->mMethod[i]->mFuncName == "Release")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == 0)\n"
				       "  {\n"
					   "    mOriginal = NULL;\n"
//					   "    logf(\"Ref count zero, calling dtor\\n\");\n"
					   "    delete this;\n"
					   "  }\n"
					   PROLOGUE
			           "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "QueryInterface")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n");
			fprintf(f, "  logfc(\" -> return %%d\\n\", x);\n");
            fprintf(f, "  if (x == 0) genericQueryInterface(%s, %s);\n", iface->mMethod[i]->mParmName[0].c_str(), iface->mMethod[i]->mParmName[1].c_str());
			fprintf(f, PROLOGUE
			           "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateClipper")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    myIDirectDrawClipper *nc = new myIDirectDrawClipper(*b);\n"
					   "    wrapstore(*b, nc);\n"
			           "    *b = nc;\n"
					   "    logf(\"Wrapped clipper.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreatePalette")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    myIDirectDrawPalette *np = new myIDirectDrawPalette(*c);\n"
					   "    wrapstore(*c, np);\n"
			           "    *c = np;\n"
					   "    logf(\"Wrapped palette.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateSurface")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDraw7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDraw4") surftype = "myIDirectDrawSurface4";

			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    %s *ns = new %s(*b);\n"
					   "    wrapstore(*b, ns);\n"
			           "    *b = ns;\n"
					   "    logf(\"Wrapped surface.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n", surftype, surftype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "DuplicateSurface")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDraw7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDraw4") surftype = "myIDirectDrawSurface4";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[0].c_str(), surftype, iface->mMethod[i]->mParmName[0].c_str());
			int j;
			for (j = 1; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    %s *ns = new %s(*b);\n"
					   "    wrapstore(*b, ns);\n"
			           "    *b = ns;\n"
					   "    logf(\"Wrapped surface.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n", surftype, surftype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetSurfaceFromDC")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDraw7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDraw4") surftype = "myIDirectDrawSurface4";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    %s *ns = new %s(*b);\n"
					   "    wrapstore(*b, ns);\n"
			           "    *b = ns;\n"
					   "    logf(\"Wrapped surface.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n", surftype, surftype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "AddAttachedSurface")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDrawSurface4") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface3") surftype = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface2") surftype = "myIDirectDrawSurface2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "Blt")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDrawSurface4") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface3") surftype = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface2") surftype = "myIDirectDrawSurface2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 1)
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "BltFast")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDrawSurface4") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface3") surftype = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface2") surftype = "myIDirectDrawSurface2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 2)
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "DeleteAttachedSurface")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDrawSurface4") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface3") surftype = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface2") surftype = "myIDirectDrawSurface2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 1)
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "Flip")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDrawSurface4") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface3") surftype = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface2") surftype = "myIDirectDrawSurface2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "UpdateOverlay")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDrawSurface4") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface3") surftype = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface2") surftype = "myIDirectDrawSurface2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 1)
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "UpdateOverlayZOrder")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface7") surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirectDrawSurface4") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface3") surftype = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface2") surftype = "myIDirectDrawSurface2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 1)
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "SetTexture")
		{
			char * surftype = "myIDirectDrawSurface7";
			if (iface->mName == "IDirect3DDevice3") surftype = "myIDirect3DTexture2";

			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 1)
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "Load")
		{
			if (iface->mName == "IDirect3DDevice7")
			{
				char * surftype = "myIDirectDrawSurface7";

				fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
				int j;
				for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
					if (j == 0)
						fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
					else
					if (j == 2)
						fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
					else
						fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
				fprintf(f, ");\n"
						   "  logfc(\" -> return %%d\\n\", x);\n"
						   PROLOGUE
					       "  return x;\n");
			}
			else
			{
				char * surftype = "myIDirect3DTexture";
				if (iface->mName == "IDirect3DTexture2") surftype = "myIDirect3DTexture2";

				fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
				int j;
				for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
					if (j == 0)
						fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
					else
						fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
				fprintf(f, ");\n"
						   "  logfc(\" -> return %%d\\n\", x);\n"
						   PROLOGUE
						   "  return x;\n");
			}
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateDevice")
		{
			char * surftype = "myIDirectDrawSurface";
			char * devtype = "myIDirect3DDevice2";
			if (iface->mName == "IDirect3D7") { surftype = "myIDirectDrawSurface7"; devtype = "myIDirect3DDevice7"; }
			if (iface->mName == "IDirect3D3") { surftype = "myIDirectDrawSurface4"; devtype = "myIDirect3DDevice3"; }
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 1)
				{
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				}
				else
				{
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
				}
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    %s *ns = new %s(*c);\n"
					   "    wrapstore(*c, ns);\n"
			           "    *c = ns;\n"
					   "    logf(\"Wrapped device.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n", devtype, devtype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "ProcessVertices")
		{
			char * buftype = "myIDirect3DVertexBuffer";
			char * devtype = "myIDirect3DDevice3";
			if (iface->mName == "IDirect3DVertexBuffer7") { buftype = "myIDirect3DVertexBuffer7"; devtype = "myIDirect3DDevice7"; }
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 3)
				{
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), buftype, iface->mMethod[i]->mParmName[j].c_str());
				}
				else
				if (j == 5)
				{
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), devtype, iface->mMethod[i]->mParmName[j].c_str());
				}
				else
				{
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
				}
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
			           "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "ProcessVerticesStrided")
		{
			char * devtype = "myIDirect3DDevice7";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 5)
				{
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), devtype, iface->mMethod[i]->mParmName[j].c_str());
				}
				else
				{
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
				}
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
			           "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "SetClipper")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((myIDirectDrawClipper *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "SetPalette")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((myIDirectDrawPalette *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "Initialize" && 
			(iface->mName == "IDirectDrawSurface" ||
			 iface->mName == "IDirectDrawSurface2" ||
			 iface->mName == "IDirectDrawSurface3" ||
			 iface->mName == "IDirectDrawSurface4" ||
			 iface->mName == "IDirectDrawSurface7"))
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((myIDirectDraw *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateVertexBuffer")
		{
			char * buftype = "myIDirect3DVertexBuffer";
			if (iface->mName == "IDirect3D7") buftype = "myIDirect3DVertexBuffer7";

			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    %s *ns = new %s(*b);\n"
					   "    wrapstore(*b, ns);\n"
			           "    *b = ns;\n"
					   "    logf(\"Wrapped vertex buffer.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n", buftype, buftype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateLight")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    myIDirect3DLight *ns = new myIDirect3DLight(*a);\n"
					   "    wrapstore(*a, ns);\n"
			           "    *a = ns;\n"
					   "    logf(\"Wrapped light.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateMaterial")
		{
			char * mattype = "myIDirect3DMaterial";
			if (iface->mName == "IDirect3D2") mattype = "myIDirect3DMaterial2";
			if (iface->mName == "IDirect3D3") mattype = "myIDirect3DMaterial3";

			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    %s *ns = new %s(*a);\n"
					   "    wrapstore(*a, ns);\n"
			           "    *a = ns;\n"
					   "    logf(\"Wrapped material.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n", mattype, mattype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "CreateViewport")
		{
			char * mattype = "myIDirect3DViewport";
			if (iface->mName == "IDirect3D2") mattype = "myIDirect3DViewport2";
			if (iface->mName == "IDirect3D3") mattype = "myIDirect3DViewport3";

			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
			           "  if (x == DD_OK)\n"
			           "  {\n"
			           "    %s *ns = new %s(*a);\n"
					   "    wrapstore(*a, ns);\n"
			           "    *a = ns;\n"
					   "    logf(\"Wrapped viewport.\\n\");\n"
			           "  }\n"
					   PROLOGUE
			           "  return x;\n", mattype, mattype);
		}
		else
		if (iface->mMethod[i]->mFuncName == "SetRenderTarget")
		{
			char * surftype = "myIDirectDrawSurface";
			if (iface->mName == "IDirect3DDevice3") surftype = "myIDirectDrawSurface4";
			if (iface->mName == "IDirect3DDevice7") surftype = "myIDirectDrawSurface7";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetHandle")
		{
			char * surftype = "myIDirect3DDevice";
			if (iface->mName == "IDirect3DMaterial2") surftype = "myIDirect3DDevice2";
			if (iface->mName == "IDirect3DMaterial3") surftype = "myIDirect3DDevice3";
			if (iface->mName == "IDirect3DTexture2") surftype = "myIDirect3DDevice2";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), surftype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "Preload")
		{
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((myIDirectDrawSurface7 *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "AddViewport" ||
			iface->mMethod[i]->mFuncName == "DeleteViewport")
		{
			char *wraptype = "myIDirect3DViewport";
			if (iface->mName == "IDirect3DDevice2") wraptype = "myIDirect3DViewport2";
			if (iface->mName == "IDirect3DDevice3") wraptype = "myIDirect3DViewport3";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), wraptype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "DrawPrimitiveVB" || iface->mMethod[i]->mFuncName == "DrawIndexedPrimitiveVB")
		{
			char * vbtype = "myIDirect3DVertexBuffer";
			if (iface->mName == "IDirect3DDevice7") vbtype = "myIDirect3DVertexBuffer7";

			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 1)
					fprintf(f, ", (%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), vbtype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "SetCurrentViewport")
		{
			char * vbtype = "myIDirect3DViewport2";
			if (iface->mName == "IDirect3DDevice3") vbtype = "myIDirect3DViewport3";

			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				if (j == 0)
					fprintf(f, "(%s)?((%s *)%s)->mOriginal:0", iface->mMethod[i]->mParmName[j].c_str(), vbtype, iface->mMethod[i]->mParmName[j].c_str());
				else
					fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n"
			           "  logfc(\" -> return %%d\\n\", x);\n"
					   PROLOGUE
					   "  return x;\n");
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetPalette")
		{
			char * datatype = "LPDIRECTDRAWPALETTE";
			char * wrapname = "myIDirectDrawPalette";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			printTemplate(f,
				       ");\n"
			           "  logfc(\" -> return %d\\n\", x);\n"
					   "  @0 n = (@0)wrapfetch(*@1);\n"
					   "  if (n == NULL && *@1 != NULL)\n"
					   "  {\n"
					   "    n = (@0)new @2(*@1);\n"
					   "    logf(\"Wrapped palette\\n\");\n"
					   "  }\n"
					   "  *@1 = n;\n"
					   PROLOGUE
					   "  return x;\n", 
					   /* 0 */ datatype,
					   /* 1 */ iface->mMethod[i]->mParmName[0].c_str(), 
					   /* 2 */ wrapname,					   
					   0);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetClipper")
		{
			char * datatype = "LPDIRECTDRAWCLIPPER";
			char * wrapname = "myIDirectDrawClipper";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			printTemplate(f,
				       ");\n"
			           "  logfc(\" -> return %d\\n\", x);\n"
					   "  @0 n = (@0)wrapfetch(*@1);\n"
					   "  if (n == NULL)\n"
					   "  {\n"
					   "    n = (@0)new @2(*@1);\n"
					   "    logf(\"Wrapped clipper\\n\");\n"
					   "  }\n"
					   "  *@1 = n;\n"
					   PROLOGUE
					   "  return x;\n", 
					   /* 0 */ datatype,
					   /* 1 */ iface->mMethod[i]->mParmName[0].c_str(), 
					   /* 2 */ wrapname,					   
					   0);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetDDInterface")
		{
			char * datatype = "LPVOID";
			char * datatype2 = "IDirectDraw2 *";
			char * wrapname = "myIDirectDraw2";
			if (iface->mName == "IDirectDrawSurface3") { wrapname = "myIDirectDraw3"; datatype2 = "IDirectDraw3 *"; }
			if (iface->mName == "IDirectDrawSurface4") { wrapname = "myIDirectDraw4"; datatype2 = "IDirectDraw4 *"; }
			if (iface->mName == "IDirectDrawSurface7") { wrapname = "myIDirectDraw7"; datatype2 = "IDirectDraw7 *"; }
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			printTemplate(f,
				       ");\n"
			           "  logfc(\" -> return %d\\n\", x);\n"
					   "  @0 n = (@0)wrapfetch(*@1);\n"
					   "  if (n == NULL && *@1 != NULL)\n"
					   "  {\n"
					   "    n = (@0)new @2((@3)*@1);\n"
					   "    logf(\"Wrapped ddraw\\n\");\n"
					   "  }\n"
					   "  *@1 = n;\n"
					   PROLOGUE
					   "  return x;\n", 
					   /* 0 */ datatype,
					   /* 1 */ iface->mMethod[i]->mParmName[0].c_str(), 
					   /* 2 */ wrapname,		
					   /* 3 */ datatype2,
					   0);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetAttachedSurface")
		{		
			char *wrapname ="myIDirectDrawSurface";
			if (iface->mName == "IDirectDrawSurface2") wrapname = "myIDirectDrawSurface2";
			if (iface->mName == "IDirectDrawSurface3") wrapname = "myIDirectDrawSurface3";
			if (iface->mName == "IDirectDrawSurface4") wrapname = "myIDirectDrawSurface4";
			if (iface->mName == "IDirectDrawSurface7") wrapname = "myIDirectDrawSurface7";
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());

			printTemplate(f,
				       ");\n"
			           "  logfc(\" -> return %d\\n\", x);\n"
					   "  @0* n = (@0 *)wrapfetch(*@1);\n"
					   "  if (n == NULL && *@1 != NULL)\n"
					   "  {\n"
					   "    n = (@0 *)new @2(*@1);\n"
					   "    logf(\"Wrapped attached surface\\n\");\n"
					   "  }\n"
					   "  *@1 = n;\n"
					   PROLOGUE
					   "  return x;\n", 
					   /* 0 */ iface->mName.c_str(),
					   /* 1 */ iface->mMethod[i]->mParmName[1].c_str(), 
					   /* 2 */ wrapname,					   
					   0);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetDirect3D")
		{
			char * datatype = "LPDIRECT3D";
			char * wrapname = "myIDirect3D";
			if (iface->mName == "IDirect3DDevice2") { datatype = "LPDIRECT3D2"; wrapname = "myIDirect3D2"; }
			if (iface->mName == "IDirect3DDevice3") { datatype = "LPDIRECT3D3"; wrapname = "myIDirect3D3"; }
			if (iface->mName == "IDirect3DDevice7") { datatype = "LPDIRECT3D7"; wrapname = "myIDirect3D7"; }
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			printTemplate(f,
				       ");\n"
			           "  logfc(\" -> return %d\\n\", x);\n"
					   "  @0 n = (@0)wrapfetch(*@1);\n"
					   "  if (n == NULL && *@1 != NULL)\n"
					   "  {\n"
					   "    n = (@0)new @2(*@1);\n"
					   "    logf(\"Wrapped ddraw\\n\");\n"
					   "  }\n"
					   "  *@1 = n;\n"
					   PROLOGUE
					   "  return x;\n", 
					   /* 0 */ datatype,
					   /* 1 */ iface->mMethod[i]->mParmName[0].c_str(), 
					   /* 2 */ wrapname,					   
					   0);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetRenderTarget")
		{
			char * datatype = "LPDIRECTDRAWSURFACE";
			char * wrapname = "myIDirectDrawSurface";
			if (iface->mName == "IDirect3DDevice3") { datatype = "LPDIRECTDRAWSURFACE4"; wrapname="myIDirectDrawSurface4"; }
			if (iface->mName == "IDirect3DDevice7") { datatype = "LPDIRECTDRAWSURFACE7"; wrapname="myIDirectDrawSurface7"; }
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			printTemplate(f,
				       ");\n"
			           "  logfc(\" -> return %d\\n\", x);\n"
					   "  @0 n = (@0)wrapfetch(*@1);\n"
					   "  if (n == NULL && *@1 != NULL)\n"
					   "  {\n"
					   "    n = (@0)new @2(*@1);\n"
					   "    logf(\"Wrapped surface\\n\");\n"
					   "  }\n"
					   "  *@1 = n;\n"
					   PROLOGUE
					   "  return x;\n", 
					   /* 0 */ datatype,
					   /* 1 */ iface->mMethod[i]->mParmName[0].c_str(), 
					   /* 2 */ wrapname,					   
					   0);
		}
		else
		if (iface->mMethod[i]->mFuncName == "GetTexture")
		{
			char * datatype = "LPDIRECT3DTEXTURE2";
			char * wrapname = "myIDirect3DTexture2";
			if (iface->mName == "IDirect3DDevice7") { datatype = "LPDIRECTDRAWSURFACE7"; wrapname = "myIDirectDrawSurface7"; }
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			printTemplate(f,
				       ");\n"
			           "  logfc(\" -> return %d\\n\", x);\n"
					   "  @0 n = (@0)wrapfetch(*@1);\n"
					   "  if (n == NULL && *@1 != NULL)\n"
					   "  {\n"
					   "    n = (@0)new @2(*@1);\n"
					   "    logf(\"Wrapped texture\\n\");\n"
					   "  }\n"
					   "  *@1 = n;\n"
					   PROLOGUE
					   "  return x;\n", 
					   /* 0 */ datatype,
					   /* 1 */ iface->mMethod[i]->mParmName[1].c_str(), 
					   /* 2 */ wrapname,					   
					   0);
		}
		else
		if (0)
		{
			fprintf(f, "  logf(\"\\n**** NOT IMPLEMENTED\\n\");\n");
			fprintf(f, "  return E_NOTIMPL;\n");
		}
		else
		{
			// generic function
			fprintf(f, "  %s x = mOriginal->%s(", iface->mMethod[i]->mRetType.c_str(), iface->mMethod[i]->mFuncName.c_str());
			int j;
			for (j = 0; j < (signed)iface->mMethod[i]->mParmName.size(); j++)
				fprintf(f, "%s%s", j?", ":"", iface->mMethod[i]->mParmName[j].c_str());
			fprintf(f, ");\n");
			if (iface->mMethod[i]->mRetType == "HRESULT" ||
				iface->mMethod[i]->mRetType == "DWORD" ||
				iface->mMethod[i]->mRetType == "ULONG")
				fprintf(f, "  logfc(\" -> return %%d\\n\", x);\n");
			else
				fprintf(f, "  logf(\"\\n\", x);\n");

			fprintf(f, PROLOGUE
				       "  return x;\n");
		}
		
		fprintf(f, "}\n"
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