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

//#include <windows.h>

using namespace std;

unsigned char* dither_image(unsigned char *image_unres, int width, int height, int bpp, vector<array<int,3>> pallete_arr, int newWidth, int newHeight, int color_model, bool transparency)
{
	int colors = 3 + transparency;
	
	size_t image_size = newWidth * newHeight * bpp;

	unsigned char *image = (unsigned char*)malloc(image_size);

	if(width!=newWidth || height!=newHeight)
	{
		stbir_resize_uint8(image_unres, width, height, 0, image, newWidth, newHeight, 0, bpp);
	
		width = newWidth;
		height = newHeight;
	} else
	{
		image = image_unres;
	}
	
	size_t output_size = width * height * colors;

	unsigned char *output_image = new unsigned char[output_size];

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
		int closestIndex = 0;
		float closestDistance = -1;
		
		float color[3];
		int closestColor[3];
		
		int h = (int)((eap/colors)/width);
		int w = (eap/colors)%width;
		
		for(int i = 0; i < 3; i++)
		{
			color[i] = round((*(p+i))*(!(bpp==4)+((*(p+3))/255.0)*(bpp==4)));
			
			color[i] = color[i]+errors[w][h][i];
		}
		
		for(unsigned int i = 0; i < pallete_arr.size(); i++)
		{
			float distance;
			if (color_model == 0)
			{
				distance = abs(pallete_arr[i][0] - color[0]) + abs(pallete_arr[i][1] - color[1]) + abs(pallete_arr[i][2] - color[2]);
			}
			else
			{
				float LABColors[6];
				LABColors[0] = color[0];
				LABColors[1] = color[1];
				LABColors[2] = color[2];
				LABColors[3] = pallete_arr[i][0];
				LABColors[4] = pallete_arr[i][1];
				LABColors[5] = pallete_arr[i][2];
				
				for(int k = 0; k < 6; k++)
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
				
				for(int k = 0; k < 2; k++)
				{
					LABColors[3*k] = LABColors[3*k]*0.4124f+LABColors[1+3*k]*0.3576f+LABColors[2+3*k]*0.1805f;
					LABColors[1+3*k] = LABColors[3*k]*0.2126f+LABColors[1+3*k]*0.7152f+LABColors[2+3*k]*0.0722f;
					LABColors[2+3*k] = LABColors[3*k]*0.0193f+LABColors[1+3*k]*0.1192f+LABColors[2+3*k]*0.9505f;
					
					LABColors[3*k] /= 95.047f;
					LABColors[1+3*k] /= 100.0f;
					LABColors[2+3*k] /= 108.883f;
					
					for(int j = 0; j < 3; j++)
					{
						if (LABColors[j+3*k] > 0.008856)
						{
							LABColors[j+3*k] = std::cbrt(LABColors[j+3*k]);//std::pow(LABColors[j+3*k],1.0/3.0);
						}
						else
						{
							LABColors[j+3*k] = (7.787f * LABColors[j+3*k]) + (16.0/116.0);
						}
					}
					
					float col0 = (116.0 * LABColors[1+3*k]) - 16;
					float col1 = 500.0 * (LABColors[3*k]-LABColors[1+3*k]);
					float col2 = 200.0 * (LABColors[1+3*k]-LABColors[2+3*k]);
					
					LABColors[3*k] = col0;
					LABColors[1+3*k] = col1;
					LABColors[2+3*k] = col2;
				}

				distance = abs(LABColors[3] - LABColors[0]) + abs(LABColors[4] - LABColors[1]) + abs(LABColors[5] - LABColors[2]);
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
		
		float error[3];
		float error7[3];
		float error5[3];
		float error3[3];
		
		bool right1 = w<(width-1);
		bool right2 = w<(width-2);
		bool down1 = h<(height-1);
		bool down2 = h<(height-2);
		bool left1 = w>0;
		bool left2 = w>1;
		
		for(int i = 0; i <  3; i++)
		{
			closestColor[i] = pallete_arr[closestIndex][i];
			
			error[i] = (color[i]-closestColor[i])/48.0;
			error7[i] = error[i]*7;
			error5[i] = error[i]*5;
			error3[i] = error[i]*3;
			
			if(right1)
			{
				errors[w+1][h][i] += error7[i];
				if(down1)
				{
					errors[w+1][h+1][i] += error5[i];
				}
				if(down2)
				{
					errors[w+1][h+2][i] += error3[i];
				}
			}
			
			if(right2)
			{
				errors[w+2][h][i] += error5[i];
				if(down1)
				{
					errors[w+2][h+1][i] += error3[i];
				}
				if(down2)
				{
					errors[w+2][h+2][i] += error[i];
				}
			}
			
			if(down1)
			{
				errors[w][h+1][i] += error7[i];
			}
			if(down2)
			{
				errors[w][h+2][i] += error5[i];
			}
			
			if(left1)
			{
				if(down1)
				{
					errors[w-1][h+1][i] += error5[i];
				}
				if(down2)
				{
					errors[w-1][h+2][i] += error3[i];
				}
			}
			
			if(left2)
			{
				if(down1)
				{
					errors[w-2][h+1][i] += error3[i];
				}
				if(down2)
				{
					errors[w-2][h+2][i] += error[i];
				}
			}
			
			*(po+i) = (uint8_t)closestColor[i];
			
		}
		if(transparency==1)
		{
			*(po+3) = (*(p+3)>127)*255;
		}
	}

	delete[] errors;

	return output_image;
}

void dither_image(string filename, string filename_o, vector<array<int,3>> pallete_arr, int newWidth, int newHeight, int color_model, bool transparency, bool fformat)
{
	int width, height, bpp;
	
	unsigned char *image_unres = stbi_load(filename.c_str(), &width, &height, &bpp, 0);
	
	unsigned char *output_image = dither_image(image_unres, width, height, bpp, pallete_arr, newWidth, newHeight, color_model, transparency);

	int colors = 3 + transparency;

	if(!fformat)
	{
		stbi_write_png(filename_o.c_str(), newWidth, newHeight, colors, output_image, newWidth * colors);
	} else
	{
		stbi_write_jpg(filename_o.c_str(), newWidth, newHeight, colors, output_image, 100);
	}

	stbi_image_free(image_unres);
	if(image_unres!=output_image)
	{
		delete[] output_image;
	}
}