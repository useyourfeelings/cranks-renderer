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

////////////////////////////

class TextureMapping2D {
public:
    // TextureMapping2D Interface
    virtual ~TextureMapping2D() {};
    virtual Point2f Map(const SurfaceInteraction& si, Vector2f* dstdx,
        Vector2f* dstdy) const = 0;
};

class UVMapping2D : public TextureMapping2D {
public:
    // UVMapping2D Public Methods
    UVMapping2D(float su = 1, float sv = 1, float du = 0, float dv = 0);
    Point2f Map(const SurfaceInteraction& si, Vector2f* dstdx,
        Vector2f* dstdy) const;

private:
    const float su, sv, du, dv;
};

#endif