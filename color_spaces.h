#ifndef COLOR_SPACES_H
#define COLOR_SPACES_H

#include <array>

class cRGB;
class cXYZ;
class cLAB;

class cRGB
{
public:
	cRGB();
	cRGB(float,float,float);
	cRGB(std::array<float,3>);
	std::array<float,3> to_array();
	cXYZ toXYZ();
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
	cRGB toRGB();
	cLAB toLAB();
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
	cXYZ toXYZ();
	float l;
	float a;
	float b;
};

#endif