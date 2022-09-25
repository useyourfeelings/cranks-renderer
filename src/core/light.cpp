#include "light.h"
#include "scene.h"

Light::~Light() {}

Light::Light(const std::string& name, int flags, const Transform& LightToWorld, int nSamples)
    : Object(name),
    flags(flags),
    nSamples(std::max(1, nSamples)),
    LightToWorld(LightToWorld),
    WorldToLight(Inverse(LightToWorld)) {
    //++numLights;
}


Spectrum Light::Le(const RayDifferential& ray) const {
    return Spectrum(1);
    return Spectrum(0.f); 
}

// uniform光源分布。每个灯取一样的权重。
UniformLightDistribution::UniformLightDistribution(const Scene& scene) {
    std::vector<float> prob(scene.lights.size(), float(1));
    distrib.reset(new Distribution1D(prob.data(), int(prob.size())));
}

Distribution1D* UniformLightDistribution::Lookup(const Point3f& p) const {
    return distrib.get();
}


std::unique_ptr<LightDistribution> CreateLightSampleDistribution(
    const std::string& name, const Scene& scene) {
    if (name == "uniform" || scene.lights.size() == 1)
        return std::unique_ptr<LightDistribution>{new UniformLightDistribution(scene)};
    
    return nullptr;
    /*else if (name == "power")
        return std::unique_ptr<LightDistribution>{
        new PowerLightDistribution(scene)};
    else if (name == "spatial")
        return std::unique_ptr<LightDistribution>{
        new SpatialLightDistribution(scene)};
    else {
        Error(
            "Light sample distribution type \"%s\" unknown. Using \"spatial\".",
            name.c_str());
        return std::unique_ptr<LightDistribution>{
            new SpatialLightDistribution(scene)};
    }*/
}