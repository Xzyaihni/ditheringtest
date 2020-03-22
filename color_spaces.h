#ifndef COLOR_SPACES_H
#define COLOR_SPACES_H

class RGB
{
public:
	int r;
	int g;
	int b;
};

class XYZ
{
public:
	float x;
	float y;
	float z;
};

class LAB
{
public:
	float l;
	float a;
	float b;
};

XYZ RGBtoXYZ(RGB);
RGB XYZtoRGB(XYZ);
XYZ LABtoXYZ(LAB);
LAB XYZtoLAB(XYZ);

#endif