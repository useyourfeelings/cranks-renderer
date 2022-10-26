#ifndef LIGHT_POINT_H
#define LIGHT_POINT_H

#include "../core/light.h"

class PointLight : public Light {
public:
    // PointLight Public Methods
    PointLight(const std::string &name, const Transform& LightToWorld, const Spectrum& I)
        : Light(name, (int)LightFlags::DeltaPosition, LightToWorld),
        pLight(LightToWorld(Point3f(0, 0, 0))),
        I(I) {
        pos = LightToWorld(Point3f(0, 0, 0));
    }
    Spectrum Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf) const;
    /*Spectrum Power() const;
    Float Pdf_Li(const Interaction&, const Vector3f&) const;
    Spectrum Sample_Le(const Point2f& u1, const Point2f& u2, Float time,
        Ray* ray, Normal3f* nLight, Float* pdfPos,
        Float* pdfDir) const;
    void Pdf_Le(const Ray&, const Normal3f&, Float* pdfPos,
        Float* pdfDir) const;*/

    Spectrum Le(const RayDifferential& ray) const;

    Point3f pLight;

private:
    // PointLight Private Data
    
    const Spectrum I;
};

#endif