#ifndef CORE_TEXTURE_H
#define CORE_TEXTURE_H

#include "pbr.h"
#include "geometry.h"

template <typename T>
class Texture {
public:
    // Texture Interface
    virtual T Evaluate(const SurfaceInteraction&) const = 0;
    virtual ~Texture() {}
};

#endif