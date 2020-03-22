#include <iostream>
#include <cmath>
#include <cstdint>
#include "color_spaces.h"
#include "dither.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define STB_IMAGE_RESIZE_IMPLEMENTATION
#include "stb_image_resize.h"

using namespace std;

void dither_image(string filename, string filename_o, vector<array<int,3>> pallete_arr, float scaleX = 1, float scaleY = 1, int color_model = 0, int fformat = 0, int transparency = 0)
{
	int width, height, bpp;
	int widthR, heightR;
	
	int colors = 3;
	colors += transparency;
	
	unsigned char *image_unres = stbi_load(filename.c_str(), &width, &height, &bpp, 0);
	
	size_t image_unres_size = width * height * bpp;
	
	widthR = round((float)width * scaleX);
	heightR = round((float)height * scaleY);
	
	size_t image_size = widthR * heightR * bpp;
	
	unsigned char *image = (unsigned char*)malloc(image_size);
	
	stbir_resize_uint8(image_unres, width, height, 0, image, widthR, heightR, 0, bpp);
	
	width = widthR;
	height = heightR;
	
	size_t output_size = width * height * colors;
	
	unsigned char *output_image = (unsigned char*)malloc(output_size);
	
	int error_clrs = 3;
	
	int ***errors = new int**[width];
	for(int i = 0; i < width; i++)
	{
		errors[i] = new int*[height];
		for(int j = 0; j < height; j++)
		{
			errors[i][j] = new int[error_clrs];
			for(int k = 0; k < error_clrs; k++)
			{
				errors[i][j][k] = 0;
			}
		}
	}
	
	int eap = 0;
	
	for(unsigned char *p = image, *po = output_image; p != image+image_size; p += bpp, po += colors, eap += colors)
	{
		float t = 1;
		
		int closestIndex = 0;
		float closestDistance = -1;
		
		int color[3];
		int closestColor[3];
		
		if(bpp==4)
		{
			t = (*(p+3))/255.0;
		}
		color[0] = round((*p)*t);
		color[1] = round((*(p+1))*t);
		color[2] = round((*(p+2))*t);
		
		int h = (int)((eap/colors)/width);
		int w = (eap/colors)%width;
		
		color[0] = color[0]+errors[w][h][0];
		color[1] = color[1]+errors[w][h][1];
		color[2] = color[2]+errors[w][h][2];
		
		for(int i = 0; i < pallete_arr.size(); i++)
		{
			float distance;
			if (color_model == 0)
			{
				distance = abs(pallete_arr[i][0] - color[0]) + abs(pallete_arr[i][1] - color[1]) + abs(pallete_arr[i][2] - color[2]);
			}
			else
			{
				RGB tempRGB;

				tempRGB.r = color[0];
				tempRGB.g = color[1];
				tempRGB.b = color[2];

				LAB LABColor = XYZtoLAB(RGBtoXYZ(tempRGB));

				tempRGB.r = pallete_arr[i][0];
				tempRGB.g = pallete_arr[i][1];
				tempRGB.b = pallete_arr[i][2];

				LAB PLABColor = XYZtoLAB(RGBtoXYZ(tempRGB));

				distance = abs(PLABColor.l - LABColor.l) + abs(PLABColor.a - LABColor.a) + abs(PLABColor.b - LABColor.b);
			}
			if(closestDistance==-1)
			{
				closestDistance=distance;
			}
			if(distance < closestDistance)
			{
				closestIndex = i;
				closestDistance = distance;
			}
		}
		
		closestColor[0] = pallete_arr[closestIndex][0];
		closestColor[1] = pallete_arr[closestIndex][1];
		closestColor[2] = pallete_arr[closestIndex][2];
		
		float error[3];
		float part = 48;
		error[0] = (color[0]-closestColor[0])/part;
		error[1] = (color[1]-closestColor[1])/part;
		error[2] = (color[2]-closestColor[2])/part;
		
		float error7[3];
		float error5[3];
		float error3[3];
		float error1[3];
		
		error7[0] = error[0]*7;
		error7[1] = error[1]*7;
		error7[2] = error[2]*7;
		
		error5[0] = error[0]*5;
		error5[1] = error[1]*5;
		error5[2] = error[2]*5;
		
		error3[0] = error[0]*3;
		error3[1] = error[1]*3;
		error3[2] = error[2]*3;
		
		bool right1 = w<(width-1);
		bool right2 = w<(width-2);
		bool down1 = h<(height-1);
		bool down2 = h<(height-2);
		bool left1 = w>0;
		bool left2 = w>1;
		
		if(right1)
		{
			errors[w+1][h][0] += error7[0];
			errors[w+1][h][1] += error7[1];
			errors[w+1][h][2] += error7[2];
			if(down1)
			{
				errors[w+1][h+1][0] += error5[0];
				errors[w+1][h+1][1] += error5[1];
				errors[w+1][h+1][2] += error5[2];
			}
			if(down2)
			{
				errors[w+1][h+2][0] += error3[0];
				errors[w+1][h+2][1] += error3[1];
				errors[w+1][h+2][2] += error3[2];
			}
		}
		
		if(right2)
		{
			errors[w+2][h][0] += error5[0];
			errors[w+2][h][1] += error5[1];
			errors[w+2][h][2] += error5[2];
			if(down1)
			{
				errors[w+2][h+1][0] += error3[0];
				errors[w+2][h+1][1] += error3[1];
				errors[w+2][h+1][2] += error3[2];
			}
			if(down2)
			{
				errors[w+2][h+2][0] += error[0];
				errors[w+2][h+2][1] += error[1];
				errors[w+2][h+2][2] += error[2];
			}
		}
		
		if(down1)
		{
			errors[w][h+1][0] += error7[0];
			errors[w][h+1][1] += error7[1];
			errors[w][h+1][2] += error7[2];
		}
		if(down2)
		{
			errors[w][h+2][0] += error5[0];
			errors[w][h+2][1] += error5[1];
			errors[w][h+2][2] += error5[2];
		}
		
		if(left1)
		{
			if(down1)
			{
				errors[w-1][h+1][0] += error5[0];
				errors[w-1][h+1][1] += error5[1];
				errors[w-1][h+1][2] += error5[2];
			}
			if(down2)
			{
				errors[w-1][h+2][0] += error3[0];
				errors[w-1][h+2][1] += error3[1];
				errors[w-1][h+2][2] += error3[2];
			}
		}
		
		if(left2)
		{
			if(down1)
			{
				errors[w-2][h+1][0] += error3[0];
				errors[w-2][h+1][1] += error3[1];
				errors[w-2][h+1][2] += error3[2];
			}
			if(down2)
			{
				errors[w-2][h+2][0] += error[0];
				errors[w-2][h+2][1] += error[1];
				errors[w-2][h+2][2] += error[2];
			}
		}
		
		*po = (uint8_t)closestColor[0];
		*(po+1) = (uint8_t)closestColor[1];
		*(po+2) = (uint8_t)closestColor[2];
		if(transparency==1)
		{
			*(po+3) = *(p+3)==255?255:0;
		}
	}
	
	delete[] errors;
	
	if(fformat==0)
	{
		stbi_write_png(filename_o.c_str(), width, height, colors, output_image, width * colors);
	} else if(fformat==1)
	{
		stbi_write_jpg(filename_o.c_str(), width, height, colors, output_image, 100);
	}
	
	stbi_image_free(image_unres);
}