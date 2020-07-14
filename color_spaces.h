#ifndef COLOR_SPACES_H
#define COLOR_SPACES_H

#include <array>

class cRGB
{
public:
	cRGB();
	cRGB(float,float,float);
	cRGB(std::array<float,3>);
	std::array<float,3> to_array();
	float r;
	float g;
	float b;
};

class cXYZ
{
public:
	cXYZ();
	cXYZ(float,float,float);
	cXYZ(std::array<float,3>);
	std::array<float,3> to_array();
	float x;
	float y;
	float z;
};

class cLAB
{
public:
	cLAB();
	cLAB(float,float,float);
	cLAB(std::array<float,3>);
	std::array<float,3> to_array();
	float l;
	float a;
	float b;
};

cXYZ RGBtoXYZ(cRGB);
cRGB XYZtoRGB(cXYZ);
cXYZ LABtoXYZ(cLAB);
cLAB XYZtoLAB(cXYZ);

#endif