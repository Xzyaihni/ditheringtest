#include <algorithm>
#include <vector>
#include <array>
#include <exception>
#include "stb_image.h"

using namespace std;

vector<array<int,3>> GetPallete(string inputPath)
{
	int width, height, bpp;
	
	vector<array<int,3>> colors;
	
	unsigned char *image;
	try
	{
		image = stbi_load(inputPath.c_str(), &width, &height, &bpp, 0);
	} catch(exception &e)
	{
		throw e;
	}
	
	size_t image_size = width * height * bpp;
	
	for(unsigned char *p = image; p != image+image_size; p += bpp)
	{
		float t = 1;
		int color[3];
		
		if(bpp==4)
		{
			t = (*(p+3))/255.0;
		}
		
		color[0] = (*p)*t;
		color[1] = (*(p+1))*t;
		color[2] = (*(p+2))*t;
		
		if(!(find(colors.begin(),colors.end(),array<int,3>{color[0],color[1],color[2]}) != colors.end()))
		{
			colors.push_back(array<int,3>{color[0],color[1],color[2]});
		}
	}
	
	stbi_image_free(image);
	
	return colors;
}