#include "light.h"

Light::~Light() {}

Light::Light(int flags, const Transform& LightToWorld, int nSamples)
    : flags(flags),
    nSamples(std::max(1, nSamples)),
    LightToWorld(LightToWorld),
    WorldToLight(Inverse(LightToWorld)) {
    //++numLights;
}


Spectrum Light::Le(const RayDifferential& ray) const {
    return Spectrum(0.8);
    return Spectrum(0.f); 
}