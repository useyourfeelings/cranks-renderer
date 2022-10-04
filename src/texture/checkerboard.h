#ifndef CORE_CHECKBOARD_H
#define CORE_CHECKBOARD_H

#include "../core/pbr.h"
#include "../core/texture.h"
#include "../core/spectrum.h"

enum class AAMethod { None, ClosedForm };

// CheckerboardTexture Declarations
template <typename T>
class Checkerboard2DTexture : public Texture<T> {
public:
    // Checkerboard2DTexture Public Methods
    Checkerboard2DTexture(std::unique_ptr<TextureMapping2D> mapping,
        const std::shared_ptr<Texture<T>>& tex1,
        const std::shared_ptr<Texture<T>>& tex2,
        AAMethod aaMethod)
        : mapping(std::move(mapping)),
        tex1(tex1),
        tex2(tex2),
        aaMethod(aaMethod) {}
    T Evaluate(const SurfaceInteraction& si) const {
        Vector2f dstdx, dstdy;
        Point2f st = mapping->Map(si, &dstdx, &dstdy);

        //std::cout << "checker Evaluate st = " << st[0] << " " << st[1] << std::endl;
        //std::cout << "checker Evaluate dstdx = " << dstdx[0] << " " << dstdx[1] << std::endl;
        //std::cout << "checker Evaluate dstdy = " << dstdy[0] << " " << dstdy[1] << std::endl;

        // todo: ClosedForm is not working
        if (aaMethod == AAMethod::None) {
            // Point sample _Checkerboard2DTexture_
            if (((int)std::floor(st[0]) + (int)std::floor(st[1])) % 2 == 0)
                return tex1->Evaluate(si);
            return tex2->Evaluate(si);
        }
        else {
            // Compute closed-form box-filtered _Checkerboard2DTexture_ value

            // Evaluate single check if filter is entirely inside one of them
            float ds = std::max(std::abs(dstdx[0]), std::abs(dstdy[0]));
            float dt = std::max(std::abs(dstdx[1]), std::abs(dstdy[1]));
            float s0 = st[0] - ds, s1 = st[0] + ds;
            float t0 = st[1] - dt, t1 = st[1] + dt;
            if (std::floor(s0) == std::floor(s1) &&
                std::floor(t0) == std::floor(t1)) {

                std::cout << "checker Evaluate in box " << std::endl;

                // Point sample _Checkerboard2DTexture_
                if (((int)std::floor(st[0]) + (int)std::floor(st[1])) % 2 == 0)
                    return tex1->Evaluate(si);
                return tex2->Evaluate(si);
            }

            std::cout << "checker Evaluate not in box " << std::endl;

            // Apply box filter to checkerboard region
            auto bumpInt = [](float x) {
                return (int)std::floor(x / 2) +
                    2 * std::max(x / 2 - (int)std::floor(x / 2) - (float)0.5,
                        (float)0);
            };
            float sint = (bumpInt(s1) - bumpInt(s0)) / (2 * ds);
            float tint = (bumpInt(t1) - bumpInt(t0)) / (2 * dt);
            float area2 = sint + tint - 2 * sint * tint;
            if (ds > 1 || dt > 1) area2 = .5f;
            return (1 - area2) * tex1->Evaluate(si) +
                area2 * tex2->Evaluate(si);
        }
    }

private:
    // Checkerboard2DTexture Private Data
    std::unique_ptr<TextureMapping2D> mapping;
    const std::shared_ptr<Texture<T>> tex1, tex2;
    const AAMethod aaMethod;
};

std::shared_ptr<Texture<Spectrum>> CreateCheckerboardSpectrumTexture(const Transform& tex2world);

#endif