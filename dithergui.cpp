#include <iostream>
#include <fstream>
#include <vector>
#include <cmath>
#include <string>
#include <exception>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <iomanip>
#include <sstream>
#include <bitset>

#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <commctrl.h>
#include <resource.h>

#include "dither.h"
#include "image_pallete.h"
#include "color_spaces.h"

#include <boost/filesystem.hpp>
#include <boost/dll.hpp>
#include <boost/timer/timer.hpp>
#include <boost/algorithm/string.hpp>

#include "stb_image_resize.h"
#include "stb_image_write.h"
#include "stb_image.h"

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
LRESULT CALLBACK EditPrc(HWND, UINT, WPARAM, LPARAM);
LRESULT ProcessCustomDraw(LPARAM);
void AddControls(HWND);
void AddControlsITT(HWND);
void AddControlsTAC(HWND);
void AddControlsSplit(HWND);
void AddControlsBWB(HWND);
bool RegisterITTDialog(HINSTANCE);
void DisplayITTDialog(HWND);
bool RegisterSplitDialog(HINSTANCE);
void DisplaySplitDialog(HWND);
bool RegisterTACDialog(HINSTANCE);
void DisplayTACDialog(HWND,int);
bool RegisterBWBDialog(HINSTANCE);
void DisplayBWBDialog(HWND);
void OpenFile(HWND,HWND);
bool ColorDialogue(HWND,DWORD*);
bool AddColor(HWND,DWORD);

COLORREF colorWhite;
COLORREF colorBlack;

HMENU hMenu;

HWND mainWindow;

HWND hEditInputPath;
HWND hEditPalletePath;
HWND hEditScaleX;
HWND hEditScaleY;
HWND hEditColorMode;

HWND hHueTrack;

HWND hColorList;

HWND hITTInputPath;

HWND hPixelInfoList;

HWND hITTDialog;

HWND hSplitInputPath;

HWND hSplitColorList;
HWND hSplitColorGroupList;

HWND hSplitDialog;
HWND hSplitMergeButton;

HWND hBWBInputPath;
HWND hBWBDialog;
HWND hBWBConvertButton;

HWND hTACString;

HWND hTACDialog;
COLORREF TACColor;

COLORREF splitMergeColor;

static HBRUSH hBrush;

int selItem;
int TACSelItem;
int selItemSplit;
int selGroupItemSplit;

boost::filesystem::path currentPATH;

std::vector<std::array<int,3>> color_pallete;

std::vector<std::array<int,3>> split_color_pallete;
std::vector<std::array<int,3>> split_color_group;

typedef struct
{
	std::array<int,3> color;
	std::wstring text;
} colorTextPair;
std::vector<colorTextPair> color_stringPairs;

WNDPROC WPA;

std::array<int, 3> colorFromIndex(int index, HWND hWnd)
{
	char colorSTR[64];

	SendMessage(hWnd, LB_GETTEXT, (WPARAM)index, (LPARAM)colorSTR);
	std::vector<std::string> strs;
	boost::split(strs, colorSTR, boost::is_any_of(" "));

	std::array<int, 3> color = {
	(int)std::stod(strs[0]),
	(int)std::stod(strs[1]),
	(int)std::stod(strs[2]) };

	return color;
}

inline bool isBiggerThan(std::array<int,3> c1, std::array<int,3> c2)
{
	return ((c1[0]>c2[0]) || ((c1[0]==c2[0])&&(c1[1]>c2[1])) || ((c1[0]==c2[0])&&(c1[1]==c2[1])&&(c1[2]>c2[2])));
}

void QuickSort(int start, int end, std::vector<std::array<int,3>>& colorList)
{
	/*
	MessageBox(NULL,("SORT FROM " + std::to_string(start) + " TO " + std::to_string(end)).c_str(),"0",0);
	std::string str0;
	for(int i = 0; i < colorList.size(); i++)
	{
		str0+=std::to_string(colorList[i][0])+" "+std::to_string(colorList[i][1])+" "+std::to_string(colorList[i][2])+"\n";
	}
	MessageBox(NULL,(str0).c_str(),"0",0);
	*/
	if(end-start>1)
	{
		bool swapped = false;

		int i = start-1;
		for(int j = start; j < end; j++)
		{
			if(!isBiggerThan(colorList[j], colorList[end]))
			{
				i++;
				std::iter_swap(colorList.begin()+i, colorList.begin()+j);
				swapped = true;
			}
		}
		
		colorList.insert(colorList.begin()+i+1, colorList[end]);

		int insOffset = 0;
		if(end>i)
		{
			insOffset = 1;
		}
		colorList.erase(colorList.begin()+end+insOffset);

		if(i < start)
		{
			i = start;
		}
		QuickSort(0,i,colorList);
		QuickSort(i+1,end,colorList);
	} else
	{
		if(isBiggerThan(colorList[start],colorList[end]))
		{
			std::iter_swap(colorList.begin()+start, colorList.begin()+end);
		}
	}
}

void UpdateColorList()
{
	SendMessage(hColorList,LB_RESETCONTENT,0,0);
	for(int i = 0; i < color_pallete.size(); i++)
	{
		std::string fullSTR;
		fullSTR += std::to_string(color_pallete[i][0]);
		fullSTR += " ";
		fullSTR += std::to_string(color_pallete[i][1]);
		fullSTR += " ";
		fullSTR += std::to_string(color_pallete[i][2]);

		int pos = (int)SendMessage(hColorList, LB_ADDSTRING, 0, (LPARAM)fullSTR.c_str());
	}
}

void DrawItemFunc(LPARAM lParam)
{
	COLORREF clrBackground;
	COLORREF clrForeground;
	COLORREF selColor;
	COLORREF selColorInv;
	COLORREF colorClosest;
	COLORREF colorClosestInv;
	TEXTMETRIC tm;
	int x;
	int y;
	char achTemp[256];

	LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

	if (lpdis->itemID == -1)
	{
		return;
	}

	std::array<int,3> color = colorFromIndex(lpdis->itemID,lpdis->hwndItem);

	RGB rgbColor;
	rgbColor.r = color[0];
	rgbColor.g = color[1];
	rgbColor.b = color[2];

	LAB colorLAB = XYZtoLAB(RGBtoXYZ(rgbColor));

	RGB rgbColorInv = XYZtoRGB(LABtoXYZ(colorLAB));

	selColor = RGB(color[0],color[1],color[2]);
	selColorInv = RGB(255-rgbColorInv.r,255-rgbColorInv.g,255-rgbColorInv.b);

	if(colorLAB.l>50)
	{
		colorClosest = colorWhite;
		colorClosestInv = colorBlack;
	} else
	{
		colorClosestInv = colorWhite;
		colorClosest = colorBlack;
	}

	clrForeground = SetTextColor(lpdis->hDC,colorClosestInv);

	clrBackground = SetBkColor(lpdis->hDC,selColor);

	GetTextMetrics(lpdis->hDC,&tm);
	x = LOWORD(GetDialogBaseUnits())/4;
	y = (lpdis->rcItem.bottom + lpdis->rcItem.top - tm.tmHeight) / 2;

	SendMessage(lpdis->hwndItem,LB_GETTEXT,lpdis->itemID,(LPARAM)&achTemp);
	ExtTextOut(lpdis->hDC,10+2*x,y,ETO_CLIPPED|ETO_OPAQUE,&lpdis->rcItem,achTemp,(UINT)strlen(achTemp),NULL);

	SetTextColor(lpdis->hDC,clrForeground);
	SetBkColor(lpdis->hDC,clrBackground);

	if (lpdis->itemState & ODS_FOCUS)
	{
		DrawFocusRect(lpdis->hDC, &lpdis->rcItem);
	}
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
	WNDCLASS wc = {};

	wc.lpfnWndProc    = WndProc;
	wc.hInstance      = hInstance;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName  = "MyWinClass";
	
	colorWhite = RGB(255,255,255);
	colorBlack = RGB(0,0,0);

	if(!RegisterClass(&wc))
	{
		MessageBox(NULL,"call to registerclass failed","exception",0);
		
		return 0;
	}
	
	if(!RegisterITTDialog(hInstance))
	{
		MessageBox(NULL,"registering dialog class failed","exception",0);
		
		return 0;
	}

	if(!RegisterSplitDialog(hInstance))
	{
		MessageBox(NULL,"registering split dialog class failed","exception",0);

		return 0;
	}
	
	if(!RegisterBWBDialog(hInstance))
	{
		MessageBox(NULL,"registering bwb dialog class failed","exception",0);

		return 0;
	}
	
	if(!RegisterTACDialog(hInstance))
	{
		MessageBox(NULL,"registering dialog class 2 failed","exception",0);
		
		return 0;
	}
	
	HWND hWnd = CreateWindow
	(
		"MyWinClass",
		"Dither GUI",
		WS_OVERLAPPED | WS_MINIMIZEBOX | WS_SYSMENU,
		CW_USEDEFAULT, CW_USEDEFAULT,
		350, 345,
		NULL,
		NULL,
		NULL,
		NULL
	);

	mainWindow = hWnd;
	
	if(hWnd==NULL)
	{
		MessageBox(NULL,"window creation failed","exception",0);
		
		return 0;
	}
	
	ShowWindow(hWnd,nCmdShow);
	
	MSG msg = {};
	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}
	
	return 0;
}

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{	
	switch (message)
	{
		case WM_CREATE:
			currentPATH = boost::dll::program_location().parent_path();
			AddControls(hWnd);
			break;
			
		case WM_DESTROY:
			PostQuitMessage(0);
			break;

		case WM_DRAWITEM:
			DrawItemFunc(lParam);
			break;

		case WM_COMMAND:
			if(LOWORD(wParam)==IDC_CONVERTBUTTON)
			{			
				char scaleXSTR[1024];
				char scaleYSTR[1024];
				double scaleX;
				double scaleY;
				
				char inputSTR[1024];

				char colormodeSTR[1024];
				
				GetWindowText(hEditScaleX,scaleXSTR,1024);
				GetWindowText(hEditScaleY,scaleYSTR,1024);
				GetWindowText(hEditInputPath,inputSTR,1024);
				GetWindowText(hEditColorMode,colormodeSTR,1024);
				
				scaleX = std::stod(scaleXSTR);
				scaleY = std::stod(scaleYSTR);
				
				boost::filesystem::path inputPATH(inputSTR);
				
				boost::filesystem::path outputPATH = currentPATH / "output" / inputPATH.filename().stem();
				
				int transparency = 0;
				if(IsDlgButtonChecked(hWnd,IDC_TRANSPARENCYCHECKBOX))
				{
					transparency = 1;
				}

				int color_mode = std::stod(colormodeSTR);
				
				boost::filesystem::create_directory((currentPATH / "output").string());
				
				boost::timer::cpu_timer timer;
				
				if(boost::filesystem::exists(inputPATH.string()))
				{
					//1 in color thing means lab instead of rgb for closest color
					//0 fformat to always save as png
					if(color_pallete.size()==0)
					{
						MessageBox(NULL,"empty pallete","exception",0);
						return 0;
					}
					if(IsDlgButtonChecked(hWnd,IDC_OPTIMIZECHECKBOX))
					{
						float multiplyScale = SendMessage(hHueTrack,TBM_GETPOS,0,0)/100.0;

						std::string convertedInputString = (currentPATH / "output" / inputPATH.filename().stem()).string()+"CONVERTED.png";
						std::array<float, 3> avgColor = {0,0,0};
						std::array<float, 3> avgImageColor = {0,0,0};
						std::array<float, 3> colorMultiply = {0,0,0};
						float colorAmount = color_pallete.size();
						for(int i = 0; i < colorAmount; i++)
						{
							avgColor[0]+=color_pallete[i][0]/colorAmount;
							avgColor[1]+=color_pallete[i][1]/colorAmount;
							avgColor[2]+=color_pallete[i][2]/colorAmount;
						}

						int width, height, bpp;
						int widthR, heightR;

						int colors = 3;
						colors += transparency;

						unsigned char *image_unres = stbi_load(inputPATH.string().c_str(), &width, &height, &bpp, 0);

						size_t image_unres_size = width * height * bpp;

						widthR = round((float)width * scaleX);
						heightR = round((float)height * scaleY);

						size_t image_size = widthR * heightR * bpp;

						unsigned char *image = (unsigned char*)malloc(image_size);

						stbir_resize_uint8(image_unres, width, height, 0, image, widthR, heightR, 0, bpp);

						width = widthR;
						height = heightR;

						size_t output_size = width * height * colors;

						size_t pixel_amount = width*height;

						unsigned char *output_image = (unsigned char*)malloc(output_size);

						for(unsigned char *p = image; p != image+image_size; p += bpp)
						{
							float t = 1;

							if(bpp==4)
							{
								t = (*(p+3))/255.0;
							}

							avgImageColor[0]+=(*p)*t;
							avgImageColor[1]+=(*(p+1))*t;
							avgImageColor[2]+=(*(p+2))*t;
						}

						avgImageColor[0]/=pixel_amount;
						avgImageColor[1]/=pixel_amount;
						avgImageColor[2]/=pixel_amount;

						colorMultiply[0]=multiplyScale+(avgColor[0]/avgImageColor[0])*(1-multiplyScale);
						colorMultiply[1]=multiplyScale+(avgColor[1]/avgImageColor[1])*(1-multiplyScale);
						colorMultiply[2]=multiplyScale+(avgColor[2]/avgImageColor[2])*(1-multiplyScale);
						
						for(unsigned char *p = image, *po = output_image; p != image+image_size; p += bpp, po += colors)
						{
							int t = 1;

							int color[3];

							if(bpp==4)
							{
								t = (*(p+3))/255.0;
							}
							color[0] = std::round(((*p)*colorMultiply[0]) > 255 ? 255 : ((*p)*colorMultiply[0]))*t;
							color[1] = std::round(((*(p+1))*colorMultiply[1]) > 255 ? 255 : ((*(p+1))*colorMultiply[1]))*t;
							color[2] = std::round(((*(p+2))*colorMultiply[2]) > 255 ? 255 : ((*(p+2))*colorMultiply[2]))*t;

							*po = (uint8_t)color[0];
							*(po+1) = (uint8_t)color[1];
							*(po+2) = (uint8_t)color[2];
							if(transparency==1)
							{
								*(po+3) = *(p+3)==255?255:0;
							}
						}

						stbi_write_png(convertedInputString.c_str(), width, height, colors, output_image, width * colors);

						stbi_image_free(image_unres);
						
						dither_image(convertedInputString,(outputPATH.string()+".png"),color_pallete,1,1,color_mode,0,transparency);
					} else
					{
					dither_image(inputPATH.string(),(outputPATH.string()+".png"),color_pallete,scaleX,scaleY,color_mode,0,transparency);
					}
				} else
				{
					MessageBox(NULL,"input file doesn't exist","exception",0);
					return 0;
				}
				
				std::string finish_message;
				finish_message = "dithering complete in ";
				finish_message += timer.format(3,"%w");
				
				MessageBox(NULL,finish_message.c_str(),"status",0);
			}

			if(LOWORD(wParam)==IDC_TOTEXTMENU)
			{
				DisplayITTDialog(hWnd);
			}

			if(LOWORD(wParam)==IDC_SPLITCOLORMENU)
			{
				DisplaySplitDialog(hWnd);
			}
			
			if(LOWORD(wParam)==IDC_BWBRAILLEMENU)
			{
				DisplayBWBDialog(hWnd);
			}
			
			if(LOWORD(wParam)==IDC_TRANSPARENCYCHECKBOX)
			{
				if(IsDlgButtonChecked(hWnd,IDC_TRANSPARENCYCHECKBOX))
				{
					CheckDlgButton(hWnd,IDC_TRANSPARENCYCHECKBOX,BST_UNCHECKED);
				} else
				{
					CheckDlgButton(hWnd,IDC_TRANSPARENCYCHECKBOX,BST_CHECKED);
				}
			}

			if(LOWORD(wParam)==IDC_OPTIMIZECHECKBOX)
			{
				if(IsDlgButtonChecked(hWnd,IDC_OPTIMIZECHECKBOX))
				{
					CheckDlgButton(hWnd,IDC_OPTIMIZECHECKBOX,BST_UNCHECKED);
				} else
				{
					CheckDlgButton(hWnd,IDC_OPTIMIZECHECKBOX,BST_CHECKED);
				}
			}
			
			if(LOWORD(wParam)==IDC_COLORLIST)
			{
				selItem = (int)SendMessage(hColorList,LB_GETCURSEL,0,0);
				if(selItem == -1)
				{
					selItem = (int)SendMessage(hColorList,LB_GETCOUNT,0,0)-1;
				}
				if(HIWORD(wParam)==LBN_DBLCLK)
				{	
					std::array<int, 3> color = colorFromIndex(selItem,hColorList);

					if(AddColor(hWnd,RGB(color[0],color[1],color[2])))
					{
						color_pallete.erase(std::find(color_pallete.begin(),color_pallete.end(),color));
					}
				}
			}
			
			if(LOWORD(wParam)==IDC_COLORLOADBUTTON)
			{	
				char palleteSTR[1024];
				GetWindowText(hEditPalletePath,palleteSTR,1024);
				boost::filesystem::path palletePATH(palleteSTR);

				if(boost::filesystem::exists(palletePATH.string()))
				{
					color_pallete = GetPallete(palletePATH.string());
				} else
				{
					MessageBox(NULL,"pallete file doesn't exist","exception",0);
					return 0;
				}
				
				QuickSort(0, color_pallete.size()-1, color_pallete);
				UpdateColorList();
			}
			
			if(LOWORD(wParam)==IDC_BROWSEINPUTBUTTON)
			{
				OpenFile(hWnd,hEditInputPath);
			}
			
			if(LOWORD(wParam)==IDC_BROWSEPALLETEBUTTON)
			{
				OpenFile(hWnd,hEditPalletePath);
			}
			
			if(LOWORD(wParam)==IDC_COLORCLEARBUTTON)
			{
				color_pallete.clear();
				SendMessage(hColorList,LB_RESETCONTENT,0,0);
			}
			
			if(LOWORD(wParam)==IDC_ADDCOLORBUTTON)
			{
				AddColor(hWnd,RGB(0,0,0));
			}
			
			if(LOWORD(wParam)==IDC_DELETECOLORBUTTON)
			{
				if((int)SendMessage(hColorList,LB_GETCOUNT,0,0)>0)
				{
					selItem = (int)SendMessage(hColorList,LB_GETCURSEL,0,0);
					if(selItem==-1)
					{
						selItem = (int)SendMessage(hColorList,LB_GETCOUNT,0,0)-1;
					}
					
					std::array<int,3> color = colorFromIndex(selItem, hColorList);
					
					color_pallete.erase(std::find(color_pallete.begin(),color_pallete.end(),color));
					SendMessage(hColorList,LB_DELETESTRING,selItem,0);
				}
			}
			
			break;
			
		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
	return 0;
}

LRESULT CALLBACK SplitDialogPrc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CREATE:
			AddControlsSplit(hWnd);
			break;

		case WM_DRAWITEM:
			if(LOWORD(wParam)==IDC_SPLITCOLORLIST||LOWORD(wParam)==IDC_SPLITCOLORGROUPLIST)
			{
				DrawItemFunc(lParam);
			}
			if(LOWORD(wParam)==IDC_SELECTMERGECOLORBUTTON)
			{
				HFONT hFont, hOldFont;
				LPDRAWITEMSTRUCT lpdis = (LPDRAWITEMSTRUCT)lParam;

				RGB rgbColor;
				rgbColor.r = GetRValue(splitMergeColor);
				rgbColor.g = GetGValue(splitMergeColor);
				rgbColor.b = GetBValue(splitMergeColor);

				LAB colorLAB = XYZtoLAB(RGBtoXYZ(rgbColor));
				colorLAB.l *= 0.75;
				colorLAB.a *= 0.75;
				colorLAB.b *= 0.75;

				RGB rgbDarkColor = XYZtoRGB(LABtoXYZ(colorLAB));

				SetDCBrushColor(lpdis->hDC,splitMergeColor);
				SelectObject(lpdis->hDC,GetStockObject(DC_BRUSH));
				RoundRect(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.top,lpdis->rcItem.right,lpdis->rcItem.bottom,5,5);

				SetDCBrushColor(lpdis->hDC,RGB(rgbDarkColor.r,rgbDarkColor.g,rgbDarkColor.b));
				SetBkMode(lpdis->hDC, TRANSPARENT);
				hFont = (HFONT)GetStockObject(SYSTEM_FONT); 
				if (hOldFont = (HFONT)SelectObject(lpdis->hDC, hFont))
				{
					//last variable is string length
					TextOut(lpdis->hDC, 20, 5, "select merge color", 18); 
      
					SelectObject(lpdis->hDC, hOldFont); 
				}

				return true;
			}
			break;

		case WM_CLOSE:
			SetFocus(mainWindow);
			DestroyWindow(hWnd);
			break;

		case WM_COMMAND:
			if(LOWORD(wParam)==IDC_SPLITCOLORLIST)
			{
				selItemSplit = (int)SendMessage(hSplitColorList,LB_GETCURSEL,0,0);
				if(selItemSplit == -1)
				{
					selItemSplit = (int)SendMessage(hSplitColorList,LB_GETCOUNT,0,0)-1;
				}
			}
			if(LOWORD(wParam)==IDC_SPLITCOLORGROUPLIST)
			{
				selGroupItemSplit = (int)SendMessage(hSplitColorGroupList,LB_GETCURSEL,0,0);
				if(selGroupItemSplit == -1)
				{
					selGroupItemSplit = (int)SendMessage(hSplitColorGroupList,LB_GETCOUNT,0,0)-1;
				}
			}
			if(LOWORD(wParam)==IDC_ADDTOGROUPBUTTON)
			{
				if((int)SendMessage(hSplitColorList,LB_GETCOUNT,0,0)>0)
				{
					std::array<int,3> color = colorFromIndex(selItemSplit,hSplitColorList);
					split_color_group.push_back(color);

					std::string fullSTR;
					fullSTR += std::to_string(color[0]);
					fullSTR += " ";
					fullSTR += std::to_string(color[1]);
					fullSTR += " ";
					fullSTR += std::to_string(color[2]);

					int pos = 0;
					for(int i = 0; i < split_color_group.size(); i++)
					{
						if(!isBiggerThan(color,split_color_group[i]))
						{
							pos = i;
							break;
						}
					}

					SendMessage(hSplitColorGroupList, LB_INSERTSTRING, pos, (LPARAM)fullSTR.c_str());

					SendMessage(hSplitColorList,LB_DELETESTRING,selItemSplit,0);
					split_color_pallete.erase(std::find(split_color_pallete.begin(),split_color_pallete.end(),color));
				}
			}
			if(LOWORD(wParam)==IDC_REMOVEFROMGROUPBUTTON)
			{
				if((int)SendMessage(hSplitColorGroupList,LB_GETCOUNT,0,0)>0)
				{
					std::array<int,3> color = colorFromIndex(selGroupItemSplit,hSplitColorGroupList);
					split_color_pallete.push_back(color);

					std::string fullSTR;
					fullSTR += std::to_string(color[0]);
					fullSTR += " ";
					fullSTR += std::to_string(color[1]);
					fullSTR += " ";
					fullSTR += std::to_string(color[2]);

					int pos = 0;
					for(int i = 0; i < split_color_pallete.size(); i++)
					{
						if(!isBiggerThan(color,split_color_pallete[i]))
						{
							pos = i;
							break;
						}
					}

					SendMessage(hSplitColorList, LB_INSERTSTRING, pos, (LPARAM)fullSTR.c_str());

					SendMessage(hSplitColorGroupList,LB_DELETESTRING,selGroupItemSplit,0);
					split_color_group.erase(std::find(split_color_group.begin(),split_color_group.end(),color));
				}
			}
			if(LOWORD(wParam)==IDC_SPLITCOLORSBUTTON)
			{
				int width, height, bpp;

				std::array<int, 3> color = {GetRValue(splitMergeColor),GetGValue(splitMergeColor),GetBValue(splitMergeColor)};

				char imagepathSTR[1024];
				GetWindowText(hSplitInputPath,imagepathSTR,1024);
				boost::filesystem::path inputPATH(imagepathSTR);

				unsigned char *pixels = stbi_load(inputPATH.string().c_str(), &width, &height, &bpp, 0);

				size_t image_size = width * height * bpp;

				size_t output_size = width * height * 3;

				unsigned char *output_image = (unsigned char*)malloc(output_size);

				boost::filesystem::path outputPATH = currentPATH / "output" / inputPATH.filename().stem() / inputPATH.filename().stem();

				if(!boost::filesystem::exists(inputPATH.string()))
				{
					MessageBox(NULL,"input file doesn't exist","exception",0);
					return 0;
				}
				
				boost::filesystem::create_directory((currentPATH / "output").string());
				boost::filesystem::create_directory((currentPATH / "output" / inputPATH.filename().stem()).string());

				std::stringstream ss;
				ss << std::hex << color[0];
				if(ss.str().length()==1)
				{
					ss.seekp(0);
					ss << "0" << std::hex << color[0];
				}
				ss << std::hex << color[1];
				if(ss.str().length()==3)
				{
					ss.seekp(2);
					ss << "0" << std::hex << color[1];
				}
				ss << std::hex << color[2];
				if(ss.str().length()==5)
				{
					ss.seekp(4);
					ss << "0" << std::hex << color[2];
				}
				std::string filepath = (outputPATH.string()+ss.str()+".png");

				for(unsigned char *p = pixels, *po = output_image; p != pixels+image_size; p += bpp, po += 3)
				{
					int match = 0;

					for(int i = 0; i < split_color_group.size(); i++)
					{
						bool check = true;
						if(bpp==4)
						{
							if(*(p+3)!=255)
							{
								check = false;
							}
						}
						if(*p==split_color_group[i][0] && *(p+1)==split_color_group[i][1] && *(p+2)==split_color_group[i][2] && check)
						{
							match = 1;
						}
					}

					*po = (uint8_t)color[0]*match;
					*(po+1) = (uint8_t)color[1]*match;
					*(po+2) = (uint8_t)color[2]*match;
				}

				stbi_write_png(filepath.c_str(), width, height, 3, output_image, width*3);
				stbi_image_free(pixels);
			}
			if(LOWORD(wParam)==IDC_SELECTMERGECOLORBUTTON)
			{
				DWORD defaultColor = splitMergeColor;
				bool result = ColorDialogue(hWnd,&splitMergeColor);
				if(result==false)
				{
					splitMergeColor = defaultColor;
				}
				RedrawWindow(hSplitMergeButton,0,0,RDW_INVALIDATE);
			}
			if(LOWORD(wParam)==IDC_BROWSESPLITINPUTBUTTON)
			{
				OpenFile(hWnd,hSplitInputPath);
			}
			if(LOWORD(wParam)==IDC_LOADSPLITIMAGEBUTTON)
			{
				char imagepathSTR[1024];
				GetWindowText(hSplitInputPath,imagepathSTR,1024);
				boost::filesystem::path inputPATH(imagepathSTR);

				if(!boost::filesystem::exists(inputPATH.string()))
				{
					MessageBox(NULL,"input file doesn't exist","exception",0);
					return 0;
				}

				split_color_pallete = GetPallete(inputPATH.string());
				QuickSort(0,split_color_pallete.size()-1,split_color_pallete);
				split_color_group.clear();

				SendMessage(hSplitColorList,LB_RESETCONTENT,0,0);
				SendMessage(hSplitColorGroupList,LB_RESETCONTENT,0,0);
				for(int i = 0; i < split_color_pallete.size(); i++)
				{
					std::string fullSTR;
					fullSTR += std::to_string(split_color_pallete[i][0]);
					fullSTR += " ";
					fullSTR += std::to_string(split_color_pallete[i][1]);
					fullSTR += " ";
					fullSTR += std::to_string(split_color_pallete[i][2]);

					int pos = (int)SendMessage(hSplitColorList, LB_ADDSTRING, 0, (LPARAM)fullSTR.c_str());
				}
			}
		break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT CALLBACK DialogPrc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CREATE:
			AddControlsITT(hWnd);
			break;

		case WM_CLOSE:
			SetFocus(mainWindow);
			DestroyWindow(hWnd);
			break;
			
		case WM_DRAWITEM:
			DrawItemFunc(lParam);
			break;

		case WM_COMMAND:
			if(LOWORD(wParam)==IDC_BROWSEITTINPUTBUTTON)
			{
				OpenFile(hWnd,hITTInputPath);
			}
			if(LOWORD(wParam)==IDC_PIXELINFOLIST)
			{
				int TACSelItem = (int)SendMessage(hPixelInfoList,LB_GETCURSEL,0,0);
				if(TACSelItem ==-1)
				{
					TACSelItem = (int)SendMessage(hPixelInfoList,LB_GETCOUNT,0,0)-1;
				}
				if(HIWORD(wParam)==LBN_DBLCLK)
				{
					DisplayTACDialog(hWnd, TACSelItem);
				}
			}
			if(LOWORD(wParam)==IDC_CONVERTTOTEXTBUTTON)
			{
				int width, height, bpp;
				
				char imagepathSTR[1024];
				GetWindowText(hITTInputPath,imagepathSTR,1024);
				boost::filesystem::path inputPATH(imagepathSTR);
				
				if(!boost::filesystem::exists(inputPATH.string()))
				{
					MessageBox(NULL,"input file doesn't exist","exception",0);
					return 0;
				}
				
				unsigned char *pixels = stbi_load(inputPATH.string().c_str(), &width, &height, &bpp, 0);
				size_t image_size = width * height * bpp;

				boost::filesystem::path outputPATH = currentPATH / "output" / inputPATH.filename().stem();
				
				boost::filesystem::create_directory((currentPATH / "output").string());
				
				std::string filepath = (outputPATH.string()+".txt");
				
				std::wofstream filestream(filepath.c_str());
				
				filestream.imbue(std::locale(filestream.getloc(), new std::codecvt_utf8_utf16<wchar_t>));
				
				int eap = 0;
				for(unsigned char *p = pixels; p != pixels+image_size; p += bpp, eap += bpp)
				{
					int w = (eap/bpp)%width;
					
					for(int i = 0; i < color_stringPairs.size(); i++)
					{
						std::array<int,3> color = color_stringPairs[i].color;
						
						if((*p) == color[0] && (*(p+1)) == color[1] && (*(p+2)) == color[2])
						{
							filestream << color_stringPairs[i].text.c_str();
						}
					}
					
					if(w==(width-1))
					{
						filestream << "\n";
					}
				}
				
				filestream.close();
				stbi_image_free(pixels);
			}
			if(LOWORD(wParam)==IDC_LOADITTIMAGEBUTTON)
			{
				char imagepathSTR[1024];
				GetWindowText(hITTInputPath,imagepathSTR,1024);
				boost::filesystem::path inputPATH(imagepathSTR);

				if(!boost::filesystem::exists(inputPATH.string()))
				{
					MessageBox(NULL,"input file doesn't exist","exception",0);
					return 0;
				}
				
				color_stringPairs.clear();
				
				std::vector<std::array<int,3>> pallete = GetPallete(inputPATH.string());
				QuickSort(0,pallete.size()-1,pallete);
				
				for(int i = 0; i < pallete.size(); i++)
				{
					colorTextPair pair;
					pair.color = pallete[i];
					pair.text = L"";
					color_stringPairs.push_back(pair);
				}
				
				SendMessage(hPixelInfoList,LB_RESETCONTENT,0,0);
				for(int i = 0; i < color_stringPairs.size(); i++)
				{
					std::string fullSTR;
					fullSTR += std::to_string(color_stringPairs[i].color[0]);
					fullSTR += " ";
					fullSTR += std::to_string(color_stringPairs[i].color[1]);
					fullSTR += " ";
					fullSTR += std::to_string(color_stringPairs[i].color[2]);
					
					int pos = (int)SendMessage(hPixelInfoList, LB_ADDSTRING, 0, (LPARAM)fullSTR.c_str());
				}
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
	return 0;
}

LRESULT CALLBACK TacDialogPrc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	PAINTSTRUCT ps;
	HDC hdc;
	switch(message)
	{
		case WM_CREATE:
			AddControlsTAC(hWnd);
			break;

		case WM_CLOSE:
			SetFocus(hITTDialog);
			DestroyWindow(hWnd);
			break;
			
		case WM_PAINT:
		{
			hdc = BeginPaint(hWnd,&ps);
			
			SendMessage(hWnd,WM_ERASEBKGND,(WPARAM)hdc,0);
			SetBkColor(hdc,TACColor);
			
			HBRUSH brush = CreateSolidBrush(TACColor);
			
			RECT TACRect;
			TACRect.left = 0;
			TACRect.top = 0;
			TACRect.right = 140;
			TACRect.bottom = 125;
			
			FillRect(hdc,&TACRect,brush); 
			
			EndPaint(hWnd,&ps);
			break;
		}
		
		case WM_COMMAND:
			if(LOWORD(wParam)==IDC_ADDTACTEXTBUTTON)
			{
				wchar_t rstringSTR[32];
				GetWindowTextW(hTACString,rstringSTR,32);
				
				std::array<int,3> color = {
				GetRValue(TACColor),
				GetGValue(TACColor),
				GetBValue(TACColor)};
				
				for(int i = 0; i < color_stringPairs.size(); i++)
				{
					if(color_stringPairs[i].color==color)
					{
						color_stringPairs[i].text = rstringSTR;
					}
				}
				
				DestroyWindow(hWnd);
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
	return 0;
}

LRESULT CALLBACK BWBDialogPrc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
		case WM_CREATE:
			AddControlsBWB(hWnd);
			break;

		case WM_CLOSE:
			SetFocus(mainWindow);
			DestroyWindow(hWnd);
			break;
		
		case WM_COMMAND:
			if(LOWORD(wParam)==IDC_BROWSEBWBINPUTBUTTON)
			{
				OpenFile(hWnd,hBWBInputPath);
			}
			if(LOWORD(wParam)==IDC_BWBCONVERTBUTTON)
			{
				int width, height, bpp;
				
				char imagepathSTR[1024];
				GetWindowText(hBWBInputPath,imagepathSTR,1024);
				boost::filesystem::path inputPATH(imagepathSTR);

				if(!boost::filesystem::exists(inputPATH.string()))
				{
					MessageBox(NULL,"input file doesn't exist","exception",0);
					return 0;
				}
				
				unsigned char *pixels = stbi_load(inputPATH.string().c_str(), &width, &height, &bpp, 0);
				size_t image_size = width * height * bpp;

				boost::filesystem::path outputPATH = currentPATH / "output" / inputPATH.filename().stem();
				
				boost::filesystem::create_directory((currentPATH / "output").string());
				
				std::string filepath = (outputPATH.string()+".txt");
				
				std::wofstream filestream(filepath.c_str());
				
				filestream.imbue(std::locale(filestream.getloc(), new std::codecvt_utf8_utf16<wchar_t>));
				
				int remainderY = height % 4;
				int remainderX = width % 2;
				
				int yoff = remainderY>0?1:0;
				
				int smallImgSize = (height/4+yoff)*(width+remainderX)*3;
				
				//i say black and white to braille but it can take images of any color and just uses the first color channel and sees is it closer to black or white

				boost::timer::cpu_timer timer;

				int smallWidth = (width/2+remainderX);
				
				//MessageBox(NULL,("aaaaaaaaaaaaaaa: "+std::to_string((height))).c_str(),"B",0);
				
				int eap = 0;
				for(unsigned char *p = pixels; p != pixels+smallImgSize; p += bpp*2, eap += bpp*2)
				{
					int w = (eap/(bpp*2))%smallWidth;
					
					int h = (eap/(bpp*2))/smallWidth;
					
					std::bitset<8> blockNUM;

					int bitVals[8];

					bitVals[0] = (*(p+(smallWidth*2*h*3*bpp))>127?0:1);
					bitVals[3] = (*(p+bpp+(smallWidth*2*h*3*bpp))>127?0:1);

					if(eap/(smallWidth*3)<(height/2))
					{
						bitVals[1] = (*(p+(smallWidth*2*(h*3+1)*bpp))>127?0:1);
						bitVals[4] = (*(p+bpp+(smallWidth*2*(h*3+1)*bpp))>127?0:1);
					} else
					{
						bitVals[1] = 0;
						bitVals[4] = 0;
					}
						
					if(eap/(smallWidth*3)<(height/2-1))
					{
						bitVals[2] = (*(p+(smallWidth*2*(h*3+2)*bpp))>127?0:1);
						bitVals[5] = (*(p+bpp+(smallWidth*2*(h*3+2)*bpp))>127?0:1);
					} else
					{
						bitVals[2] = 0;
						bitVals[5] = 0;
					}
						
					if(eap/(smallWidth*3)<(height/2-2))
					{
						bitVals[6] = (*(p+(smallWidth*2*(h*3+3)*bpp))>127?0:1);
						bitVals[7] = (*(p+bpp+(smallWidth*2*(h*3+3)*bpp))>127?0:1);
					} else
					{
						bitVals[6] = 0;
						bitVals[7] = 0;
					}
					
					blockNUM.set(0,bitVals[0]);
					blockNUM.set(1,bitVals[1]);
					blockNUM.set(2,bitVals[2]);
					blockNUM.set(3,bitVals[3]);
					blockNUM.set(4,bitVals[4]);
					blockNUM.set(5,bitVals[5]);
					blockNUM.set(6,bitVals[6]);
					blockNUM.set(7,bitVals[7]);
					
					filestream << (wchar_t)(0x2800+blockNUM.to_ulong());
					
					//MessageBox(NULL,(std::to_string(w)+std::to_string((width/2+remainderX)-1)).c_str(),"a",0);
					if(w==(smallWidth-1))
					{
						//MessageBox(NULL,"aaaaaaaaaaaaaaaaaaaa","B",0);
						filestream << "\n";
					}
				}
				
				filestream.close();
				stbi_image_free(pixels);
				
				std::string finish_message;
				finish_message = "conversion complete in ";
				finish_message += timer.format(3,"%w");
				
				MessageBox(NULL,finish_message.c_str(),"status",0);
				
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
	return 0;
}

LRESULT CALLBACK EditPrc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	if(message==WM_CHAR&&wParam==1)
	{
		SendMessage(hWnd,EM_SETSEL,0,-1);
		return 0;
	}
	
	return CallWindowProc(WPA,hWnd,message,wParam,lParam);;
}

void AddControls(HWND hWnd)
{
	hMenu = CreateMenu();
	
	AppendMenu(hMenu,MF_STRING,IDC_TOTEXTMENU,"ITT");
	AppendMenu(hMenu,MF_STRING,IDC_SPLITCOLORMENU,"Split colors");
	AppendMenu(hMenu,MF_STRING,IDC_BWBRAILLEMENU,"BW to braille");
	
	SetMenu(hWnd, hMenu);
	
	CreateWindow("static","path to input image",WS_VISIBLE|WS_CHILD|ES_CENTER,15,15,150,25,hWnd,NULL,NULL,NULL);
	hEditInputPath = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","c:/sample/path/image.png",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,40,40,125,25,hWnd,NULL,NULL,NULL);
	WPA = (WNDPROC)SetWindowLongPtr(hEditInputPath,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	CreateWindow("button","...",WS_VISIBLE|WS_CHILD,15,40,25,25,hWnd,(HMENU)IDC_BROWSEINPUTBUTTON,NULL,NULL);
	
	CreateWindow("static","path to pallete image",WS_VISIBLE|WS_CHILD|ES_CENTER,175,15,150,25,hWnd,NULL,NULL,NULL);
	hEditPalletePath = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","c:/sample/path/pallete.png",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,200,40,125,25,hWnd,NULL,NULL,NULL);
	SetWindowLongPtr(hEditPalletePath,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	CreateWindow("button","...",WS_VISIBLE|WS_CHILD,175,40,25,25,hWnd,(HMENU)IDC_BROWSEPALLETEBUTTON,NULL,NULL);
	
	CreateWindow("static","size multipliers",WS_VISIBLE|WS_CHILD|ES_CENTER,15,70,150,25,hWnd,NULL,NULL,NULL);
	
	CreateWindow("static","X",WS_VISIBLE|WS_CHILD|ES_CENTER,15,90,50,25,hWnd,NULL,NULL,NULL);
	hEditScaleX = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","1.0",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,15,115,50,25,hWnd,NULL,NULL,NULL);
	SetWindowLongPtr(hEditScaleX,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	
	CreateWindow("static","Y",WS_VISIBLE|WS_CHILD|ES_CENTER,115,90,50,25,hWnd,NULL,NULL,NULL);
	hEditScaleY = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","1.0",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,115,115,50,25,hWnd,NULL,NULL,NULL);
	SetWindowLongPtr(hEditScaleY,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	
	CreateWindow("button","optimize image hue",WS_VISIBLE|WS_CHILD|BS_CHECKBOX,15,140,150,25,hWnd,(HMENU)IDC_OPTIMIZECHECKBOX,NULL,NULL);

	hHueTrack = CreateWindowEx(0,TRACKBAR_CLASS,"hue trackbar",WS_CHILD|WS_VISIBLE,15,165,150,25,hWnd,NULL,NULL,NULL);
	SendMessage(hHueTrack,TBM_SETRANGE,(WPARAM)TRUE,(LPARAM) MAKELONG(0, 100));

	CreateWindow("button","transparency",WS_VISIBLE|WS_CHILD|BS_CHECKBOX,15,195,150,25,hWnd,(HMENU)IDC_TRANSPARENCYCHECKBOX,NULL,NULL);
	
	CreateWindow("static","color mode",WS_VISIBLE|WS_CHILD|ES_CENTER,65,220,100,25,hWnd,NULL,NULL,NULL);
	hEditColorMode = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","1",WS_VISIBLE|WS_CHILD,15,220,50,25,hWnd,NULL,NULL,NULL);
	
	CreateWindow("button","load",WS_VISIBLE|WS_CHILD,175,70,65,25,hWnd,(HMENU)IDC_COLORLOADBUTTON,NULL,NULL);
	CreateWindow("button","clear",WS_VISIBLE|WS_CHILD,260,70,65,25,hWnd,(HMENU)IDC_COLORCLEARBUTTON,NULL,NULL);
	
	hColorList = CreateWindowEx(WS_EX_CLIENTEDGE,"listbox",NULL,WS_VISIBLE|WS_CHILD|WS_VSCROLL|LBS_DISABLENOSCROLL|LBS_HASSTRINGS|LBS_OWNERDRAWFIXED|LBS_NOTIFY,175,105,150,165,hWnd,(HMENU)IDC_COLORLIST,NULL,NULL);
	CreateWindow("button","add",WS_VISIBLE|WS_CHILD,175,270,75,20,hWnd,(HMENU)IDC_ADDCOLORBUTTON,NULL,NULL);
	CreateWindow("button","delete",WS_VISIBLE|WS_CHILD,250,270,75,20,hWnd,(HMENU)IDC_DELETECOLORBUTTON,NULL,NULL);
	
	CreateWindow("button","convert",WS_VISIBLE|WS_CHILD|WS_BORDER,15,255,150,35,hWnd,(HMENU)IDC_CONVERTBUTTON,NULL,NULL);
}

void AddControlsITT(HWND hWnd)
{
	CreateWindow("static","path to input image",WS_VISIBLE|WS_CHILD|ES_CENTER,15,15,150,25,hWnd,NULL,NULL,NULL);
	hITTInputPath = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","c:/sample/path/image.png",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,40,40,125,25,hWnd,NULL,NULL,NULL);
	(WNDPROC)SetWindowLongPtr(hITTInputPath,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	CreateWindow("button","...",WS_VISIBLE|WS_CHILD,15,40,25,25,hWnd,(HMENU)IDC_BROWSEITTINPUTBUTTON,NULL,NULL);

	hPixelInfoList = CreateWindowEx(WS_EX_CLIENTEDGE,"listbox",NULL,WS_VISIBLE|WS_CHILD|WS_VSCROLL|LBS_DISABLENOSCROLL|LBS_HASSTRINGS|LBS_OWNERDRAWFIXED|LBS_NOTIFY,175,15,170,135,hWnd,(HMENU)IDC_PIXELINFOLIST,NULL,NULL);

	CreateWindow("button","load",WS_VISIBLE|WS_CHILD,15,75,150,25,hWnd,(HMENU)IDC_LOADITTIMAGEBUTTON,NULL,NULL);

	CreateWindow("button","convert",WS_VISIBLE|WS_CHILD|WS_BORDER,15,110,150,35,hWnd,(HMENU)IDC_CONVERTTOTEXTBUTTON,NULL,NULL);
}

void AddControlsSplit(HWND hWnd)
{
	CreateWindow("static","path to input image",WS_VISIBLE|WS_CHILD|ES_CENTER,15,15,150,25,hWnd,NULL,NULL,NULL);
	hSplitInputPath = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","c:/sample/path/image.png",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,40,40,125,25,hWnd,NULL,NULL,NULL);
	(WNDPROC)SetWindowLongPtr(hSplitInputPath,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	CreateWindow("button","...",WS_VISIBLE|WS_CHILD,15,40,25,25,hWnd,(HMENU)IDC_BROWSESPLITINPUTBUTTON,NULL,NULL);

	hSplitColorList = CreateWindowEx(WS_EX_CLIENTEDGE,"listbox",NULL,WS_VISIBLE|WS_CHILD|WS_VSCROLL|LBS_DISABLENOSCROLL|LBS_HASSTRINGS|LBS_OWNERDRAWFIXED|LBS_NOTIFY,175,15,170,135,hWnd,(HMENU)IDC_SPLITCOLORLIST,NULL,NULL);

	CreateWindow("button","->",WS_VISIBLE|WS_CHILD,350,55,25,25,hWnd,(HMENU)IDC_ADDTOGROUPBUTTON,NULL,NULL);
	CreateWindow("button","<-",WS_VISIBLE|WS_CHILD,350,85,25,25,hWnd,(HMENU)IDC_REMOVEFROMGROUPBUTTON,NULL,NULL);

	hSplitColorGroupList = CreateWindowEx(WS_EX_CLIENTEDGE,"listbox",NULL,WS_VISIBLE|WS_CHILD|WS_VSCROLL|LBS_DISABLENOSCROLL|LBS_HASSTRINGS|LBS_OWNERDRAWFIXED|LBS_NOTIFY,380,15,170,110,hWnd,(HMENU)IDC_SPLITCOLORGROUPLIST,NULL,NULL);

	splitMergeColor = RGB(255,255,255);

	hSplitMergeButton = CreateWindow("button","",WS_VISIBLE|BS_OWNERDRAW|WS_CHILD,380,115,170,30,hWnd,(HMENU)IDC_SELECTMERGECOLORBUTTON,NULL,NULL);

	CreateWindow("button","load",WS_VISIBLE|WS_CHILD,15,75,150,25,hWnd,(HMENU)IDC_LOADSPLITIMAGEBUTTON,NULL,NULL);

	CreateWindow("button","convert",WS_VISIBLE|WS_CHILD|WS_BORDER,15,110,150,35,hWnd,(HMENU)IDC_SPLITCOLORSBUTTON,NULL,NULL);
}

void AddControlsBWB(HWND hWnd)
{
	CreateWindow("static","path to input image",WS_VISIBLE|WS_CHILD|ES_CENTER,15,15,150,25,hWnd,NULL,NULL,NULL);
	hBWBInputPath = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","c:/sample/path/image.png",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,40,40,125,25,hWnd,NULL,NULL,NULL);
	(WNDPROC)SetWindowLongPtr(hBWBInputPath,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	CreateWindow("button","...",WS_VISIBLE|WS_CHILD,15,40,25,25,hWnd,(HMENU)IDC_BROWSEBWBINPUTBUTTON,NULL,NULL);

	CreateWindow("button","convert",WS_VISIBLE|WS_CHILD|WS_BORDER,15,75,150,35,hWnd,(HMENU)IDC_BWBCONVERTBUTTON,NULL,NULL);
}

void AddControlsTAC(HWND hWnd)
{
	std::array<int,3> color = colorFromIndex(TACSelItem, hPixelInfoList);

	int prevIndex;

	for(int i = 0; i < color_stringPairs.size(); i++)
	{
		if(color_stringPairs[i].color==color)
		{
			prevIndex = i;
		}
	}

	TACColor = RGB(color[0],color[1],color[2]);

	std::wstring previousValue = color_stringPairs[prevIndex].text;

	hTACString = CreateWindowExW(WS_EX_CLIENTEDGE,L"edit",previousValue.c_str(),WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,5,10,125,30,hWnd,NULL,NULL,NULL);
	(WNDPROC)SetWindowLongPtr(hTACString,GWL_WNDPROC,(LONG_PTR)&EditPrc);

	CreateWindowW(L"button",L"set",WS_VISIBLE|WS_CHILD,5,60,125,30,hWnd,(HMENU)IDC_ADDTACTEXTBUTTON,NULL,NULL);
}

bool RegisterITTDialog(HINSTANCE hInstance)
{
	WNDCLASS wc = {};

	wc.lpfnWndProc    = DialogPrc;
	wc.hInstance      = hInstance;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName  = "MyDialogClass";
	
	if(!RegisterClass(&wc))
	{
		MessageBox(NULL,"call to registerclass failed","exception",0);
		return false;
	}
	return true;
}

bool RegisterTACDialog(HINSTANCE hInstance)
{
	WNDCLASS wc = {};

	wc.lpfnWndProc    = TacDialogPrc;
	wc.hInstance      = hInstance;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName  = "MyTacDialogClass";
	
	if(!RegisterClass(&wc))
	{
		MessageBox(NULL,"call to registerclass failed","exception",0);
		return false;
	}
	return true;
}

bool RegisterSplitDialog(HINSTANCE hInstance)
{
	WNDCLASS wc = {};

	wc.lpfnWndProc    = SplitDialogPrc;
	wc.hInstance      = hInstance;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName  = "MySplitDialogClass";

	if(!RegisterClass(&wc))
	{
		MessageBox(NULL,"call to registerclass failed","exception",0);
		return false;
	}
	return true;
}

bool RegisterBWBDialog(HINSTANCE hInstance)
{
	WNDCLASS wc = {};

	wc.lpfnWndProc    = BWBDialogPrc;
	wc.hInstance      = hInstance;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName  = "MyBWBDialogClass";

	if(!RegisterClass(&wc))
	{
		MessageBox(NULL,"call to registerclass failed","exception",0);
		return false;
	}
	return true;
}

void OpenFile(HWND hWnd, HWND textBox)
{
	OPENFILENAME ofn;
	
	char filename[1024];
	
	ZeroMemory(&ofn,sizeof(OPENFILENAME));
	
	ofn.lStructSize = sizeof(OPENFILENAME);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = filename;
	ofn.lpstrFile[0] = '\0';
	ofn.nMaxFile = 1024;
	ofn.lpstrFilter = "all files\0*.*\0image files\0*.png;*.jpg;*.jpeg;*.bmp;*.hdr;*.psd;*.tga;*.gif;*.pic\0";
	ofn.nFilterIndex = 1;
	
	GetOpenFileName(&ofn);
	
	SetWindowText(textBox,filename);
}

bool ColorDialogue(HWND hWnd, DWORD* returnColor)
{
	CHOOSECOLOR cc;

	static COLORREF custClrs[16];

	ZeroMemory(&cc,sizeof(CHOOSECOLOR));

	cc.lStructSize = sizeof(CHOOSECOLOR);
	cc.hwndOwner = hWnd;
	cc.lpCustColors = custClrs;
	cc.rgbResult = *returnColor;
	cc.Flags = CC_FULLOPEN | CC_RGBINIT;

	if(!ChooseColor(&cc))
	{
		return false;
	}

	*returnColor = cc.rgbResult;

	return true;
}

bool AddColor(HWND hWnd, DWORD defColor)
{
	DWORD colorCR;
	colorCR = defColor;

	bool result = ColorDialogue(hWnd,&colorCR);
	if(result==false)
	{
		return false;
	}
	
	std::array<int,3> color = {GetRValue(colorCR),GetGValue(colorCR),GetBValue(colorCR)};
	
	std::vector<std::array<int,3>>::iterator position = std::find(color_pallete.begin(),color_pallete.end(),color);
	
	if(position==color_pallete.end())
	{
		int pos = 0;
		for(int i = 0; i < color_pallete.size(); i++)
		{
			if(!isBiggerThan(color,color_pallete[i]))
			{
				pos = i;
				break;
			}
		}

		std::string fullSTR;
		fullSTR += std::to_string(color[0]);
		fullSTR += " ";
		fullSTR += std::to_string(color[1]);
		fullSTR += " ";
		fullSTR += std::to_string(color[2]);

		SendMessage(hColorList, LB_INSERTSTRING, pos, (LPARAM)fullSTR.c_str());
		color_pallete.insert(color_pallete.begin()+pos,color);

		return true;
	}
	return false;
}

void DisplayITTDialog(HWND hWnd)
{
	if(!IsWindow(hITTDialog))
	{
		POINT p;
		GetCursorPos(&p);
		hITTDialog = CreateWindow("MyDialogClass","image to text",WS_OVERLAPPED|WS_VISIBLE|WS_SYSMENU,p.x-200/2,p.y-100/2,360,185,hWnd,NULL,NULL,NULL);
	}
}

void DisplayTACDialog(HWND hWnd, int itemIndex)
{
	if(!IsWindow(hTACDialog))
	{
		TACSelItem = itemIndex;
		POINT p;
		GetCursorPos(&p);
		hTACDialog = CreateWindowW(L"MyTacDialogClass",L"symbol",WS_VISIBLE|WS_OVERLAPPED|WS_SYSMENU,p.x-200/2,p.y-100/2,140,125,hWnd,NULL,NULL,NULL);
	}	
}

void DisplaySplitDialog(HWND hWnd)
{
	if(!IsWindow(hSplitDialog))
	{
		POINT p;
		GetCursorPos(&p);

		hSplitDialog = CreateWindow("MySplitDialogClass","split image by colors",WS_VISIBLE|WS_OVERLAPPED|WS_SYSMENU,p.x-200/2,p.y-100/2,570,185,hWnd,NULL,NULL,NULL);
	}
}

void DisplayBWBDialog(HWND hWnd)
{
	if(!IsWindow(hBWBDialog))
	{
		POINT p;
		GetCursorPos(&p);

		hBWBDialog = CreateWindow("MyBWBDialogClass","convert black white img to braille",WS_VISIBLE|WS_OVERLAPPED|WS_SYSMENU,p.x-200/2,p.y-100/2,190,145,hWnd,NULL,NULL,NULL);
	}
}