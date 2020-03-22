#include <iostream>
#include <cmath>
#include <vector>
#include <array>
#include <regex>
#include "dither.h"
#include "image_pallete.h"

using namespace std;

inline int cl_byte(int uncl)
{
	int x = uncl>255?255:uncl;
	return x<0?0:x;
}

int main()
{
	int fformat;
	int transparent;
	float scaleX;
	float scaleY;
	
	vector<array<int,3>> pallete_arr;
	
	string palletePath;
	string pallete;
	string filename;
	string filename_o;
	
	cout << "png or jpg output (0 for png, 1 for jpg): \n";
	cin >> fformat;
	cout << "\nmake fully transparent pixels transparent on the image? (0 for no, 1 for yes): \n";
	cin >> transparent;
	cout << "\nimage scale factor X: \n";
	cin >> scaleX;
	cout << "\nimage scale factor Y: \n";
	cin >> scaleY;
	cin.ignore();
	cout << "\npath to color pallete image (leave blank for manual colors): \n";
	getline(cin,palletePath);
	if(palletePath.length()==0)
	{
		cout << "colors of pallete in rgb (3 numbers from 0-255 for each color): \n";
		getline(cin,pallete);
		regex reg("\\d{1,3}");
		
		int ni = 0;
		
		for(sregex_iterator it = sregex_iterator(pallete.begin(), pallete.end(), reg); 
		it != sregex_iterator(); it++, ni++)
		{
			smatch match = *it;
			
			if(ni==3)
			{
				ni=0;
			}
			
			if(ni==0)
			{
				pallete_arr.push_back(array<int,3>{0,0,0});
			}
			pallete_arr.back()[ni]=stoi(match.str(0));
		}
	} else
	{
		pallete_arr = GetPallete(palletePath);
		cout << pallete_arr.size() << " colors in pallete\n";
	}
	cout << endl << "full path to input image: \n";
	getline(cin,filename);
	cout << endl << "full path to output image: \n";
	getline(cin,filename_o);
	
	cout << "\ndithering in process plz wait\n\n";
	
	dither_image(filename, filename_o, pallete_arr, scaleX, scaleY, 3, fformat, transparent);
	
	system("pause");
	
	return 0;
}