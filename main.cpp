/*
    Color Mill Filter for VirtualDub (2.1).
		Based on Red/Green/Blue (RGB) Adjustment Filter for VirtualDub.(1.0) by Donald A. Graft
		Real HUE Adjustment taken from Hue/Saturation/Intensity Filter for VirtualDub (1.2)  by Donald A. Graft
    Copyright (C) 1999-2000 Donald A. Graft
	Copyright (C) 2004 Eugene Khoroshavin

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

	The author can be contacted at:
	Eugene Khoroshavin
	fdump@mail.ru
*/
/*	Ability to compile 64 bit version added.
	Contact:
	Billy Blair
	adviceuneed@hotmail.com
*/

#include <windows.h>
#include <commctrl.h>
#include <stdio.h>
#include <crtdbg.h>
#include <math.h>
#include <stdlib.h>

#include "ScriptInterpreter.h"
#include "ScriptError.h"
#include "ScriptValue.h"

#include "resource.h"
#include "filter.h"

///////////////////////////////////////////////////////////////////////////

int RunProc(const FilterActivation *fa, const FilterFunctions *ff);
int StartProc(FilterActivation *fa, const FilterFunctions *ff);
int EndProc(FilterActivation *fa, const FilterFunctions *ff);
//long ParamProc(FilterActivation *fa, const FilterFunctions *ff);
int InitProc(FilterActivation *fa, const FilterFunctions *ff);
int ConfigProc(FilterActivation *fa, const FilterFunctions *ff, HWND hwnd);
void StringProc(const FilterActivation *fa, const FilterFunctions *ff, char *str);
//void ScriptConfig(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc);
bool FssProc(FilterActivation *fa, const FilterFunctions *ff, char *buf, int buflen);
int normpos( int a, int b, int c, int pos);
void identmat(float mat[4][4]);
void cscalemat(float mat[4][4], float rscale, float gscale, float bscale);
void saturatemat(float mat[4][4], float sat);
void simplehuerotatemat(float mat[4][4], float rot);
void mat2lmat(float mat[4][4], long lmat[4][4]);
void lxformRGB(long mat[4][4], long *r, long *g, long *b);
//void xformRGB(float mat[4][4], long *r, long *g, long *b);
void normrgb(long *r, long *g, long *b);
///////////////////////////////////////////////////////////////////////////
#define M_PI (3.1415926535897932384626433832795) // this is a joke
#define u22 (16384)
#define TTH (10000)

const char * rtxt = "Red";
const char * gtxt = "Green";
const char * btxt = "Blue";
const char * dtxt = "Dark";
const char * mtxt = "Middle";
const char * ltxt = "Light";
const char * htxt = "sHue or Real";
const char * stxt = "sSat or Real";
const char * vtxt = "sValue or Real";
const char * mptxt = "Middle Point";
const char * mstxt = "Booster";
const char * sitxt = "Base Shift";
const char * shprtxt = "Preprocess";
const char * shxztxt = "Dam H & Low";
const char * shpotxt = "Postprocess";
const char * func1txt = "Middle Point";
const char * func2txt = "Similar or real HSV";
const char * func3txt = "Gamma";
const char * func4txt = "Levels";
const char * func5txt = "R-G-B Dark";
const char * func6txt = "R-G-B Middle";
const char * func7txt = "R-G-B Light";
const char * func8txt = "Saturation";
const char * func9txt = "Sharp <-> Smooth";
Pixel32 *nwpic;





typedef struct MyFilterData {
	IFilterPreview		*ifp;

	int					redD;
	int					greenD;
	int					blueD;
	int					redM;
	int					greenM;
	int					blueM;
	int					redL;
	int					greenL;
	int					blueL;
	int					lvD;
	int					lvM;
	int					lvL;
	int					satD;
	int					satM;
	int					satL;
	int					gamR;
	int					gamG;
	int					gamB;
	int					hue;
	int					sat;
	int					val;
	int					mpl;
	int					mps;
	int					shi;
	int					shpr;
	int					shxz;
	int					shpo;
	int					shlow;

	unsigned char		rtbl[256];
	unsigned char		gtbl[256];
	unsigned char		btbl[256];

	int					rgbFlg;
	int					lvFlg;
	int					satFlg;
	int					gamFlg;
	int					hsvFlg;
	int					gPreFlg;
	int					badFlg;
	int					kebrFlg;
	int					mpoFlg;

	int					darlock;
	int					midlock;
	int					liglock;
	int					levlock;
	int					satlock;
	int					gamlock;
	int					hsvlock;
	int					mpolock;
	int					shalock;
	int					levunion;
	int					satunion;
	int					gamunion;
	int					realhsv;
	int					rgbunion;

	int					nmfunc;

	long				cost[201];
	long				sint[201];
	long				partcoef[129];
	long				partcoe3[129];
    float				mat[4][4];
	long				lmat[4][4];
	long				incs[256];
	long				incl[256];
	long				incr[256];
	long				incg[256];
	long				incb[256];
	long				inclmp[256];
} MyFilterData;

////////////////////////////////////////////////////////////////////////////////
void setTab (MyFilterData *mfd);
void setFlg (MyFilterData *mfd);
void InitMatrix(MyFilterData *mfd);
void satur(MyFilterData *mfd, long *r, long *g, long *b);
void rgbdo(MyFilterData *mfd, long *r, long *g, long *b);
void lvels(MyFilterData *mfd, long *r, long *g, long *b);
void hsvdo(MyFilterData *mfd, long *ri, long *gi, long *bi);
void midpoint(MyFilterData *mfd, long *r, long *g, long *b);
void sharp(MyFilterData *mfd, long *r, long *g, long *b, Pixel32 *src, PixDim height, PixDim width, int x, int y, PixOffset pitch, int ncall);
void gamma(MyFilterData *mfd, long *r, long *g, long *b);
///////////////////////////////////////////////////////////////////////////////

bool FssProc(FilterActivation *fa, const FilterFunctions *ff, char *buf, int buflen) {
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	_snprintf_s(buf, buflen, _TRUNCATE, "Config(%d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d, %d)",
												//1	 2	3	4	5	6	7	8	9	a   b   c   d   e   f
		(mfd->redD + 100)	| ((mfd->greenD + 100) << 8),	//1
		(mfd->blueD + 100)	| ((mfd->redM + 100) << 8),		//2
		(mfd->greenM + 100)	| ((mfd->blueM + 100) << 8),	//3
		(mfd->redL + 100)	| ((mfd->greenL + 100) << 8),	//4
		(mfd->blueL +100)	| ((mfd->lvD + 100) << 8),		//5
		(mfd->lvM + 100)	| ((mfd->lvL + 100) << 8),		//6
		(mfd->satD + 100)	| ((mfd->satM + 100) << 8),		//7
		(mfd->satL + 100)	| ((mfd->gamR + 100) << 8),		//8
		(mfd->gamG + 100)	| ((mfd->gamB + 100) << 8),		//9
		(mfd->hue + 100)	| ((mfd->sat + 100) << 8),		//a
		(mfd->val + 100)	| ((mfd->mpl + 100) << 8),		//b
		(mfd->mps + 100)	| ((mfd->shi + 100) << 8),		//c
		(mfd->shpr + 100)	| ((mfd->shxz + 100) << 8),		//d
		(mfd->shpo + 100)	| ((
								(mfd->shalock)			|
								(mfd->shlow	<< 1)		) << 8),	//e 6 bit

		(mfd->kebrFlg)			|
		(mfd->badFlg	<< 1)	|
		(mfd->gPreFlg	<< 2)	|
		(mfd->darlock	<< 3)	|
		(mfd->midlock	<< 4)	|
		(mfd->liglock	<< 5)	|
		(mfd->levlock	<< 6)	|
		(mfd->satlock	<< 7)	|
		(mfd->gamlock	<< 8)	|
		(mfd->hsvlock	<< 9)	|
		(mfd->levunion	<< 10)	|
		(mfd->satunion	<< 11)	|
		(mfd->gamunion	<< 12)	|
		(mfd->realhsv	<< 13)	|
		(mfd->rgbunion	<< 14)	|
		(mfd->mpolock	<< 15)	);							//f

	return true;
}

void ScriptConfig(IScriptInterpreter *isi, void *lpVoid, CScriptValue *argv, int argc) {

	FilterActivation *fa = (FilterActivation *)lpVoid;
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	mfd->redD			= ( argv[0].asInt() & 0xff) - 100;
	mfd->greenD			= ((argv[0].asInt() & 0xff00) >>8) -100;
	mfd->blueD			= ( argv[1].asInt() & 0xff) - 100;
	mfd->redM			= ((argv[1].asInt() & 0xff00) >>8) -100;
	mfd->greenM			= ( argv[2].asInt() & 0xff) - 100;
	mfd->blueM			= ((argv[2].asInt() & 0xff00) >>8) -100;
	mfd->redL			= ( argv[3].asInt() & 0xff) - 100;
	mfd->greenL			= ((argv[3].asInt() & 0xff00) >>8) -100;
	mfd->blueL			= ( argv[4].asInt() & 0xff) - 100;
	mfd->lvD			= ((argv[4].asInt() & 0xff00) >>8) -100;
	mfd->lvM			= ( argv[5].asInt() & 0xff) - 100;
	mfd->lvL			= ((argv[5].asInt() & 0xff00) >>8) -100;
	mfd->satD			= ( argv[6].asInt() & 0xff) - 100;
	mfd->satM			= ((argv[6].asInt() & 0xff00) >>8) -100;
	mfd->satL			= ( argv[7].asInt() & 0xff) - 100;
	mfd->gamR			= ((argv[7].asInt() & 0xff00) >>8) -100;
	mfd->gamG			= ( argv[8].asInt() & 0xff) - 100;
	mfd->gamB			= ((argv[8].asInt() & 0xff00) >>8) -100;
	mfd->hue			= ( argv[9].asInt() & 0xff) - 100;
	mfd->sat			= ((argv[9].asInt() & 0xff00) >>8) -100;
	mfd->val			= ( argv[10].asInt() & 0xff) - 100;
	mfd->mpl			= ((argv[10].asInt() & 0xff00) >>8) -100;
	mfd->mps			= ( argv[11].asInt() & 0xff) - 100;
	mfd->shi			= ((argv[11].asInt() & 0xff00) >>8) -100;
	mfd->shpr			= ( argv[12].asInt() & 0xff) - 100;
	mfd->shxz			= ((argv[12].asInt() & 0xff00) >>8) -100;
	mfd->shpo			= ( argv[13].asInt() & 0xff) - 100;

	int flgs			= ((argv[13].asInt() & 0xff00) >>8);
	mfd->shalock		= ((flgs & 0x0001) > 0);
	mfd->shlow			= ((flgs & 0x007E) >> 1);

	flgs = argv[14].asInt();
	mfd->kebrFlg		= ((flgs & 0x0001) > 0);
	mfd->badFlg			= ((flgs & 0x0002) > 0);
	mfd->gPreFlg		= ((flgs & 0x0004) > 0);
	mfd->darlock		= ((flgs & 0x0008) > 0);
	mfd->midlock		= ((flgs & 0x0010) > 0);
	mfd->liglock		= ((flgs & 0x0020) > 0);
	mfd->levlock		= ((flgs & 0x0040) > 0);
	mfd->satlock		= ((flgs & 0x0080) > 0);
	mfd->gamlock		= ((flgs & 0x0100) > 0);
	mfd->hsvlock		= ((flgs & 0x0200) > 0);
	mfd->levunion		= ((flgs & 0x0400) > 0);
	mfd->satunion		= ((flgs & 0x0800) > 0);
	mfd->gamunion		= ((flgs & 0x1000) > 0);
	mfd->realhsv		= ((flgs & 0x2000) > 0);
	mfd->rgbunion		= ((flgs & 0x4000) > 0);
	mfd->mpolock		= ((flgs & 0x8000) > 0);

	setTab(mfd);
	
	mfd->nmfunc = IDC_LEVELS;
	setFlg(mfd);
	mfd->nmfunc = IDC_SAT;
	setFlg(mfd);
	mfd->nmfunc = IDC_GAMMA;
	setFlg(mfd);
	mfd->nmfunc = IDC_MPO; 
	setFlg(mfd);
	mfd->nmfunc	= IDC_SHARP;
	setFlg(mfd);
	mfd->nmfunc	= IDC_MIDDLE;
	setFlg(mfd);
}

ScriptFunctionDef func_defs[]={
	{ (ScriptFunctionPtr)ScriptConfig, "Config", "0iiiiiiiiiiiiiii" },
		//										   123456789abcdef
	{ NULL },
};

CScriptObject script_obj={
	NULL, func_defs
};

struct FilterDefinition filterDef_tutorial = {

	NULL, NULL, NULL,		// next, prev, module
	"Color Mill (2.1)",	// name
	"Adjust RGB, RGB gamma, Levels, Saturation, HSV and similar HSV(distorted), Middle Point, also  Smooth<->Sharp.",
							// desc
	"Eugene Khoroshavin", 		// maker
	NULL,					// private_data
	sizeof(MyFilterData),	// inst_data_size

	InitProc,				// initProc
	NULL,					// deinitProc
	RunProc,				// runProc
	NULL,					// paramProc
	ConfigProc, 			// configProc
	StringProc, 			// stringProc
	StartProc,				// startProc
	EndProc,				// endProc

	&script_obj,			// script_obj
	FssProc,				// fssProc

};

int StartProc(FilterActivation *fa, const FilterFunctions *ff)
{
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;
	InitMatrix(mfd);
	setTab(mfd);
	const PixDim	vol = fa->src.w * fa->src.h + fa->src.w;
	nwpic = new Pixel32 [vol];
	return 0;
}

int EndProc(FilterActivation *fa, const FilterFunctions *ff)
{
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;
	delete nwpic;
	return 0;
}

///////////////////////////////////////////////////////////////////////////

int RunProc(const FilterActivation *fa, const FilterFunctions *ff)
{
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;
	const PixDim	width = fa->src.w;
	const PixDim	height = fa->src.h;
	Pixel32 *src, *dst;
	int x, y;
	long r, g, b;
	Pixel32 *nwpi;
	nwpi = nwpic;
	src = fa->src.data;
	dst = fa->dst.data;
	PixOffset nnn = width * 4;
	for (y = 0; y < height; y++)
	{
		for (x = 0; x < width; x++)
		{
			r = ((src[x] & 0xff0000) >> 16);
			g = ((src[x] & 0xff00) >> 8);
			b = (src[x] & 0xff);
			if (mfd->shpr) sharp(mfd, &r, &g, &b, src, height, width, x, y, fa->src.pitch, 1);

			if (mfd->gamFlg && mfd->gPreFlg) gamma(mfd, &r, &g, &b);

			if (mfd->mpoFlg) midpoint(mfd, &r, &g, &b);

			if (mfd->hsvFlg) hsvdo(mfd, &r, &g, &b);

			if (mfd->lvFlg) lvels(mfd, &r, &g, &b);

			if (mfd->rgbFlg) rgbdo(mfd, &r, &g, &b);

			if (mfd->satFlg) satur(mfd, &r, &g, &b);

			if (mfd->gamFlg && !mfd->gPreFlg) gamma(mfd, &r, &g, &b);

			dst[x] = ((int) r << 16) | ((int) g << 8) | ((int) b);
			nwpi[x] = dst[x];
		}
		src	= (Pixel *)((char *)src + fa->src.pitch);
		dst	= (Pixel *)((char *)dst + fa->dst.pitch);
		nwpi = (Pixel *)((char *)nwpi + nnn);
	}
	if (mfd->shpo) {
		src = nwpic;
		dst = fa->dst.data;
		for (y = 0; y < height; y++)
		{
			for (x = 0; x < width; x++)
			{
				r = ((src[x] & 0xff0000) >> 16);
				g = ((src[x] & 0xff00) >> 8);
				b = (src[x] & 0xff);
				sharp(mfd, &r, &g, &b, src, height, width, x, y, nnn, 2);
				dst[x] = ((int) r << 16) | ((int) g << 8) | ((int) b);			 
			}
			src	= (Pixel *)((char *)src + nnn);
			dst	= (Pixel *)((char *)dst + fa->dst.pitch);
		}
	}
	return 0;
}

extern "C" int __declspec(dllexport) __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat);
extern "C" void __declspec(dllexport) __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff);

static FilterDefinition *fd_tutorial;

int __declspec(dllexport) __cdecl VirtualdubFilterModuleInit2(FilterModule *fm, const FilterFunctions *ff, int& vdfd_ver, int& vdfd_compat) {
	if (!(fd_tutorial = ff->addFilter(fm, &filterDef_tutorial, sizeof(FilterDefinition))))
		return 1;

	vdfd_ver = VIRTUALDUB_FILTERDEF_VERSION;
	vdfd_compat = VIRTUALDUB_FILTERDEF_COMPATIBLE;

	return 0;
}

void __declspec(dllexport) __cdecl VirtualdubFilterModuleDeinit(FilterModule *fm, const FilterFunctions *ff) {
	ff->removeFilter(fd_tutorial);
}

int InitProc(FilterActivation *fa, const FilterFunctions *ff) {
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;

	mfd->redD = 0;
	mfd->greenD = 0;
	mfd->blueD = 0;
	mfd->redM = 0;
	mfd->greenM = 0;
	mfd->blueM = 0;
	mfd->redL = 0;
	mfd->greenL = 0;
	mfd->blueL = 0;
	mfd->lvD = 0;
	mfd->lvM = 0;
	mfd->lvL = 0;
	mfd->satD = 0;
	mfd->satM = 0;
	mfd->satL = 0;
	mfd->gamR = 0;
	mfd->gamG = 0;
	mfd->gamB = 0;
	mfd->hue = 0;
	mfd->sat = 0;
	mfd->val = 0;
	mfd->mpl = 0;
	mfd->mps = 0;
	mfd->shi = 0;
	mfd->shpr = 0;
	mfd->shxz = 0;
	mfd->shpo = 0;
	mfd->shlow = 2;
	mfd->kebrFlg = TRUE;
	mfd->badFlg = FALSE;
	mfd->gPreFlg = TRUE;
	mfd->mpoFlg = FALSE;
	mfd->darlock = FALSE;
	mfd->midlock = FALSE;
	mfd->liglock = FALSE;
	mfd->levlock = FALSE;
	mfd->satlock = FALSE;
	mfd->gamlock = FALSE;
	mfd->hsvlock = FALSE;
	mfd->mpolock = FALSE;
	mfd->shalock = FALSE;
	mfd->levunion = FALSE;
	mfd->satunion = FALSE;
	mfd->gamunion = FALSE;
	mfd->realhsv = FALSE;
	mfd->rgbunion = FALSE;
	mfd->nmfunc = IDC_MIDDLE;
	return 0;
}

INT_PTR CALLBACK ConfigDlgProc(HWND hdlg, UINT msg, WPARAM wParam, LPARAM lParam)
{
	MyFilterData *mfd = (MyFilterData *)GetWindowLongPtr(hdlg, DWLP_USER);
	int mPr=0;
	bool redo = false;
	bool undo = false;

	switch(msg) {
		case WM_INITDIALOG:
			SetWindowLongPtr(hdlg, DWLP_USER, lParam);
			mfd = (MyFilterData *)lParam;
			HWND hWnd;

			setFlg(mfd);
			hWnd = GetDlgItem(hdlg, IDC_RED);
			SendMessage(hWnd, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(-100, 100));
			SendMessage(hWnd, TBM_SETTICFREQ, 10 , 0);
			hWnd = GetDlgItem(hdlg, IDC_GREEN);
			SendMessage(hWnd, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(-100, 100));
			SendMessage(hWnd, TBM_SETTICFREQ, 10, 0);
			hWnd = GetDlgItem(hdlg, IDC_BLUE);
			SendMessage(hWnd, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(-100, 100));
			SendMessage(hWnd, TBM_SETTICFREQ, 10, 0);

			hWnd = GetDlgItem(hdlg, IDC_LOWTR);
			SendMessage(hWnd, TBM_SETRANGE, (WPARAM)TRUE, MAKELONG(0, 63));
			SendMessage(hWnd, TBM_SETTICFREQ, 1, 0);

			SendMessage(GetDlgItem(hdlg, IDC_KEEPBR), BM_SETCHECK, mfd->kebrFlg, 0);
			SendMessage(GetDlgItem(hdlg, IDC_BEDSRC), BM_SETCHECK, mfd->badFlg, 0);
			if (mfd->gPreFlg) CheckRadioButton(hdlg,IDC_GPRE, IDC_GPOST, IDC_GPRE);
			else  CheckRadioButton(hdlg,IDC_GPRE, IDC_GPOST, IDC_GPOST);
			SendMessage(GetDlgItem(hdlg, IDC_DARLOCK), BM_SETCHECK, mfd->darlock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_MIDLOCK), BM_SETCHECK, mfd->midlock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_LIGLOCK), BM_SETCHECK, mfd->liglock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_LEVLOCK), BM_SETCHECK, mfd->levlock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_SATLOCK), BM_SETCHECK, mfd->satlock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_GAMLOCK), BM_SETCHECK, mfd->gamlock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_HSVLOCK), BM_SETCHECK, mfd->hsvlock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_LEVUNION), BM_SETCHECK, mfd->levunion, 0);
			SendMessage(GetDlgItem(hdlg, IDC_SATUNION), BM_SETCHECK, mfd->satunion, 0);
			SendMessage(GetDlgItem(hdlg, IDC_GAMUNION), BM_SETCHECK, mfd->gamunion, 0);
			SendMessage(GetDlgItem(hdlg, IDC_REALHSV), BM_SETCHECK, mfd->realhsv, 0);
			SendMessage(GetDlgItem(hdlg, IDC_RGBUNION), BM_SETCHECK, mfd->rgbunion, 0);
			SendMessage(GetDlgItem(hdlg, IDC_MPOLOCK), BM_SETCHECK, mfd->mpolock, 0);
			SendMessage(GetDlgItem(hdlg, IDC_SHALOCK), BM_SETCHECK, mfd->shalock, 0);

			CheckRadioButton(hdlg,IDC_DARK, IDC_SHARP, mfd->nmfunc);
			SendMessage(GetDlgItem(hdlg, mfd->nmfunc), BM_CLICK, TRUE, 0);


			mfd->ifp->InitButton(GetDlgItem(hdlg, IDPREVIEW));
			mfd->ifp->Toggle(hdlg);
			return TRUE;

		case WM_COMMAND:
			switch(LOWORD(wParam)) {
			case IDPREVIEW:
				mfd->ifp->Toggle(hdlg);
				break;
			case IDOK:
				EndDialog(hdlg, 0);
				return TRUE;
			case IDHELP:
				{
				char prog[256];
				char path[256];
				LPTSTR ptr;
				GetModuleFileName(NULL, prog, 255);
				GetFullPathName(prog, 255, path, &ptr);
				*ptr = 0;
				strcat_s(path, "plugins64\\ColorMill.txt");
				OutputDebugString(path);
				OutputDebugString("\n");
				strcpy_s(prog, "Notepad ");
				strcat_s(prog, path);
				WinExec(prog, SW_SHOW);
				return TRUE;
				}
			case IDCANCEL:
				EndDialog(hdlg, 1);
				return TRUE;
			case IDC_ZEROR:
				switch (mfd->nmfunc) {
				case IDC_DARK:
					if (mfd->darlock) undo = true;
					else if (mfd->rgbunion) {
							mfd->redD = 0;
							mfd->redM = 0;
							mfd->redL = 0;
					}
					else mfd->redD = 0;
					break;
				case IDC_MIDDLE:
					if (mfd->midlock) undo = true;
					else if (mfd->rgbunion) {
							mfd->redD = 0;
							mfd->redM = 0;
							mfd->redL = 0;
					}
					else mfd->redM = 0;
					break;
				case IDC_LIGHT:
					if (mfd->liglock) undo = true;
					else if (mfd->rgbunion) {
							mfd->redD = 0;
							mfd->redM = 0;
							mfd->redL = 0;
					}
					else mfd->redL = 0;
					break;
				case IDC_LEVELS:
					if (mfd->levlock) undo = true;
					else if (mfd->levunion) {
							mfd->lvD = 0;
							mfd->lvM = 0;
							mfd->lvL = 0;
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
					}
					else mfd->lvD = 0;
					break;
				case IDC_SAT:
					if (mfd->satlock) undo = true;
					else if (mfd->satunion) {
							mfd->satD = 0;
							mfd->satM = 0;
							mfd->satL = 0;
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
					}
					else mfd->satD = 0;
					break;
				case IDC_GAMMA:
					if (mfd->gamlock) undo = true;
					else if (mfd->gamunion) {
							mfd->gamR = 0;
							mfd->gamG = 0;
							mfd->gamB = 0;
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
					}
					else {
					mfd->gamR = 0;
					}
					break;
				case IDC_HSV:
					if (mfd->hsvlock) undo = true;
					else {
						mfd->hue = 0;
						InitMatrix(mfd);
					}
					break;
				case IDC_MPO:
					if (mfd->mpolock) undo = true;
					else {
						mfd->mpl = 0;
					}
					break;
				case IDC_SHARP:
					if (mfd->shalock) undo = true;
					else {
						mfd->shpr = 0;
					}
				}
				if (!undo) {
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
					setFlg(mfd);
					mfd->ifp->RedoFrame();
				}
				undo = false;
				return TRUE;
			case IDC_ZEROG:
				switch (mfd->nmfunc) {
				case IDC_DARK:
					if (mfd->darlock) undo = true;
					else if (mfd->rgbunion) {
							mfd->greenD = 0;
							mfd->greenM = 0;
							mfd->greenL = 0;
					}
					else mfd->greenD = 0;
					break;
				case IDC_MIDDLE:
					if (mfd->midlock) undo = true;
					else if (mfd->rgbunion) {
							mfd->greenD = 0;
							mfd->greenM = 0;
							mfd->greenL = 0;
					}
					else mfd->greenM = 0;
					break;
				case IDC_LIGHT:
					if (mfd->liglock) undo = true;
					else if (mfd->rgbunion) {
							mfd->greenD = 0;
							mfd->greenM = 0;
							mfd->greenL = 0;
					}
					else mfd->greenL = 0;
					break;
				case IDC_LEVELS:
					if (mfd->levlock) undo = true;
					else if (mfd->levunion) {
							mfd->lvD = 0;
							mfd->lvM = 0;
							mfd->lvL = 0;
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
					}
					else mfd->lvM = 0;
					break;
				case IDC_SAT:
					if (mfd->satlock) undo = true;
					else if (mfd->satunion) {
							mfd->satD = 0;
							mfd->satM = 0;
							mfd->satL = 0;
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
					}
					else mfd->satM = 0;
					break;
				case IDC_GAMMA:
					if (mfd->gamlock) undo = true;
					else if (mfd->gamunion) {
							mfd->gamR = 0;
							mfd->gamG = 0;
							mfd->gamB = 0;
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
					}
					else {
					mfd->gamG = 0;
					}
					break;
				case IDC_HSV:
					if (mfd->hsvlock) undo = true;
					else {
						mfd->sat = 0;
						InitMatrix(mfd);
					}
					break;
				case IDC_MPO:
					if (mfd->mpolock) undo = true;
					else {
						mfd->mps = 0;
					}
					break;
				case IDC_SHARP:
					if (mfd->shalock) undo = true;
					else {
						mfd->shxz = 0;
					}
				}
				if ( !undo) {
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
					setFlg(mfd);
					mfd->ifp->RedoFrame();
				}
				undo = false;
				return TRUE;
			case IDC_ZEROB:
				switch (mfd->nmfunc) {
				case IDC_DARK:
					if (mfd->darlock) undo = true;
					else if (mfd->rgbunion) {
							mfd->blueD = 0;
							mfd->blueM = 0;
							mfd->blueL = 0;
					}
					else mfd->blueD = 0;
					break;
				case IDC_MIDDLE:
					if (mfd->midlock) undo = true;
					else if (mfd->rgbunion) {
							mfd->blueD = 0;
							mfd->blueM = 0;
							mfd->blueL = 0;
					}
					else mfd->blueM = 0;
					break;
				case IDC_LIGHT:
					if (mfd->liglock) undo = true;
					else if (mfd->rgbunion) {
							mfd->blueD = 0;
							mfd->blueM = 0;
							mfd->blueL = 0;
					}
					else mfd->blueL = 0;
					break;
				case IDC_LEVELS:
					if (mfd->levlock) undo = true;
					else if (mfd->levunion) {
							mfd->lvD = 0;
							mfd->lvM = 0;
							mfd->lvL = 0;
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
					}
					else mfd->lvL = 0;
					break;
				case IDC_SAT:
					if (mfd->satlock) undo = true;
					else if (mfd->satunion) {
							mfd->satD = 0;
							mfd->satM = 0;
							mfd->satL = 0;
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
					}
					else mfd->satL = 0;
					break;
				case IDC_GAMMA:
					if (mfd->gamlock) undo = true;
					else if (mfd->gamunion) {
							mfd->gamR = 0;
							mfd->gamG = 0;
							mfd->gamB = 0;
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
					SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
					}
					else {
					mfd->gamB = 0;
					}
					break;
				case IDC_HSV:
					if (mfd->hsvlock) undo = true;
					else {
						mfd->val = 0;
						InitMatrix(mfd);
					}
					break;
				case IDC_MPO:
					if (mfd->mpolock) undo = true;
					else {
						mfd->shi = 0;
					}
					break;
				case IDC_SHARP:
					if (mfd->shalock) undo = true;
					else {
						mfd->shpo = 0;
					}
				}
				if (!undo) {
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
				setFlg(mfd);
				mfd->ifp->RedoFrame();
				}
				undo = false;
				return TRUE;
			case IDC_ZEROALL:
				SendMessage(GetDlgItem(hdlg, IDC_DARLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_MIDLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_LIGLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_LEVLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_SATLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_GAMLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_HSVLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_LEVUNION), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_SATUNION), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_GAMUNION), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_RGBUNION), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_MPOLOCK), BM_SETCHECK, FALSE, 0);
				SendMessage(GetDlgItem(hdlg, IDC_SHALOCK), BM_SETCHECK, FALSE, 0);
				mfd->darlock = FALSE;
				mfd->midlock = FALSE;
				mfd->liglock = FALSE;
				mfd->levlock = FALSE;
				mfd->satlock = FALSE;
				mfd->gamlock = FALSE;
				mfd->hsvlock = FALSE;
				mfd->levunion = FALSE;
				mfd->satunion = FALSE;
				mfd->gamunion = FALSE;
				mfd->rgbunion = FALSE;
				mfd->mpolock = FALSE;
				mfd->shalock = FALSE;

				mfd->shlow = 2;
				SendMessage(GetDlgItem(hdlg, IDC_LOWTR), TBM_SETPOS, (WPARAM)TRUE, 2);
				SetDlgItemInt(hdlg, IDC_LOWVAL, 2, TRUE);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, 0);
				SetDlgItemInt(hdlg, IDC_REDVAL, 0, TRUE);
				mfd->redD = 0;
				mfd->redM = 0;
				mfd->redL = 0;
				mfd->lvD = 0;
				mfd->satD = 0;
				mfd->gamR = 0;
				mfd->hue = 0;
				mfd->mpl = 0;
				mfd->shpr = 0;
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, 0);
				SetDlgItemInt(hdlg, IDC_GREENVAL, 0, TRUE);
				mfd->greenD = 0;
				mfd->greenM = 0;
				mfd->greenL = 0;
				mfd->lvM = 0;
				mfd->satM = 0;
				mfd->gamG = 0;
				mfd->sat = 0;
				mfd->mps = 0;
				mfd->shxz = 0;
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, 0);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, 0, TRUE);
				mfd->blueD = 0;
				mfd->blueM = 0;
				mfd->blueL = 0;
				mfd->lvL = 0;
				mfd->satL = 0;
				mfd->gamB = 0;
				mfd->val = 0;
				mfd->shi = 0;
				mfd->shpo = 0;
				setFlg(mfd);
				InitMatrix(mfd);
				mfd->ifp->RedoFrame();
				return TRUE;
			case IDC_DARK:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, rtxt);
				SetDlgItemText(hdlg, IDC_GTEXT, gtxt);
				SetDlgItemText(hdlg, IDC_BTEXT, btxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func5txt);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->redD);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->redD, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->greenD);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->greenD, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->blueD);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->blueD, TRUE);
				mfd->nmfunc = IDC_DARK;
				return TRUE;
			case IDC_MIDDLE:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, rtxt);
				SetDlgItemText(hdlg, IDC_GTEXT, gtxt);
				SetDlgItemText(hdlg, IDC_BTEXT, btxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func6txt);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->redM);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->redM, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->greenM);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->greenM, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->blueM);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->blueM, TRUE);
				mfd->nmfunc = IDC_MIDDLE;
				return TRUE;
			case IDC_LIGHT:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, rtxt);
				SetDlgItemText(hdlg, IDC_GTEXT, gtxt);
				SetDlgItemText(hdlg, IDC_BTEXT, btxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func7txt);
				
				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->redL);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->redL, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->greenL);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->greenL, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->blueL);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->blueL, TRUE);
				mfd->nmfunc = IDC_LIGHT;
				return TRUE;
			case IDC_LEVELS:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, dtxt);
				SetDlgItemText(hdlg, IDC_GTEXT, mtxt);
				SetDlgItemText(hdlg, IDC_BTEXT, ltxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func4txt);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->lvD);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->lvD, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->lvM);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->lvM, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->lvL);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->lvL, TRUE);
				mfd->nmfunc = IDC_LEVELS;
				return TRUE;
			case IDC_SAT:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, dtxt);
				SetDlgItemText(hdlg, IDC_GTEXT, mtxt);
				SetDlgItemText(hdlg, IDC_BTEXT, ltxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func8txt);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->satD);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->satD, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->satM);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->satM, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->satL);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->satL, TRUE);
				mfd->nmfunc = IDC_SAT;
				return TRUE;
			case IDC_GAMMA:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, rtxt);
				SetDlgItemText(hdlg, IDC_GTEXT, gtxt);
				SetDlgItemText(hdlg, IDC_BTEXT, btxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func3txt);
				
				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->gamR);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->gamR, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->gamG);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->gamG, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->gamB);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->gamB, TRUE);
				mfd->nmfunc = IDC_GAMMA;
				return TRUE;
			case IDC_HSV:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, htxt);
				SetDlgItemText(hdlg, IDC_GTEXT, stxt);
				SetDlgItemText(hdlg, IDC_BTEXT, vtxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func2txt);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->hue);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->hue, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->sat);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->sat, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->val);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->val, TRUE);
				mfd->nmfunc = IDC_HSV;
				return TRUE;
			case IDC_MPO:
				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_HIDE);
				SetDlgItemText(hdlg, IDC_RTEXT, mptxt);
				SetDlgItemText(hdlg, IDC_GTEXT, mstxt);
				SetDlgItemText(hdlg, IDC_BTEXT, sitxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func1txt);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->mpl);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->mpl, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->mps);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->mps, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->shi);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->shi, TRUE);
				mfd->nmfunc = IDC_MPO;
				return TRUE;
			case IDC_SHARP:
				SetDlgItemText(hdlg, IDC_RTEXT, shprtxt);
				SetDlgItemText(hdlg, IDC_GTEXT, shxztxt);
				SetDlgItemText(hdlg, IDC_BTEXT, shpotxt);
				SetDlgItemText(hdlg, IDC_FUNAM, func9txt);

				ShowWindow(GetDlgItem(hdlg, IDC_STATICK), SW_HIDE);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWTR), SW_SHOW);
				ShowWindow(GetDlgItem(hdlg, IDC_LOWVAL), SW_SHOW);

				SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -mfd->shpr);
				SetDlgItemInt(hdlg, IDC_REDVAL, mfd->shpr, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -mfd->shxz);
				SetDlgItemInt(hdlg, IDC_GREENVAL, mfd->shxz, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -mfd->shpo);
				SetDlgItemInt(hdlg, IDC_BLUEVAL, mfd->shpo, TRUE);
				SendMessage(GetDlgItem(hdlg, IDC_LOWTR), TBM_SETPOS, (WPARAM)TRUE, mfd->shlow);
				SetDlgItemInt(hdlg, IDC_LOWVAL, mfd->shlow, TRUE);
				mfd->nmfunc = IDC_SHARP;
				return TRUE;
			case IDC_KEEPBR:{
				mfd->kebrFlg = IsDlgButtonChecked(hdlg, IDC_KEEPBR);
				setFlg(mfd);
				mfd->ifp->RedoFrame();}
				return TRUE;
			case IDC_BEDSRC:{
				mfd->badFlg = IsDlgButtonChecked(hdlg, IDC_BEDSRC);
				setFlg(mfd);
				mfd->ifp->RedoFrame();}
				return TRUE;
			case IDC_GPRE:
			case IDC_GPOST:
				{
				mfd->gPreFlg = IsDlgButtonChecked(hdlg, IDC_GPRE);
				setFlg(mfd);
				mfd->ifp->RedoFrame();}
				return TRUE;
			case IDC_DARLOCK:
			case IDC_MIDLOCK:
			case IDC_LIGLOCK:
				if (IsDlgButtonChecked(hdlg, LOWORD(wParam))) {
				SendMessage(GetDlgItem(hdlg, IDC_RGBUNION), BM_SETCHECK, FALSE, 0);
				mfd->rgbunion = FALSE;
				}
			case IDC_LEVLOCK:
			case IDC_SATLOCK:
			case IDC_GAMLOCK:
			case IDC_HSVLOCK:
			case IDC_LEVUNION:
			case IDC_SATUNION:
			case IDC_GAMUNION:
			case IDC_MPOLOCK:
			case IDC_SHALOCK:
				mfd->darlock = IsDlgButtonChecked(hdlg, IDC_DARLOCK);
				mfd->midlock = IsDlgButtonChecked(hdlg, IDC_MIDLOCK);
				mfd->liglock = IsDlgButtonChecked(hdlg, IDC_LIGLOCK);
				mfd->levlock = IsDlgButtonChecked(hdlg, IDC_LEVLOCK);
				mfd->satlock = IsDlgButtonChecked(hdlg, IDC_SATLOCK);
				mfd->gamlock = IsDlgButtonChecked(hdlg, IDC_GAMLOCK);
				mfd->hsvlock = IsDlgButtonChecked(hdlg, IDC_HSVLOCK);
				mfd->levunion = IsDlgButtonChecked(hdlg, IDC_LEVUNION);
				mfd->satunion = IsDlgButtonChecked(hdlg, IDC_SATUNION);
				mfd->gamunion = IsDlgButtonChecked(hdlg, IDC_GAMUNION);
				mfd->mpolock = IsDlgButtonChecked(hdlg, IDC_MPOLOCK);
				mfd->shalock = IsDlgButtonChecked(hdlg, IDC_SHALOCK);
				return TRUE;
			case IDC_RGBUNION:
				if (mfd->darlock || mfd->midlock || mfd->liglock) {
					SendMessage(GetDlgItem(hdlg, IDC_RGBUNION), BM_SETCHECK, FALSE, 0);
					mfd->rgbunion = FALSE;
				}
				else mfd->rgbunion = IsDlgButtonChecked(hdlg, IDC_RGBUNION);
				return TRUE;
			case IDC_REALHSV:
				mfd->realhsv = IsDlgButtonChecked(hdlg, IDC_REALHSV);
				if (IsDlgButtonChecked(hdlg, IDC_REALHSV)) {
					InitMatrix(mfd);
				}
					mfd->ifp->RedoFrame();
			return TRUE;
			}
			break;
		case WM_VSCROLL:
			{
				int red, green, blue, redold, greenold, blueold, dpos, shlow, shlowold;
				red = -(int)SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_GETPOS, 0, 0);
				green = -(int)SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_GETPOS, 0, 0);
				blue = -(int)SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_GETPOS, 0, 0);
				switch (mfd->nmfunc) {
				case IDC_DARK:
						redold = mfd->redD;
						greenold = mfd->greenD;
						blueold = mfd->blueD;
					if (mfd->darlock) undo = true;
					else {
						if((mfd->redD != red) | (mfd->greenD != green) | (mfd->blueD != blue)) {
							redo = true;
							if (mfd->rgbunion) {
								if (red != redold) {
									dpos = red - redold;
									dpos = normpos(mfd->redD, mfd->redM, mfd->redL, dpos);
									red = redold + dpos;
									mfd->redM += dpos;
									mfd->redL += dpos;
								}
								if (green != greenold) {
									dpos = green - greenold;
									dpos = normpos(mfd->greenD, mfd->greenM, mfd->greenL, dpos);
									green = greenold + dpos;
									mfd->greenM += dpos;
									mfd->greenL += dpos;
								}
								if (blue != blueold) {
									dpos = blue - blueold;
									dpos = normpos(mfd->blueD, mfd->blueM, mfd->blueL, dpos);
									blue = blueold + dpos;
									mfd->blueM += dpos;
									mfd->blueL += dpos;
								}
							}
							mfd->redD = red;
							mfd->greenD = green;
							mfd->blueD = blue;
						}
					}
					break;
				case IDC_MIDDLE:
						redold = mfd->redM;
						greenold = mfd->greenM;
						blueold = mfd->blueM;
					if (mfd->midlock) undo = true;
					else {
						if((mfd->redM != red) | (mfd->greenM != green) | (mfd->blueM != blue)) {
							redo = true;
							if (mfd->rgbunion) {
								if (red != redold) {
									dpos = red - redold;
									dpos = normpos(mfd->redD, mfd->redM, mfd->redL, dpos);
									red = redold + dpos;
									mfd->redD += dpos;
									mfd->redL += dpos;
								}
								if (green != greenold) {
									dpos = green - greenold;
									dpos = normpos(mfd->greenD, mfd->greenM, mfd->greenL, dpos);
									green = greenold + dpos;
									mfd->greenD += dpos;
									mfd->greenL += dpos;
								}
								if (blue != blueold) {
									dpos = blue - blueold;
									dpos = normpos(mfd->blueD, mfd->blueM, mfd->blueL, dpos);
									blue = blueold + dpos;
									mfd->blueD += dpos;
									mfd->blueL += dpos;
								}
							}
							mfd->redM = red;
							mfd->greenM = green;
							mfd->blueM = blue;
						}
					}
					break;
				case IDC_LIGHT:
						redold = mfd->redL;
						greenold = mfd->greenL;
						blueold = mfd->blueL;
					if (mfd->liglock) undo = true;
					else {
						if((mfd->redL != red) | (mfd->greenL != green) | (mfd->blueL != blue)) {
							redo = true;
							if (mfd->rgbunion) {
								if (red != redold) {
									dpos = red - redold;
									dpos = normpos(mfd->redD, mfd->redM, mfd->redL, dpos);
									red = redold + dpos;
									mfd->redD += dpos;
									mfd->redM += dpos;
								}
								if (green != greenold) {
									dpos = green - greenold;
									dpos = normpos(mfd->greenD, mfd->greenM, mfd->greenL, dpos);
									green = greenold + dpos;
									mfd->greenD += dpos;
									mfd->greenM += dpos;
								}
								if (blue != blueold) {
									dpos = blue - blueold;
									dpos = normpos(mfd->blueD, mfd->blueM, mfd->blueL, dpos);
									blue = blueold + dpos;
									mfd->blueD += dpos;
									mfd->blueM += dpos;
								}
							}
							mfd->redL = red;
							mfd->greenL = green;
							mfd->blueL = blue;
						}
					}
					break;
				case IDC_LEVELS:
						redold = mfd->lvD;
						greenold = mfd->lvM;
						blueold = mfd->lvL;
					if (mfd->levlock) undo = true;
					else {
						if((mfd->lvD != red) | (mfd->lvM != green) | (mfd->lvL != blue)) {
							redo = true;
							if (mfd->levunion) {
								dpos = (red - redold) + (green - greenold) + (blue - blueold);
								dpos = normpos(redold, greenold, blueold, dpos);
								red = redold + dpos;
								green = greenold + dpos;
								blue = blueold + dpos;
							}
							mfd->lvD = red;
							mfd->lvM = green;
							mfd->lvL = blue;
						}
					}
					break;
				case IDC_SAT:
						redold = mfd->satD;
						greenold = mfd->satM;
						blueold = mfd->satL;
					if (mfd->satlock) undo = true;
					else {
						if((mfd->satD != red) | (mfd->satM != green) | (mfd->satL != blue)) {
							redo = true;
							if (mfd->satunion) {
								dpos = (red - redold) + (green - greenold) + (blue - blueold);
								dpos = normpos(redold, greenold, blueold, dpos);
								red = redold + dpos;
								green = greenold + dpos;
								blue = blueold + dpos;
							}
							mfd->satD = red;
							mfd->satM = green;
							mfd->satL = blue;
						}
					}
					break;
				case IDC_GAMMA:
						redold = mfd->gamR;
						greenold = mfd->gamG;
						blueold = mfd->gamB;
					if (mfd->gamlock) undo = true;
					else {
						if((mfd->gamR != red) | (mfd->gamG != green) | (mfd->gamB != blue)) {
							redo = true;
							if (mfd->gamunion) {
								dpos = (red - redold) + (green - greenold) + (blue - blueold);
								dpos = normpos(redold, greenold, blueold, dpos);
								red = redold + dpos;
								green = greenold + dpos;
								blue = blueold + dpos;
							}
							mfd->gamR = red;
							mfd->gamG = green;
							mfd->gamB = blue;
						}
					}
					break;
				case IDC_HSV:
						redold = mfd->hue;
						greenold = mfd->sat;
						blueold = mfd->val;
					if (mfd->hsvlock) undo = true;
					else {
						if((mfd->hue != red) | (mfd->sat != green) | (mfd->val != blue)) {
							redo = true;
							mfd->hue = red;
							mfd->sat = green;
							mfd->val = blue;
							InitMatrix(mfd);
						}
					}
					break;
				case IDC_MPO:
						redold = mfd->mpl;
						greenold = mfd->mps;
						blueold = mfd->shi;
					if (mfd->mpolock) undo = true;
					else {
						if((mfd->mpl != red) | (mfd->mps != green) | (mfd->shi != blue)) {
							redo = true;
							mfd->mpl = red;
							mfd->mps = green;
							mfd->shi = blue;
						}
					}
					break;
				case IDC_SHARP:
						redold = mfd->shpr;
						greenold = mfd->shxz;
						blueold = mfd->shpo;
						shlowold = mfd->shlow;
						shlow = (int)SendMessage(GetDlgItem(hdlg, IDC_LOWTR), TBM_GETPOS, 0, 0);
						if (mfd->shalock) {
							undo = true;
							SendMessage(GetDlgItem(hdlg, IDC_LOWTR), TBM_SETPOS, (WPARAM)TRUE, shlowold);
						}
					else {
						if((mfd->shpr != red) | (mfd->shxz != green) | (mfd->shpo != blue) | (mfd->shlow != shlow)) {
							redo = true;
							mfd->shpr = red;
							mfd->shxz = green;
							mfd->shpo = blue;
							mfd->shlow = shlow;
							SetDlgItemInt(hdlg, IDC_LOWVAL, mfd->shlow, TRUE);
						}
					}
					break;
				}
				if (undo) {
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -redold);
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -greenold);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -blueold);
				}
				if (redo) {
					SendMessage(GetDlgItem(hdlg, IDC_RED), TBM_SETPOS, (WPARAM)TRUE, -red);
					SendMessage(GetDlgItem(hdlg, IDC_GREEN), TBM_SETPOS, (WPARAM)TRUE, -green);
					SendMessage(GetDlgItem(hdlg, IDC_BLUE), TBM_SETPOS, (WPARAM)TRUE, -blue);
					SetDlgItemInt(hdlg, IDC_REDVAL, red, TRUE);
					SetDlgItemInt(hdlg, IDC_GREENVAL, green, TRUE);
					SetDlgItemInt(hdlg, IDC_BLUEVAL, blue, TRUE);
					setFlg(mfd);
					mfd->ifp->RedoFrame();
				}
				undo = false;
				redo = false;
				break;
			}
	}

	return FALSE;
}

int ConfigProc(FilterActivation *fa, const FilterFunctions *ff, HWND hwnd)
{
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;
	MyFilterData mfd_old = *mfd;
	int ret;

	mfd->ifp = fa->ifp;

	if (DialogBoxParam(fa->filter->module->hInstModule, MAKEINTRESOURCE(IDD_FILTER),
		hwnd, ConfigDlgProc, (LPARAM)mfd))
	{
		*mfd = mfd_old;
		ret = TRUE;
	}
    else
	{
		ret = FALSE;
	}
	return(ret);
}

void StringProc(const FilterActivation *fa, const FilterFunctions *ff, char *str)
{
	MyFilterData *mfd = (MyFilterData *)fa->filter_data;
}
 

void setFlg ( MyFilterData *mfd ) {
	int i;
	mfd->rgbFlg =  ((mfd->redD != 0) || (mfd->greenD != 0) || (mfd->blueD != 0)
		|| (mfd->redM != 0) || (mfd->greenM != 0) || (mfd->blueM != 0)
		|| (mfd->redL != 0) || (mfd->greenL != 0) || (mfd->blueL != 0));
	mfd->lvFlg =  ((mfd->lvD != 0) || (mfd->lvM != 0) || (mfd->lvL != 0));
	mfd->satFlg = ((mfd->satD != 0) || (mfd->satM != 0) || (mfd->satL != 0));
	mfd->gamFlg = ((mfd->gamR != 0) || (mfd->gamG != 0) || (mfd->gamB != 0));
	mfd->hsvFlg = ((mfd->hue != 0) || (mfd->sat != 0) || (mfd->val != 0));
	mfd->mpoFlg = (mfd->mpl != 0);
		switch (mfd->nmfunc) {
		case IDC_DARK:
		case IDC_MIDDLE:
		case IDC_LIGHT:
		for (i = 0; i < 256; i++) {
			long upm1 = 128 - i;
			long upm2 = 255 - i;
			long upm3 = i - 128;
				if (mfd->badFlg) {
					if (i < 128) {
						mfd->incr[i] = (mfd->redD * mfd->partcoef[i] + mfd->redM * mfd->partcoef[upm1]) / u22;
						mfd->incg[i] = (mfd->greenD * mfd->partcoef[i] + mfd->greenM * mfd->partcoef[upm1]) / u22;
						mfd->incb[i] = (mfd->blueD * mfd->partcoef[i] + mfd->blueM * mfd->partcoef[upm1]) / u22;
					}
						else {
							mfd->incr[i] = (mfd->redM * mfd->partcoef[upm3] + mfd->redL * mfd->partcoef[upm2]) / u22;
							mfd->incg[i] = (mfd->greenM * mfd->partcoef[upm3] + mfd->greenL * mfd->partcoef[upm2]) / u22;
							mfd->incb[i] = (mfd->blueM * mfd->partcoef[upm3] + mfd->blueL * mfd->partcoef[upm2]) / u22;
						}
				}
				else {
					if (i < 128) {
						mfd->incr[i] = (mfd->redD * upm1 + mfd->redM * i)/128;
						mfd->incg[i] = (mfd->greenD * upm1 + mfd->greenM * i)/128;
						mfd->incb[i] = (mfd->blueD * upm1 + mfd->blueM * i)/128;
					}	
						else {
							mfd->incr[i] = (mfd->redM * upm2 + mfd->redL * upm3)/128;
							mfd->incg[i] = (mfd->greenM * upm2 + mfd->greenL * upm3)/128;
							mfd->incb[i] = (mfd->blueM * upm2 + mfd->blueL * upm3)/128;
						}		
				}
		}
			break;
		case IDC_LEVELS:
			for (i = 0; i < 256; i++) {
				long upm1 = 128 - i;
				long upm2 = 255 - i;
				long upm3 = i - 128;
				if (mfd->badFlg) {
					if (i < 128) mfd->incl[i] = (mfd->lvD * mfd->partcoef[i] + mfd->lvM * mfd->partcoef[upm1]) / u22;
					else mfd->incl[i] = (mfd->lvM * mfd->partcoef[upm3] + mfd->lvL * mfd->partcoef[upm2]) / u22;
				}
				else {
					if (i < 128) mfd->incl[i] = (mfd->lvD * upm1 + mfd->lvM * i)/128;
					else mfd->incl[i] = (mfd->lvM * upm2 + mfd->lvL * upm3)/128;
				}
			}
				break;
		case IDC_SAT:
			for (i = 0; i < 256; i++) {
				long upm1 = 128 - i;
				long upm2 = 255 - i;
				long upm3 = i - 128;
				if (mfd->badFlg) {
					if (i < 128) mfd->incs[i] = (mfd->satD * mfd->partcoef[i] + mfd->satM * mfd->partcoef[upm1]) / u22;
					else mfd->incs[i] = (mfd->satM * mfd->partcoef[upm3] + mfd->satL * mfd->partcoef[upm2]) / u22;
				}
				else {
					if (i < 128) mfd->incs[i] = (mfd->satD * upm1 + mfd->satM * i)/128;
					else mfd->incs[i] = (mfd->satM * upm2 + mfd->satL * upm3)/128;
				}
				mfd->incs[i] = 100 + mfd->incs[i] * 2 + abs(mfd->incs[i]);
				if (mfd->incs[i] < 0) mfd->incs[i] = 0;
			}
			break;
		case IDC_GAMMA:
			for (i = 0; i < 256; i++) {
				if (mfd->gamR <= 0) mfd->rtbl[i] = (unsigned char)(255 * pow(double(i+1)/256 , double(mfd->gamR * mfd->gamR)/1000 + 1));
				else mfd->rtbl[i] = (unsigned char)(255 * pow(double(i+1)/256 , double(100 - mfd->gamR)/100));
				if (mfd->gamG <= 0) mfd->gtbl[i] = (unsigned char)(255 * pow(double(i+1)/256 , double(mfd->gamG * mfd->gamG)/1000 + 1));
				else mfd->gtbl[i] = (unsigned char)(255 * pow(double(i+1)/256 , double(100 - mfd->gamG)/100));
				if (mfd->gamB <= 0) mfd->btbl[i] = (unsigned char)(255 * pow(double(i+1)/256 , double(mfd->gamB * mfd->gamB)/1000 + 1));
				else mfd->btbl[i] = (unsigned char)(255 * pow(double(i+1)/256 , double(100 - mfd->gamB)/100));
			}
			break;
		case IDC_MPO: 
			double partc, bas, leftpn, leftpk;
			long leftp;

			leftp = 64 + mfd->shi/2;
			leftpn = leftp;
			leftpn = leftpn * leftpn;
			leftpk = 255.0 - leftp;
			leftpk = leftpk * leftpk;
			bas = (double)(128 + mfd->mps/2);
			bas = bas * bas;
				for (i = 0; i < 256; i++) {
					if (i < leftp) {
						partc = (double) (i * i);
						partc = (bas * partc) / leftpn;
					}
					else {
						partc = i - leftp;
						partc = partc * partc * partc;
						partc = (bas - (bas * partc) / leftpk / (255.0 - leftp));
					}
					mfd->inclmp[i] = (long)(mfd->mpl * partc);
				}
			break;
	}
}


int normpos( int a, int b, int c, int dpos) {
int ret;
if (dpos >0) {
		ret = a;
		if (b > ret) ret = b;
		if (c > ret) ret = c;
		if ((ret + dpos) < 101) ret = dpos;
		else ret = 100 - ret;
}
else {
		ret = a;
		if (b < ret) ret = b;
		if (c < ret) ret = c;
		if ((ret + dpos) > -101) ret = dpos;
		else ret = -100 - ret;
}
return(ret);
}

void setTab (MyFilterData *mfd) {
	int i;
	for (i = 0; i < 201; i++) {
		double per = i - 100;
		per = (double)(M_PI * per)/100.0;
		mfd->cost[i] = (long) (cos(per) * 10000.0);
		mfd->sint[i] = (long) (sin(per) * 10000.0);
	}
	for (i = 0; i < 129; i++) {
		mfd->partcoef[i] = 16384 - i*i;
	}
}

void InitMatrix(MyFilterData *mfd)
{
	float i = (float) (1.0 + mfd->val * 0.01);
	float s = (float) (1.0 + (mfd->sat * 2 + abs(mfd->sat)) /100.0);
	float h = (float) (mfd->hue * 180 / 100);

	/* Create the transformation matrix. */
	identmat(mfd->mat);
	cscalemat(mfd->mat, i, i, i);
	saturatemat(mfd->mat, s);
	simplehuerotatemat(mfd->mat, h);
	mat2lmat(mfd->mat, mfd->lmat);
}


#define RLUM    ((float) (0.3086))
#define GLUM    ((float) (0.6094))
#define BLUM    ((float) (0.0820))

/* 
 *	matrixmult -	
 *		multiply two matricies
 */
void matrixmult(float a[4][4], float b[4][4], float c[4][4])
{
    int x, y;
    float temp[4][4];

    for(y=0; y<4 ; y++)
        for(x=0 ; x<4 ; x++) {
            temp[y][x] = b[y][0] * a[0][x]
                       + b[y][1] * a[1][x]
                       + b[y][2] * a[2][x]
                       + b[y][3] * a[3][x];
        }
    for(y=0; y<4; y++)
        for(x=0; x<4; x++)
            c[y][x] = temp[y][x];
}

/* 
 *	identmat -	
 *		make an identity matrix
 */
void identmat(float mat[4][4])
{
	float *matrix = (float *) mat;

    *matrix++ = 1.0;    /* row 1        */
    *matrix++ = 0.0;
    *matrix++ = 0.0;
    *matrix++ = 0.0;
    *matrix++ = 0.0;    /* row 2        */
    *matrix++ = 1.0;
    *matrix++ = 0.0;
    *matrix++ = 0.0;
    *matrix++ = 0.0;    /* row 3        */
    *matrix++ = 0.0;
    *matrix++ = 1.0;
    *matrix++ = 0.0;
    *matrix++ = 0.0;    /* row 4        */
    *matrix++ = 0.0;
    *matrix++ = 0.0;
    *matrix++ = 1.0;
}

/* 
 *	cscalemat -	
 *		make a color scale marix
 */
void cscalemat(float mat[4][4], float rscale, float gscale, float bscale)
{
    float mmat[4][4];

    mmat[0][0] = rscale;
    mmat[0][1] = 0.0;
    mmat[0][2] = 0.0;
    mmat[0][3] = 0.0;

    mmat[1][0] = 0.0;
    mmat[1][1] = gscale;
    mmat[1][2] = 0.0;
    mmat[1][3] = 0.0;


    mmat[2][0] = 0.0;
    mmat[2][1] = 0.0;
    mmat[2][2] = bscale;
    mmat[2][3] = 0.0;

    mmat[3][0] = 0.0;
    mmat[3][1] = 0.0;
    mmat[3][2] = 0.0;
    mmat[3][3] = 1.0;
    matrixmult(mmat,mat,mat);
}

/* 
 *	saturatemat -	
 *		make a saturation marix
 */
void saturatemat(float mat[4][4], float sat)
{
    float mmat[4][4];
    float a, b, c, d, e, f, g, h, i;
    float rwgt, gwgt, bwgt;

    rwgt = RLUM;
    gwgt = GLUM;
    bwgt = BLUM;

    a = (float) ((1.0-sat)*rwgt + sat);
    b = (float) ((1.0-sat)*rwgt);
    c = (float) ((1.0-sat)*rwgt);
    d = (float) ((1.0-sat)*gwgt);
    e = (float) ((1.0-sat)*gwgt + sat);
    f = (float) ((1.0-sat)*gwgt);
    g = (float) ((1.0-sat)*bwgt);
    h = (float) ((1.0-sat)*bwgt);
    i = (float) ((1.0-sat)*bwgt + sat);
    mmat[0][0] = a;
    mmat[0][1] = b;
    mmat[0][2] = c;
    mmat[0][3] = 0.0;

    mmat[1][0] = d;
    mmat[1][1] = e;
    mmat[1][2] = f;
    mmat[1][3] = 0.0;

    mmat[2][0] = g;
    mmat[2][1] = h;
    mmat[2][2] = i;
    mmat[2][3] = 0.0;

    mmat[3][0] = 0.0;
    mmat[3][1] = 0.0;
    mmat[3][2] = 0.0;
    mmat[3][3] = 1.0;
    matrixmult(mmat,mat,mat);
}

/* 
 *	xrotate -	
 *		rotate about the x (red) axis
 */
void xrotatemat(float mat[4][4], float rs, float rc)
{
    float mmat[4][4];

    mmat[0][0] = 1.0;
    mmat[0][1] = 0.0;
    mmat[0][2] = 0.0;
    mmat[0][3] = 0.0;

    mmat[1][0] = 0.0;
    mmat[1][1] = rc;
    mmat[1][2] = rs;
    mmat[1][3] = 0.0;

    mmat[2][0] = 0.0;
    mmat[2][1] = -rs;
    mmat[2][2] = rc;
    mmat[2][3] = 0.0;

    mmat[3][0] = 0.0;
    mmat[3][1] = 0.0;
    mmat[3][2] = 0.0;
    mmat[3][3] = 1.0;
    matrixmult(mmat,mat,mat);
}

/* 
 *	yrotate -	
 *		rotate about the y (green) axis
 */
void yrotatemat(float mat[4][4], float rs, float rc)
{
    float mmat[4][4];

    mmat[0][0] = rc;
    mmat[0][1] = 0.0;
    mmat[0][2] = -rs;
    mmat[0][3] = 0.0;

    mmat[1][0] = 0.0;
    mmat[1][1] = 1.0;
    mmat[1][2] = 0.0;
    mmat[1][3] = 0.0;

    mmat[2][0] = rs;
    mmat[2][1] = 0.0;
    mmat[2][2] = rc;
    mmat[2][3] = 0.0;

    mmat[3][0] = 0.0;
    mmat[3][1] = 0.0;
    mmat[3][2] = 0.0;
    mmat[3][3] = 1.0;
    matrixmult(mmat,mat,mat);
}

/* 
 *	zrotate -	
 *		rotate about the z (blue) axis
 */
void zrotatemat(float mat[4][4], float rs, float rc)
{
    float mmat[4][4];

    mmat[0][0] = rc;
    mmat[0][1] = rs;
    mmat[0][2] = 0.0;
    mmat[0][3] = 0.0;

    mmat[1][0] = -rs;
    mmat[1][1] = rc;
    mmat[1][2] = 0.0;
    mmat[1][3] = 0.0;

    mmat[2][0] = 0.0;
    mmat[2][1] = 0.0;
    mmat[2][2] = 1.0;
    mmat[2][3] = 0.0;

    mmat[3][0] = 0.0;
    mmat[3][1] = 0.0;
    mmat[3][2] = 0.0;
    mmat[3][3] = 1.0;
    matrixmult(mmat,mat,mat);
}

/* 
 *	zshear -	
 *		shear z using x and y.
 */
void zshearmat(float mat[4][4], float dx, float dy)
{
    float mmat[4][4];

    mmat[0][0] = 1.0;
    mmat[0][1] = 0.0;
    mmat[0][2] = dx;
    mmat[0][3] = 0.0;

    mmat[1][0] = 0.0;
    mmat[1][1] = 1.0;
    mmat[1][2] = dy;
    mmat[1][3] = 0.0;

    mmat[2][0] = 0.0;
    mmat[2][1] = 0.0;
    mmat[2][2] = 1.0;
    mmat[2][3] = 0.0;

    mmat[3][0] = 0.0;
    mmat[3][1] = 0.0;
    mmat[3][2] = 0.0;
    mmat[3][3] = 1.0;
    matrixmult(mmat,mat,mat);
}

/* 
 *	simplehuerotatemat -	
 *		simple hue rotation. This changes luminance 
 */
void simplehuerotatemat(float mat[4][4], float rot)
{
    float mag;
    float xrs, xrc;
    float yrs, yrc;
    float zrs, zrc;

/* rotate the grey vector into positive Z */
    mag = (float) sqrt(2.0);
    xrs = (float) 1.0/mag;
    xrc = (float) 1.0/mag;
    xrotatemat(mat,xrs,xrc);

    mag = (float) sqrt(3.0);
    yrs = (float) -1.0/mag;
    yrc = (float) sqrt(2.0)/mag;
    yrotatemat(mat,yrs,yrc);

/* rotate the hue */
    zrs = (float) sin(rot*M_PI/180.0);
    zrc = (float) cos(rot*M_PI/180.0);
    zrotatemat(mat,zrs,zrc);

/* rotate the grey vector back into place */
    yrotatemat(mat,-yrs,yrc);
    xrotatemat(mat,-xrs,xrc);
}

/* 
 *	huerotatemat -	
 *		rotate the hue, while maintaining luminance.
 */

#define DENOM 0x10000

inline void lxformRGB(long mat[4][4], long *r, long *g, long *b)
{
    long ir, ig, ib;

	ir = *r;
	ig = *g;
	ib = *b;
	*r = (long) ((ir*mat[0][0] + ig*mat[1][0] + ib*mat[2][0] /*+ mat[3][0]*/)/DENOM);
	*g = (long) ((ir*mat[0][1] + ig*mat[1][1] + ib*mat[2][1] /*+ mat[3][1]*/)/DENOM);
	*b = (long) ((ir*mat[0][2] + ig*mat[1][2] + ib*mat[2][2] /*+ mat[3][2]*/)/DENOM);
	if(*r<0) *r = 0;
	else if(*r>255) *r = 255;
	if(*g<0) *g = 0;
	else if(*g>255) *g = 255;
	if(*b<0) *b = 0;
	else if(*b>255) *b = 255;
}

void mat2lmat(float mat[4][4], long lmat[4][4])
{
	int x, y;

	for (y = 0; y < 4; y++)
		for (x = 0; x < 4; x++) 
			lmat[y][x] = (long) (mat[y][x] * DENOM);
}

inline void normrgb(long *r, long *g, long *b) {
	if (*r < 0) *r = 0;
	if (*r > 255) *r = 255;
	if (*g < 0) *g = 0;
	if (*g > 255) *g = 255;
	if (*b < 0) *b = 0;
	if (*b > 255) *b = 255;
}

inline void gamma(MyFilterData *mfd, long *r, long *g, long *b) {
	*r = mfd->rtbl[*r];
	*g = mfd->gtbl[*g];
	*b = mfd->btbl[*b];
}

inline void sharp(MyFilterData *mfd, long *r, long *g, long *b, Pixel32 *src, PixDim height, PixDim width, int x, int y, PixOffset pitch, int ncall) {
	Pixel32 *src0;
	Pixel32 *src2;
	long UU, u, u1;
	int x0 = x - 1;
	int x2 = x + 1;
	int dy, DU;
	int dlow = mfd->shlow;
	u = *r + *g + *b;
	DU = (int)(128 + (mfd->shxz * 128)/100);
	if (y == 0) {
		src2 = (Pixel *)((char *)src + pitch);
		if (x == 0) {
			u1 = ((src[x2] & 0xff0000) >> 16) + ((src[x2] & 0xff00) >> 8) + (src[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src2[x] & 0xff0000) >> 16) + ((src2[x] & 0xff00) >> 8) + (src2[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src2[x2] & 0xff0000) >> 16) + ((src2[x2] & 0xff00) >> 8) + (src2[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/9;
		}
		else if (x == width) {
			u1 = ((src[x0] & 0xff0000) >> 16) + ((src[x0] & 0xff00) >> 8) + (src[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src2[x] & 0xff0000) >> 16) + ((src2[x] & 0xff00) >> 8) + (src2[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src2[x0] & 0xff0000) >> 16) + ((src2[x0] & 0xff00) >> 8) + (src2[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/9;
		}
		else {
			u1 = ((src[x2] & 0xff0000) >> 16) + ((src[x2] & 0xff00) >> 8) + (src[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src2[x] & 0xff0000) >> 16) + ((src2[x] & 0xff00) >> 8) + (src2[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src2[x2] & 0xff0000) >> 16) + ((src2[x2] & 0xff00) >> 8) + (src2[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src[x0] & 0xff0000) >> 16) + ((src[x0] & 0xff00) >> 8) + (src[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src2[x0] & 0xff0000) >> 16) + ((src2[x0] & 0xff00) >> 8) + (src2[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/15;
		}
	}
	else if (y == height) {
		src0	= (Pixel *)((char *)src - pitch);
		if (x == 0) {
			u1 = ((src[x2] & 0xff0000) >> 16) + ((src[x2] & 0xff00) >> 8) + (src[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src0[x] & 0xff0000) >> 16) + ((src0[x] & 0xff00) >> 8) + (src0[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src0[x2] & 0xff0000) >> 16) + ((src0[x2] & 0xff00) >> 8) + (src0[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/9;
		}
		else if (x == width) {
			u1 = ((src[x0] & 0xff0000) >> 16) + ((src[x0] & 0xff00) >> 8) + (src[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src0[x] & 0xff0000) >> 16) + ((src0[x] & 0xff00) >> 8) + (src0[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src0[x0] & 0xff0000) >> 16) + ((src0[x0] & 0xff00) >> 8) + (src0[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/9;
		}
		else {
			u1 = ((src[x2] & 0xff0000) >> 16) + ((src[x2] & 0xff00) >> 8) + (src[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src0[x] & 0xff0000) >> 16) + ((src0[x] & 0xff00) >> 8) + (src0[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src0[x2] & 0xff0000) >> 16) + ((src0[x2] & 0xff00) >> 8) + (src0[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src[x0] & 0xff0000) >> 16) + ((src[x0] & 0xff00) >> 8) + (src[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src0[x0] & 0xff0000) >> 16) + ((src0[x0] & 0xff00) >> 8) + (src0[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/15;
		}
	}
	else {
		src0	= (Pixel *)((char *)src - pitch);
		src2	= (Pixel *)((char *)src + pitch);
		if (x == 0) {
			u1 = ((src0[x] & 0xff0000) >> 16) + ((src0[x] & 0xff00) >> 8) + (src0[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src0[x2] & 0xff0000) >> 16) + ((src0[x2] & 0xff00) >> 8) + (src0[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src[x2] & 0xff0000) >> 16) + ((src[x2] & 0xff00) >> 8) + (src[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			//u1 = ((src2[x] & 0xff0000) >> 16) + ((src2[x] & 0xff00) >> 8) + (src2[x] & 0xff);// Breaks 2
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			//u1 = ((src2[x2] & 0xff0000) >> 16) + ((src2[x2] & 0xff00) >> 8) + (src2[x2] & 0xff);// Break 3
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/15;
		}
		else if (x == width) {
			u1 = ((src0[x] & 0xff0000) >> 16) + ((src0[x] & 0xff00) >> 8) + (src0[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src0[x0] & 0xff0000) >> 16) + ((src0[x0] & 0xff00) >> 8) + (src0[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src[x0] & 0xff0000) >> 16) + ((src[x0] & 0xff00) >> 8) + (src[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src2[x] & 0xff0000) >> 16) + ((src2[x] & 0xff00) >> 8) + (src2[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src2[x0] & 0xff0000) >> 16) + ((src2[x0] & 0xff00) >> 8) + (src2[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/15;
		}
		else {
			u1 = ((src0[x] & 0xff0000) >> 16) + ((src0[x] & 0xff00) >> 8) + (src0[x] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU = u1;
			else UU = u;
			u1 = ((src0[x2] & 0xff0000) >> 16) + ((src0[x2] & 0xff00) >> 8) + (src0[x2] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src0[x0] & 0xff0000) >> 16) + ((src0[x0] & 0xff00) >> 8) + (src0[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			//u1 = ((src[x2] & 0xff0000) >> 16) + ((src[x2] & 0xff00) >> 8) + (src[x2] & 0xff);// Breaks 6
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			//u1 = ((src2[x] & 0xff0000) >> 16) + ((src2[x] & 0xff00) >> 8) + (src2[x] & 0xff);// Breaks 4
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			//u1 = ((src2[x2] & 0xff0000) >> 16) + ((src2[x2] & 0xff00) >> 8) + (src2[x2] & 0xff);// Breaks 1
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			u1 = ((src[x0] & 0xff0000) >> 16) + ((src[x0] & 0xff00) >> 8) + (src[x0] & 0xff);
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			//u1 = ((src2[x0] & 0xff0000) >> 16) + ((src2[x0] & 0xff00) >> 8) + (src2[x0] & 0xff);// Breaks 5
			dy = abs((int)(u1-u));
			if ((dy < DU) && (dy > dlow)) UU += u1;
			else UU += u;
			UU = UU/24;
		}
	}
	int eff;
	if (ncall == 1) eff = mfd->shpr;
	else eff = mfd->shpo;
	u = u/3;
	if (eff > 0) {
		u = u - UU;
		u = (u * eff)/30;
	}
	else {
		u = u - UU;
		u = (u * eff)/100;
	}
	*r = *r + u;
	*g = *g + u;
	*b = *b + u;
	normrgb(r, g, b);
		
}

inline void hsvdo(MyFilterData *mfd, long *ri, long *gi, long *bi) {
		if (mfd->realhsv) {
			lxformRGB(mfd->lmat, ri, gi, bi);
			}
		else {
			long r = *ri;
			long g = *gi;
			long b = *bi;
			long V = 2990 * r + 5870 * g + 1140 * b;
			long c1 = (-1687 * r - 3313 * g + 5000 * b)/TTH;
			long c2 = (5000 * r - 4187 * g - 813 * b)/TTH;

			V += mfd->val * TTH;
			if (V > 2550000) V = 2550000;
			if (V < 0) V = 0;

			if (mfd->sat) {
				long incr = 100 + mfd->sat * 2 + abs(mfd->sat);
				c1 = (c1 * incr) / 100;
				c2 = (c2 * incr) / 100;
				if (c1 < -128) c1 = -128;
				if (c1 > 127) c1 = 127;
				if (c2 < -128) c2 = -128;
				if (c2 > 127) c2 = 127;
			}
			if (mfd->hue) {
				int ii = mfd->hue + 100;
				c1 = (c1 * mfd->cost[ii] + c2 * mfd->sint[ii])/TTH; // distortion. not real rotate
				c2 = (c2 * mfd->cost[ii] + c1 * mfd->sint[ii])/TTH; // distortion. not real rotate
				if (c1 < -128) c1 = -128;
				if (c1 > 127) c1 = 127;
				if (c2 < -128) c2 = -128;
				if (c2 > 127) c2 = 127;
			}
			r = (V + 14020 * c2)/TTH;
			g = (V - (34414 * c1 + 71414 * c2)/10)/TTH;
			b = (V + 17720 * c1)/TTH;
			normrgb(&r, &g, &b);
			*ri = r;
			*gi = g;
			*bi = b;
		}
}

inline void lvels(MyFilterData *mfd, long *r, long *g, long *b) {
	int u = (*r + *g + *b)/3;
	*r += mfd->incl[u];
	*g += mfd->incl[u];
	*b += mfd->incl[u];
	normrgb(r, g, b);
}

inline void rgbdo(MyFilterData *mfd, long *r, long *g, long *b) {
	long uold = *r + *g + *b;
	long u = (uold)/3;
	*r += mfd->incr[u];
	*g += mfd->incg[u];
	*b += mfd->incb[u];
	normrgb(r, g, b);

	if (mfd->kebrFlg) {
		long up = *r + *g + *b;
		if (up > 0) {
			u = uold - up;
			*r += (u * *r)/up;
			*g += (u * *g)/up;
			*b += (u * *b)/up;
			normrgb(r, g, b);
		}
	}
}

inline void satur(MyFilterData *mfd, long *r, long *g, long *b) {
	long u = (*r + *g + *b)/3;
	*r = u + ((*r - u) * mfd->incs[u])/100;
	*g = u + ((*g - u) * mfd->incs[u])/100;
	*b = u + ((*b - u) * mfd->incs[u])/100;
	normrgb(r, g, b);
}

inline void midpoint(MyFilterData *mfd, long *r, long *g, long *b) {
	long u, up, cf1, cf2, cf3, du;
	if (mfd->mpl > 0) {
		u = (*r + *g + *b)/3;
		cf3 = mfd->inclmp[u];
		du = u + (cf3 * u)/u22/100;
		if (du > 255) du = 255;
		if (du < 0) du = 0;
		up = abs(du - 128);
		cf1 = mfd->partcoef[128 - up];
		cf2 = mfd->partcoef[up];

		*r = *r + (((*r * cf1 + u * cf2)/u22) * cf3)/u22/50;
		*g = *g + (((*g * cf1 + u * cf2)/u22) * cf3)/u22/50;
		*b = *b + (((*b * cf1 + u * cf2)/u22) * cf3)/u22/50;
	}
	else {
		u = 255 - (*r + *g + *b)/3;
		cf3 = mfd->inclmp[u];
		du = u + (cf3 * u)/u22/100;
		if (du > 255) du = 255;
		if (du < 0) du = 0;
		up = abs(du - 128);
		cf1 = mfd->partcoef[128 - up];
		cf2 = mfd->partcoef[up];

		*r = *r + ((((255 - *r) * cf1 + u * cf2)/u22) * cf3)/u22/50;
		*g = *g + ((((255 - *g) * cf1 + u * cf2)/u22) * cf3)/u22/50;
		*b = *b + ((((255 - *b) * cf1 + u * cf2)/u22) * cf3)/u22/50;
	}

	normrgb(r, g, b);
}
