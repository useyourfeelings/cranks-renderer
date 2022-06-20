#include "film.h"
#include<fstream>
#include "../tool/logger.h"

void Film::WritePPMImage() {
	Log("WritePPMImage");
	std::fstream f;
	f.open(filename, std::fstream::out | std::fstream::trunc);
	if (!f.is_open()) {
		Log("open failed");
		return;
	}

	f << "P3" << std::endl << fullResolution.x << " " << fullResolution.y << std::endl << 255 << std::endl;

	int index;
	for (int j = 0; j < fullResolution.y; ++j) {
		for (int i = 0; i < fullResolution.x; ++i) {
			index = i + fullResolution.x * j;
			f << std::min(int(pixels[index].c[0] * 255), 255) << " " 
				<< std::min(int(pixels[index].c[1] * 255), 255) << " "
				<< std::min(int(pixels[index].c[2] * 255), 255) << std::endl;
		}
	}

	f.close();
}

void Film::WriteImage() {
	WritePPMImage();
}