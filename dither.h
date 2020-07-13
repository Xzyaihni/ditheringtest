#ifndef DITHER_H
#define DITHER_H

#include <vector>
#include <array>

unsigned char* dither_image(unsigned char*,int,int,int,std::vector<std::array<int,3>>,int,int,int=0,bool=false);
void dither_image(std::string,std::string,std::vector<std::array<int,3>>,int,int,int=0,bool=false,bool=false);

#endif