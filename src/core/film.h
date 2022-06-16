#ifndef CORE_FILM_H
#define CORE_FILM_H

#include <string>
#include <vector>
#include "interaction.h"
#include "transform.h"

struct Pixel {
    Pixel() { xyz[0] = xyz[1] = xyz[2] = 0; }
    float xyz[3];
    /*Float filterWeightSum;
    AtomicFloat splatXYZ[3];
    Float pad;*/
};

class Film {
public:
	Film(const Point2i& resolution) : fullResolution(resolution), filename("pbr_img.ppm") {
        status = 0;
        pixels.resize(resolution.x * resolution.y);
    }

    void WriteImage();
    void WritePPMImage();

    const Point2i fullResolution;

    int status;

    const std::string filename;

    std::vector<Spectrum> pixels;
};



#endif