#ifndef DITHER_H
#define DITHER_H

#include <vector>
#include <array>

void dither_image(std::string,std::string,std::vector<std::array<int,3>>,float,float,int,int,int);

#endif