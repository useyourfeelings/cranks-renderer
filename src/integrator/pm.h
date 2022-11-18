#ifndef CORE_INTEGRATOR_PM_H
#define CORE_INTEGRATOR_PM_H

#include "../core/geometry.h"
#include "../core/integrator.h"
#include "../core/light.h"
#include "../core/kdtree.h"
#include "../core/camera.h"
#include "../core/film.h"

//struct Photon {
//    Point3f pos; // position
//    Vector3f dir; // incident dir
//    Spectrum energy;
//};

class PMIntegrator : public Integrator {
public:
    PMIntegrator(std::shared_ptr<Camera> camera,
        std::shared_ptr<Sampler> sampler
        //const BBox2i& pixelBounds, float rrThreshold = 1,
        //const std::string& lightSampleStrategy = "uniform"
    );

    void Render(Scene& scene);
    void SetOptions(const json& data);

private:
    json GetRenderStatus();
    void EmitPhoton(Scene& scene);
    void RenderPhoton(Scene& scene);
    Spectrum Li(MemoryBlock& mb, const RayDifferential& ray, Scene& scene, int depth, int samplingFlag = 0);

    std::shared_ptr<Camera> camera;
    std::shared_ptr<Sampler> sampler;
    //const float rrThreshold;
    //const std::string lightSampleStrategy;
    std::unique_ptr<LightDistribution> lightDistribution;
    int maxDepth = 10;

    int filter; // 0-none 1-cone 2-gaussian
    int emitPhotons;
    int gatherPhotons;
    float gatherPhotonsR;
    int gatherMethod;
    float energyScale;
    int reemitPhotons;

    int renderDirect = 1;
    int renderSpecular = 1;
    int renderCaustic = 1;
    int renderDiffuse = 1;
    int renderGlobal = 0;

    int specularMethod = 0; // 0-monte carlo 1-whitted
    int specularRTSamples = 5;

    KDTree<Photon> causticPhotonMap;
    KDTree<Photon> diffusePhotonMap;
    KDTree<Photon> globalPhotonMap;
};


#endif