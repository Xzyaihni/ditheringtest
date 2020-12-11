#include <iostream>
#include <vector>
#include <cmath>
#include <string>
#include <exception>
#include <algorithm>
#include <locale>
#include <codecvt>
#include <iomanip>
#include <sstream>

#include <io.h>
#include <fcntl.h>
#include <windows.h>
#include <commctrl.h>
#include <resource.h>

#include "dither.h"
#include "image_pallete.h"
#include "color_spaces.h"
#include "to_braille.h"

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
void AddControlsSize(HWND);
bool RegisterITTDialog(HINSTANCE);
void DisplayITTDialog(HWND);
bool RegisterSplitDialog(HINSTANCE);
void DisplaySplitDialog(HWND);
bool RegisterTACDialog(HINSTANCE);
void DisplayTACDialog(HWND,int);
bool RegisterBWBDialog(HINSTANCE);
void DisplayBWBDialog(HWND);
bool RegisterSizeDialog(HINSTANCE);
void DisplaySizeDialog(HWND);
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
HWND hITTAutoColorsEdit;

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

HWND hSizeInputPath;
HWND hSizeDialog;
HWND hSizeConvertButton;
HWND hSizeComboBox;

HWND hSizeXText;
HWND hSizeYText;
HWND hSizeXEdit;
HWND hSizeYEdit;
HWND hSizeExactText;
HWND hSizeTotalEdit;
HWND hSizeTotalText;
HWND hSizeTotalAspect;

HWND hTACString;

HWND hTACDialog;
COLORREF TACColor;

COLORREF splitMergeColor;

int selItem;
int TACSelItem;
int selItemSplit;
int selGroupItemSplit;

boost::filesystem::path currentPATH;

std::vector<std::array<int,3>> color_pallete;

std::vector<std::array<int,3>> split_color_pallete;
std::vector<std::array<int,3>> split_color_group;

float targetX = 1.0;
float targetY = 1.0;

int resizeMode = 0;

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
	if(end-start>1)
	{
		int i = start-1;
		for(int j = start; j < end; j++)
		{
			if(!isBiggerThan(colorList[j], colorList[end]))
			{
				i++;
				std::iter_swap(colorList.begin()+i, colorList.begin()+j);
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
	for(unsigned int i = 0; i < color_pallete.size(); i++)
	{
		std::string fullSTR;
		fullSTR += std::to_string(color_pallete[i][0]);
		fullSTR += " ";
		fullSTR += std::to_string(color_pallete[i][1]);
		fullSTR += " ";
		fullSTR += std::to_string(color_pallete[i][2]);
		
		SendMessage(hColorList,LB_ADDSTRING,0,(LPARAM)fullSTR.c_str());
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

	cRGB rgbColor;
	rgbColor.r = color[0];
	rgbColor.g = color[1];
	rgbColor.b = color[2];

	cLAB colorLAB = rgbColor.toXYZ().toLAB();

	cRGB rgbColorInv = colorLAB.toXYZ().toRGB();

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

	if(!RegisterSizeDialog(hInstance))
	{
		MessageBox(NULL,"registering size dialog class failed","exception",0);

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
				char inputSTR[1024];

				char colormodeSTR[1024];
				
				GetWindowText(hEditInputPath,inputSTR,1024);
				GetWindowText(hEditColorMode,colormodeSTR,1024);
				
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

					int newWidth;
					int newHeight;

					int width, height;
					stbi_load(inputPATH.string().c_str(), &width, &height, NULL, 0);

					if(resizeMode==0)
					{
						newWidth = round(width*targetX);
						newHeight = round(height*targetY);
					}

					if(resizeMode==1)
					{
						if(targetX==-1||targetY==-1)
						{
							if(targetX==targetY)
							{
								newWidth = width;
								newHeight = height;
							}
							float ratio;
							if(targetY==-1)
							{
								ratio = (float)width/height;
								newWidth = targetX;
								newHeight = (targetX/ratio);
							}
							if(targetX==-1)
							{
								ratio = (float)height/width;
								newHeight = targetY;
								newWidth = (targetY/ratio);
							}
						} else
						{
							newWidth = targetX;
							newHeight = targetY;
						}
					}

					if(resizeMode==2)
					{
						float mul = std::sqrt(targetX/(width*height));
						if(targetY<1)
						{
							newWidth = width*(mul*(1/targetY));
							newHeight = height*(mul*(targetY));
						} else
						{
							float newTarget = 1-(targetY-1);
							newWidth = width*(mul*(newTarget));
							newHeight = height*(mul*(1/newTarget));
						}
					}

					if(resizeMode==3)
					{
						int b = 2*width;
						long a = width*height;
						double discriminant = std::sqrt(std::pow(b,2)-4*a*(-targetX+2));
						float x2 = (float)(b-discriminant)/(2*a);
						newWidth = width*(std::abs(x2));
						newHeight = height*(std::abs(x2));
					}

					

					if(resizeMode==2 && (newWidth * newHeight)>targetX)
					{
						if(newWidth>newHeight)
						{
							newWidth--;
						} else
						{
							newHeight--;
						}
					}

					if(IsDlgButtonChecked(hWnd,IDC_OPTIMIZECHECKBOX))
					{
						float multiplyScale = SendMessage(hHueTrack,TBM_GETPOS,0,0)/100.0;

						std::array<float, 3> avgColor = {0,0,0};
						std::array<float, 3> avgImageColor = {0,0,0};
						std::array<float, 3> colorMultiply = {0,0,0};
						float colorAmount = color_pallete.size();
						for(int i = 0; i < colorAmount; i++)
						{
							for(int k = 0; k < 3; k++)
							{
								avgColor[k]+=color_pallete[i][k]/colorAmount;
							}
						}

						int width,height,bpp;

						unsigned char *image_unres = stbi_load(inputPATH.string().c_str(), &width, &height, &bpp, 0);

						size_t image_size = newWidth * newHeight * bpp;

						unsigned char *image = (unsigned char*)malloc(image_size);

						stbir_resize_uint8(image_unres, width, height, 0, image, newWidth, newHeight, 0, bpp);

						width = newWidth;
						height = newHeight;

						size_t output_size = width * height * bpp;

						size_t pixel_amount = width*height;

						unsigned char *output_image = (unsigned char*)malloc(output_size);

						for(unsigned char *p = image; p != image+image_size; p += bpp)
						{
							int t = 1;

							if(bpp==4)
							{
								t = (*(p+3))/255.0;
							}
							
							if(color_mode==0)
							{
								avgImageColor[0] += (*p)*t;
								avgImageColor[1] += (*(p+1))*t;
								avgImageColor[2] += (*(p+2))*t;
							} else
							{
								float LABColors[3] = {(*p)*t,(*(p+1))*t,(*(p+2))*t};
								for(int k = 0; k < 3; k++)
								{
									LABColors[k] /= 255;
									
									if(LABColors[k] > 0.04045f)
									{
										float x = (LABColors[k]+0.055f)/1.055f;
										LABColors[k] = (0x1.117542p-12 + x * (-0x5.91e6ap-8 + x * (0x8.0f50ep-4 + x * (0xa.aa231p-4 + x * (-0x2.62787p-4)))));//std::pow((LABColors[k]+0.055f)/1.055f,2.4f);
									}
									else
									{
										LABColors[k] /= 12.92f;
									}
									
									LABColors[k] *= 100;
								}
								
								LABColors[0] = LABColors[0]*0.4124f+LABColors[1]*0.3576f+LABColors[2]*0.1805f;
								LABColors[1] = LABColors[0]*0.2126f+LABColors[1]*0.7152f+LABColors[2]*0.0722f;
								LABColors[2] = LABColors[0]*0.0193f+LABColors[1]*0.1192f+LABColors[2]*0.9505f;
								
								LABColors[0] /= 95.047f;
								LABColors[1] /= 100.0f;
								LABColors[2] /= 108.883f;
								
								for(int k = 0; k < 3; k++)
								{
									if (LABColors[k] > 0.008856)
									{
										LABColors[k] = std::cbrt(LABColors[k]);//std::pow(LABColors[j+3*k],1.0/3.0);
									}
									else
									{
										LABColors[k] = (7.787f * LABColors[k]) + (16.0/116.0);
									}
								}
								
								avgImageColor[0] += (116.0 * LABColors[1]) - 16;
								avgImageColor[1] += 500.0 * (LABColors[0]-LABColors[1]);
								avgImageColor[2] += 200.0 * (LABColors[1]-LABColors[2]);
							}
						}


						for(int i = 0; i < 3; i++)
						{
							avgImageColor[i]/=pixel_amount;
						}
						
						if(color_mode==1)
						{
							cRGB ccolor;
							ccolor = cLAB(avgImageColor[0],avgImageColor[1],avgImageColor[2]).toXYZ().toRGB();
							avgImageColor[0] = ccolor.r;
							avgImageColor[1] = ccolor.g;
							avgImageColor[2] = ccolor.b;
						}
						
						for(int i =0; i < 3; i++)
						{
							colorMultiply[i]=multiplyScale+(avgColor[i]/avgImageColor[i])*(1-multiplyScale);
						}
						for(unsigned char *p = image, *po = output_image; p != image+image_size; p += bpp, po += bpp)
						{
							int t = 1;

							int color[3];

							for(int i = 0; i < 3; i++)
							{
								color[i] = std::round(((*(p+i))*colorMultiply[i]) > 255 ? 255 : ((*(p+i))*colorMultiply[i]));
	
								*(po+i) = (uint8_t)color[i];
							}
							if(bpp==4)
							{
								*(po+3) = *(p+3);
							}
						}



						stbi_image_free(image_unres);
						unsigned char *save_image = dither_image(output_image,width,height,bpp,color_pallete,newWidth,newHeight,color_mode,transparency);
						stbi_write_png((outputPATH.string()+".png").c_str(), newWidth, newHeight, 3+transparency, save_image, newWidth * (3+transparency));

						delete[] save_image;
					} else
					{
						dither_image(inputPATH.string(),(outputPATH.string()+".png"),color_pallete,newWidth,newHeight,color_mode,transparency);
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
			
			if(LOWORD(wParam)==IDC_SIZECHANGEBUTTON)
			{
				DisplaySizeDialog(hWnd);
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

				cRGB rgbColor;
				rgbColor.r = GetRValue(splitMergeColor);
				rgbColor.g = GetGValue(splitMergeColor);
				rgbColor.b = GetBValue(splitMergeColor);

				cLAB colorLAB = rgbColor.toXYZ().toLAB();
				colorLAB.l *= 0.75;
				colorLAB.a *= 0.75;
				colorLAB.b *= 0.75;

				cRGB rgbDarkColor = colorLAB.toXYZ().toRGB();

				SetDCBrushColor(lpdis->hDC,splitMergeColor);
				SelectObject(lpdis->hDC,GetStockObject(DC_BRUSH));
				RoundRect(lpdis->hDC,lpdis->rcItem.left,lpdis->rcItem.top,lpdis->rcItem.right,lpdis->rcItem.bottom,5,5);

				SetDCBrushColor(lpdis->hDC,RGB(rgbDarkColor.r,rgbDarkColor.g,rgbDarkColor.b));
				SetBkMode(lpdis->hDC, TRANSPARENT);
				hFont = (HFONT)GetStockObject(SYSTEM_FONT);
				hOldFont = (HFONT)SelectObject(lpdis->hDC, hFont);
				if (hOldFont)
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
					for(unsigned int i = 0; i < split_color_group.size(); i++)
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
					for(unsigned int i = 0; i < split_color_pallete.size(); i++)
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

					for(unsigned int i = 0; i < split_color_group.size(); i++)
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
				for(unsigned int i = 0; i < split_color_pallete.size(); i++)
				{
					std::string fullSTR;
					fullSTR += std::to_string(split_color_pallete[i][0]);
					fullSTR += " ";
					fullSTR += std::to_string(split_color_pallete[i][1]);
					fullSTR += " ";
					fullSTR += std::to_string(split_color_pallete[i][2]);
					SendMessage(hSplitColorList,LB_ADDSTRING,0,(LPARAM)fullSTR.c_str());
				}
			}
		break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	return 0;
}

LRESULT LOADIMAGEITT(LPARAM lParam)
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
	
	for(unsigned int i = 0; i < pallete.size(); i++)
	{			
		colorTextPair pair;
		pair.color = pallete[i];
		pair.text = L"";
		color_stringPairs.push_back(pair);
	}
	
	SendMessage(hPixelInfoList,LB_RESETCONTENT,0,0);
	for(unsigned int i = 0; i < color_stringPairs.size(); i++)
	{
		std::string fullSTR;
		fullSTR += std::to_string(color_stringPairs[i].color[0]);
		fullSTR += " ";
		fullSTR += std::to_string(color_stringPairs[i].color[1]);
		fullSTR += " ";
		fullSTR += std::to_string(color_stringPairs[i].color[2]);
		SendMessage(hPixelInfoList,LB_ADDSTRING,0,(LPARAM)fullSTR.c_str());
	}
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
				//you might run out of character space with this thing but who cares
				//change both the 4096's to something bigger if you care
				char autocolorsCSTR[4096];
				GetWindowText(hITTInputPath,imagepathSTR,1024);
				GetWindowText(hITTAutoColorsEdit,autocolorsCSTR,4096);
				boost::filesystem::path inputPATH(imagepathSTR);
				
				std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
				std::wstring autocolorsSTR = converter.from_bytes(autocolorsCSTR);;
				
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
				
				size_t pos = 0;
				std::wstring match;
				std::vector<std::wstring> colstrlinks;
				while ((pos = autocolorsSTR.find(L";")) != std::string::npos) 
				{
					match = autocolorsSTR.substr(0, pos);
					colstrlinks.push_back(match);
					autocolorsSTR.erase(0, pos+1);
				}
				
				colstrlinks.push_back(autocolorsSTR);
				
				
				//i wanted to add a thing that checks the amount of colors to describe each pixel for like black and white only pictures
				//but nobody cares about those just convert them to rgb
				for(int i = 0; i < colstrlinks.size(); i++)
				{
					std::array<int,3> color;
					colorTextPair collink;
					for(int k = 0; k < 3; k++)
					{
						color[k] = std::stoi(colstrlinks[i].substr(k*3,3));
					}
					
					for(int k = 0; k < color_stringPairs.size(); k++)
					{
						if(color==color_stringPairs[k].color)
						{
							colstrlinks[i].erase(0,9);
							color_stringPairs[k].text = colstrlinks[i].c_str();
							break;
						}
					}
				}
				
				int eap = 0;
				for(unsigned char *p = pixels; p != pixels+image_size; p += bpp, eap += bpp)
				{
					int w = (eap/bpp)%width;
					
					for(unsigned int i = 0; i < color_stringPairs.size(); i++)
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
				
				/*for(int i = 0; i < color_stringPairs.size(); i++)
				{
					std::string cols = (std::to_string(color_stringPairs[i].color[0])+","+std::to_string(color_stringPairs[i].color[1])+","+std::to_string(color_stringPairs[i].color[2]));
					MessageBox(NULL,(cols+"///"+converter.to_bytes(color_stringPairs[i].text)+"   "+std::to_string(color_stringPairs.size())).c_str(),"exception",0);
				}*/
				
				filestream.close();
				stbi_image_free(pixels);
				
				return LOADIMAGEITT(lParam);
			}
			if(LOWORD(wParam)==IDC_LOADITTIMAGEBUTTON)
			{
				return LOADIMAGEITT(lParam);
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
				
				for(unsigned int i = 0; i < color_stringPairs.size(); i++)
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

				boost::filesystem::path outputPATH = currentPATH / "output" / inputPATH.filename().stem();
				
				boost::filesystem::create_directory((currentPATH / "output").string());
				
				std::string filepath = (outputPATH.string()+".txt");
				
				boost::timer::cpu_timer timer;
				
				bool inverted = (bool)IsDlgButtonChecked(hWnd,IDC_INVERTEDCHECKBOX);
				bool noEmptyChars = (bool)IsDlgButtonChecked(hWnd,IDC_NOSPACECHECKBOX);

				to_braille(pixels,width,height,bpp,filepath,inverted,noEmptyChars);
				
				stbi_image_free(pixels);
				
				std::string finish_message;
				finish_message = "conversion complete in ";
				finish_message += timer.format(3,"%w");
				
				MessageBox(NULL,finish_message.c_str(),"status",0);
				
			}
			if(LOWORD(wParam)==IDC_NOSPACECHECKBOX)
			{
				if(IsDlgButtonChecked(hWnd,IDC_NOSPACECHECKBOX))
				{
					CheckDlgButton(hWnd,IDC_NOSPACECHECKBOX,BST_UNCHECKED);
				} else
				{
					CheckDlgButton(hWnd,IDC_NOSPACECHECKBOX,BST_CHECKED);
				}
			}
			if(LOWORD(wParam)==IDC_INVERTEDCHECKBOX)
			{
				if(IsDlgButtonChecked(hWnd,IDC_INVERTEDCHECKBOX))
				{
					CheckDlgButton(hWnd,IDC_INVERTEDCHECKBOX,BST_UNCHECKED);
				} else
				{
					CheckDlgButton(hWnd,IDC_INVERTEDCHECKBOX,BST_CHECKED);
				}
			}
			break;

		default:
			return DefWindowProc(hWnd, message, wParam, lParam);
	}
	
	return 0;
}

void UpdateDialogues(int mode)
{
	int state0 = (mode==0||mode==1) ? SW_SHOW : SW_HIDE;
	int state1 = (mode==1) ? SW_SHOW : SW_HIDE;
	int state2 = (mode==2||mode==3) ? SW_SHOW : SW_HIDE;
	int state3 = (mode==2) ? SW_SHOW : SW_HIDE;
	ShowWindow(hSizeXEdit, state0);
	ShowWindow(hSizeYEdit, state0);
	ShowWindow(hSizeXText, state0);
	ShowWindow(hSizeYText, state0);
	ShowWindow(hSizeExactText, state1);
	ShowWindow(hSizeTotalEdit, state2);
	ShowWindow(hSizeTotalText, state3);
	ShowWindow(hSizeTotalAspect, state3);
}

LRESULT CALLBACK SizeDialogPrc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	switch(message)
	{
	case WM_CREATE:
		AddControlsSize(hWnd);
		SendMessage(hSizeComboBox, CB_SETCURSEL, (WPARAM)resizeMode, 0);
		if(resizeMode==2||resizeMode==3)
		{
			SendMessage(hSizeTotalEdit,EM_SETSEL,0,-1);
			SendMessage(hSizeTotalEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)(std::to_string((int)round(targetX))).c_str());
			if(resizeMode==2)
			{
				SendMessage(hSizeTotalAspect,EM_SETSEL,0,-1);
				SendMessage(hSizeTotalAspect,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)(std::to_string(targetY)).c_str());
			}
		} else
		{
			std::string xString;
			std::string yString;
			if(resizeMode==1)
			{
				xString = std::to_string((int)round(targetX));
				yString = std::to_string((int)round(targetY));
			} else
			{
				xString = std::to_string(targetX);
				yString = std::to_string(targetY);
			}
			SendMessage(hSizeXEdit,EM_SETSEL,0,-1);
			SendMessage(hSizeXEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)xString.c_str());
			SendMessage(hSizeYEdit,EM_SETSEL,0,-1);
			SendMessage(hSizeYEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)yString.c_str());
		}
		UpdateDialogues(resizeMode);
		break;

	case WM_CLOSE:
		SetFocus(mainWindow);
		DestroyWindow(hWnd);
		break;

	case WM_COMMAND:
		if(LOWORD(wParam)==IDC_SIZEACCEPTBUTTON)
		{
			char xTextBox[1024];
			char yTextBox[1024];

			switch(resizeMode)
			{
				case 2:
				case 3:
					GetWindowText(hSizeTotalEdit,xTextBox,1024);
					GetWindowText(hSizeTotalAspect,yTextBox,1024);
					break;
				default:
					GetWindowText(hSizeXEdit,xTextBox,1024);
					GetWindowText(hSizeYEdit,yTextBox,1024);
					break;

			}

			targetX = std::stod(xTextBox);
			targetY = std::stod(yTextBox);

			SetFocus(mainWindow);
			DestroyWindow(hWnd);
		}
		if(HIWORD(wParam) == CBN_SELCHANGE)
		{
			resizeMode = SendMessage((HWND)lParam, CB_GETCURSEL, 0, 0);

			if(resizeMode==0)
			{
				SendMessage(hSizeXEdit,EM_SETSEL,0,-1);
				SendMessage(hSizeXEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)"1.0");
				SendMessage(hSizeYEdit,EM_SETSEL,0,-1);
				SendMessage(hSizeYEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)"1.0");
			}
			if(resizeMode==1)
			{
				SendMessage(hSizeXEdit,EM_SETSEL,0,-1);
				SendMessage(hSizeXEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)"512");
				SendMessage(hSizeYEdit,EM_SETSEL,0,-1);
				SendMessage(hSizeYEdit,EM_REPLACESEL,(WPARAM)FALSE,(LPARAM)"-1");
			}
			
			UpdateDialogues(resizeMode);
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
	
	CreateWindow("button","change size",WS_VISIBLE|WS_CHILD|WS_BORDER,15,80,150,35,hWnd,(HMENU)IDC_SIZECHANGEBUTTON,NULL,NULL);
	
	/*
	CreateWindow("static","X",WS_VISIBLE|WS_CHILD|ES_CENTER,15,90,50,25,hWnd,NULL,NULL,NULL);
	hEditScaleX = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","1.0",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,15,115,50,25,hWnd,NULL,NULL,NULL);
	SetWindowLongPtr(hEditScaleX,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	
	CreateWindow("static","Y",WS_VISIBLE|WS_CHILD|ES_CENTER,115,90,50,25,hWnd,NULL,NULL,NULL);
	hEditScaleY = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","1.0",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,115,115,50,25,hWnd,NULL,NULL,NULL);
	SetWindowLongPtr(hEditScaleY,GWL_WNDPROC,(LONG_PTR)&EditPrc);
	*/
	
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
	
	CreateWindow("static","link colors to text (click on listbox buttons above they're better)",WS_VISIBLE|WS_CHILD|ES_CENTER,15,150,330,50,hWnd,NULL,NULL,NULL);
	hITTAutoColorsEdit = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","000255255text here;127127127some symbol;005002090you get it",WS_VISIBLE|WS_CHILD|ES_AUTOHSCROLL,15,190,330,25,hWnd,NULL,NULL,NULL);
	(WNDPROC)SetWindowLongPtr(hITTAutoColorsEdit,GWL_WNDPROC,(LONG_PTR)&EditPrc);
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

	CreateWindow("button","inverted",WS_VISIBLE|WS_CHILD|BS_CHECKBOX,175,15,135,25,hWnd,(HMENU)IDC_INVERTEDCHECKBOX,NULL,NULL);
	CreateWindow("button","replace empty characters with 1 dot",WS_VISIBLE|WS_CHILD|BS_CHECKBOX|BS_MULTILINE,175,35,135,100,hWnd,(HMENU)IDC_NOSPACECHECKBOX,NULL,NULL);

	CreateWindow("button","convert",WS_VISIBLE|WS_CHILD|WS_BORDER,15,75,150,35,hWnd,(HMENU)IDC_BWBCONVERTBUTTON,NULL,NULL);
}

void AddControlsSize(HWND hWnd)
{
	hSizeComboBox = CreateWindow(WC_COMBOBOX,"",CBS_DROPDOWNLIST|WS_CHILD|WS_OVERLAPPED|WS_VISIBLE,15,15,150,100,hWnd,NULL,NULL,NULL);
	
	SendMessage(hSizeComboBox, CB_ADDSTRING, 0, (LPARAM)TEXT("multipliers"));
	SendMessage(hSizeComboBox, CB_ADDSTRING, 0, (LPARAM)TEXT("exact"));
	SendMessage(hSizeComboBox, CB_ADDSTRING, 0, (LPARAM)TEXT("total"));
	SendMessage(hSizeComboBox, CB_ADDSTRING, 0, (LPARAM)TEXT("total w/newlines"));
	SendMessage(hSizeComboBox, CB_SETCURSEL, (WPARAM)0, 0);

	hSizeXText = CreateWindow("static","X",WS_VISIBLE|WS_CHILD|ES_CENTER,15,45,50,25,hWnd,NULL,NULL,NULL);
	hSizeXEdit = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","",WS_VISIBLE|WS_CHILD,15,65,50,25,hWnd,NULL,NULL,NULL);

	hSizeYText = CreateWindow("static","Y",WS_VISIBLE|WS_CHILD|ES_CENTER,100,45,50,25,hWnd,NULL,NULL,NULL);
	hSizeYEdit = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","",WS_VISIBLE|WS_CHILD,115,65,50,25,hWnd,NULL,NULL,NULL);

	hSizeExactText = CreateWindow("static","-1 to keep ratio",WS_VISIBLE|WS_CHILD|ES_CENTER,15,100,150,25,hWnd,NULL,NULL,NULL);

	hSizeTotalEdit = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","1024",WS_VISIBLE|WS_CHILD,15,45,150,25,hWnd,NULL,NULL,NULL);
	hSizeTotalText = CreateWindow("static","ratio (from 0 to 2)",WS_VISIBLE|WS_CHILD|ES_CENTER,15,75,150,25,hWnd,NULL,NULL,NULL);
	hSizeTotalAspect = CreateWindowEx(WS_EX_CLIENTEDGE,"edit","1.0",WS_CHILD|WS_VISIBLE,15,100,150,25,hWnd,NULL,NULL,NULL);

	CreateWindow("button","confirm",WS_VISIBLE|WS_CHILD|WS_BORDER,15,130,150,35,hWnd,(HMENU)IDC_SIZEACCEPTBUTTON,NULL,NULL);
}

void AddControlsTAC(HWND hWnd)
{
	std::array<int,3> color = colorFromIndex(TACSelItem, hPixelInfoList);

	int prevIndex;

	for(unsigned int i = 0; i < color_stringPairs.size(); i++)
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

bool RegisterSizeDialog(HINSTANCE hInstance)
{
	WNDCLASS wc = {};

	wc.lpfnWndProc    = SizeDialogPrc;
	wc.hInstance      = hInstance;
	wc.hCursor        = LoadCursor(NULL, IDC_ARROW);
	wc.hbrBackground  = (HBRUSH)COLOR_WINDOW;
	wc.lpszClassName  = "MySizeDialogClass";

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
		for(unsigned int i = 0; i < color_pallete.size(); i++)
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
		hITTDialog = CreateWindow("MyDialogClass","image to text",WS_OVERLAPPED|WS_VISIBLE|WS_SYSMENU,p.x-200/2,p.y-100/2,360,250,hWnd,NULL,NULL,NULL);
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

		hBWBDialog = CreateWindow("MyBWBDialogClass","convert black white img to braille",WS_VISIBLE|WS_OVERLAPPED|WS_SYSMENU,p.x-200/2,p.y-100/2,320,145,hWnd,NULL,NULL,NULL);
	}
}

void DisplaySizeDialog(HWND hWnd)
{
	if(!IsWindow(hSizeDialog))
	{
		POINT p;
		GetCursorPos(&p);

		hSizeDialog = CreateWindow("MySizeDialogClass","resize end image",WS_VISIBLE|WS_OVERLAPPED|WS_SYSMENU,p.x-200/2,p.y-100/2,190,210,hWnd,NULL,NULL,NULL);
	}
}