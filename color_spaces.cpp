#include <iostream>
#include <cmath>
#include "color_spaces.h"

#include <array>

cRGB::cRGB():r(0),g(0),b(0) {}
cRGB::cRGB(std::array<float,3> color):r(color[0]),g(color[1]),b(color[2]) {}
std::array<float,3> cRGB::to_array()
{
	return {this->r,this->g,this->b};
}

cXYZ::cXYZ():x(0),y(0),z(0) {}
cXYZ::cXYZ(std::array<float,3> color):x(color[0]),y(color[1]),z(color[2]) {}
std::array<float,3> cXYZ::to_array()
{
	return {this->x,this->y,this->z};
}

cLAB::cLAB():l(0),a(0),b(0) {}
cLAB::cLAB(std::array<float,3> color):l(color[0]),a(color[1]),b(color[2]) {}
std::array<float,3> cLAB::to_array()
{
	return {this->l,this->a,this->b};
}

cXYZ RGBtoXYZ(cRGB color)
{
	cXYZ retCol;

	float nR = (float)color.r/255;
	float nG = (float)color.g/255;
	float nB = (float)color.b/255;

	if (nR > 0.04045f)
	{
		nR = std::pow((nR+0.055f)/1.055f,2.4f);
	}
	else
	{
		nR /= 12.92f;
	}

	if (nG > 0.04045f)
	{
		nG = std::pow((nG + 0.055f) / 1.055f, 2.4f);
	}
	else
	{
		nG /= 12.92f;
	}

	if (nB > 0.04045f)
	{
		nB = std::pow((nB + 0.055f) / 1.055f, 2.4f);
	}
	else
	{
		nB /= 12.92f;
	}

	nR *= 100;
	nG *= 100;
	nB *= 100;

	retCol.x = nR*0.4124f+nG*0.3576f+nB*0.1805f;
	retCol.y = nR*0.2126f+nG*0.7152f+nB*0.0722f;
	retCol.z = nR*0.0193f+nG*0.1192f+nB*0.9505f;

	return retCol;
}

cRGB XYZtoRGB(cXYZ color)
{
	cRGB retCol;
	float nX = color.x/100.0;
	float nY = color.y/100.0;
	float nZ = color.z/100.0;
	
	float nR = nX*3.2406+nY*-1.5372+nZ*-0.4986;
	float nG = nX*-0.9689+nY*1.8758+nZ*0.0415;
	float nB = nX*0.0557+nY*-0.2040+nZ*1.0570;

	if (nR > 0.00031308)
	{
		nR = 1.055*(std::pow(nR,(1/2.4)))-0.055;
	}
	else
	{
		nR = 12.92*nR;
	}

	if (nG > 0.00031308)
	{
		nG = 1.055*(std::pow(nG,(1/2.4)))-0.055;
	}
	else
	{
		nG = 12.92*nG;
	}

	if (nB > 0.00031308)
	{
		nB = 1.055*(std::pow(nB,(1/2.4)))-0.055;
	}
	else
	{
		nB = 12.92*nB;
	}

	retCol.r = nR*255;
	retCol.g = nG*255;
	retCol.b = nB*255;

	return retCol;
}

cXYZ LABtoXYZ(cLAB color)
{
	cXYZ retCol;

	float nY = (color.l+16)/116.0;
	float nX = color.a/500.0+nY;
	float nZ = nY-color.b/200.0;

	if (std::pow(nY,3) > 0.008856)
	{
		nY = std::pow(nY,3);
	}
	else
	{
		nY = (nY-16/116.0)/7.787;
	}

	if (std::pow(nX,3) > 0.008856)
	{
		nX = std::pow(nX, 3);
	}
	else
	{
		nX = (nX - 16 / 116.0) / 7.787;
	}

	if (std::pow(nZ,3) > 0.008856)
	{
		nZ = std::pow(nZ, 3);
	}
	else
	{
		nZ = (nZ - 16 / 116.0) / 7.787;
	}

	retCol.x = nX*95.047f;
	retCol.y = nY*100.0f;
	retCol.z = nZ*108.883f;

	return retCol;
}

cLAB XYZtoLAB(cXYZ color)
{
	cLAB retCol;

	float nX = color.x/95.047f;
	float nY = color.y/100.0f;
	float nZ = color.z/108.883f;

	if (nX > 0.008856)
	{
		nX = std::pow(nX,1.0/3.0);
	}
	else
	{
		nX = (7.787f * nX) + (16.0/116.0);
	}

	if (nY > 0.008856)
	{
		nY = std::pow(nY, 1.0 / 3.0);
	}
	else
	{
		nY = (7.787f * nY) + (16.0 / 116.0);
	}

	if (nZ > 0.008856)
	{
		nZ = std::pow(nZ, 1.0 / 3.0);
	}
	else
	{
		nZ = (7.787f * nZ) + (16.0 / 116.0);
	}

	retCol.l = (116.0 * nY) - 16;
	retCol.a = 500.0 * (nX-nY);
	retCol.b = 200.0 * (nY-nZ);

	return retCol;
}