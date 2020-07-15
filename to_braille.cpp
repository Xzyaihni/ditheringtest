#include <codecvt>
#include <iostream>
#include <fstream>
#include <bitset>

#include "to_braille.h"

void to_braille(unsigned char* pixels, int width, int height, int bpp, std::string outputPath, bool inverted, bool noEmptyChars)
{
	std::wofstream filestream(outputPath.c_str());
				
	filestream.imbue(std::locale(filestream.getloc(), new std::codecvt_utf8_utf16<wchar_t>));
	
	int remainderX = width % 2;
	
	int smallImgSize = (height/4)*(width/2);
	
	int smallWidth = width/2;
				
	int eap = 0;

	for(unsigned char *p = pixels; p != pixels+smallImgSize; p++, eap++)
	{
		int w = eap%smallWidth;
		
		int h = eap/smallWidth;
		
		std::bitset<8> blockNUM;

		int bitVals[8] = {0};

		for(int y = 0; y < 4; y++)
		{
			for(int x = 0; x < 2; x++)
			{
				int positionOff = (w*2+x)*bpp+(h*4+y)*((smallWidth*2+remainderX)*bpp);

				int bitPos = y==3 ? 6+x : y+x*3;

				bitVals[bitPos] = 0;
				if(!(((w+1)*2)>width | ((h+1)*4)>height))
				{
					bitVals[bitPos] = (*(pixels+positionOff)>127?0:1);
				}
			}
		}
		
		blockNUM.set(0,bitVals[0]);
		blockNUM.set(1,bitVals[1]);
		blockNUM.set(2,bitVals[2]);
		blockNUM.set(3,bitVals[3]);
		blockNUM.set(4,bitVals[4]);
		blockNUM.set(5,bitVals[5]);
		blockNUM.set(6,bitVals[6]);
		blockNUM.set(7,bitVals[7]);
		
		if(inverted)
		{
			blockNUM.flip();
		}
		if(noEmptyChars&&blockNUM.none())
		{
			blockNUM.set(0,1);
		}
		
		filestream << (wchar_t)(0x2800+blockNUM.to_ulong());
		if(w==(smallWidth-1))
		{
			filestream << "\n";
		}
	}
	
	filestream.close();
}