#ifndef LIGHT_INFINITE_H
#define LIGHT_INFINITE_H

#include "../core/light.h"
#include "../core/scene.h"
#include "../core/mipmap.h"

class InfiniteAreaLight : public Light {
public:
    // InfiniteAreaLight Public Methods
    InfiniteAreaLight(const json& new_config, const Transform& LightToWorld, const Spectrum& power, float strength,
        int nSamples, const std::string& texmap);
    void Preprocess(const Scene& scene) {
        //scene.WorldBound().BoundingSphere(&worldCenter, &worldRadius);
    }
    Spectrum Power() const;
    Spectrum Le(const RayDifferential& ray) const;
    Spectrum Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf) const;
    float Pdf_Li(const Interaction&, const Vector3f&) const;
    Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, float time,
        Ray* ray, Vector3f* nLight, float* pdfPos,
        float* pdfDir) const;
    void Pdf_Le(const Ray&, const Vector3f&, float* pdfPos, float* pdfDir) const;

private:
    // InfiniteAreaLight Private Data
    std::unique_ptr<MIPMap<RGBSpectrum>> Lmap;
    Point3f worldCenter;
    float worldRadius;
    std::unique_ptr<Distribution2D> distribution;
};

#endif