CXX = g++
CXXFLAGS = -IC:/Users/Xzyaihni/Desktop/cppshit/dithering -IC:/boost_1_72_0 -LC:/boost_1_72_0/stage/lib
LIBS = -lboost_timer-mgw81-mt-s-x32-1_72 -lboost_chrono-mgw81-mt-s-x32-1_72 -lboost_filesystem-mgw81-mt-s-x32-1_72 -lcomdlg32 -static-libgcc -static-libstdc++ -Wall -Wl,-Bstatic -lstdc++ -lpthread

all: dithergui

main.o: main.cpp 
	$(CXX) $(CXXFLAGS) -c main.cpp $(LIBS)
	
dither.o: dither.cpp
	$(CXX) $(CXXFLAGS) -c dither.cpp $(LIBS)

image_pallete.o: image_pallete.cpp
	$(CXX) $(CXXFLAGS) -c image_pallete.cpp $(LIBS)
	
color_spaces.o: color_spaces.cpp
	$(CXX) $(CXXFLAGS) -c color_spaces.cpp $(LIBS)
	
to_braille.o: to_braille.cpp
	$(CXX) $(CXXFLAGS) -c to_braille.cpp $(LIBS)

dithergui.o: dithergui.cpp
	$(CXX) $(CXXFLAGS) -c dithergui.cpp $(LIBS)
	
dithergui: dithergui.o dither.o  image_pallete.o color_spaces.o to_braille.o
	$(CXX) $(CXXFLAGS) dithergui.o dither.o image_pallete.o color_spaces.o to_braille.o -o build/dithergui $(LIBS) -mwindows