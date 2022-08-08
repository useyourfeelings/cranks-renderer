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

template <typename T>
class ConstantTexture : public Texture<T> {
public:
    // ConstantTexture Public Methods
    ConstantTexture(const T& value) : value(value) {}
    T Evaluate(const SurfaceInteraction&) const { return value; }

private:
    T value;
};

//

float Lanczos(float, float tau = 2);

#endif