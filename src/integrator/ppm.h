#ifndef CORE_INTEGRATOR_PPM_H
#define CORE_INTEGRATOR_PPM_H

#include "../core/geometry.h"
#include "../core/integrator.h"
#include "../core/light.h"
#include "../core/kdtree.h"
#include "../core/camera.h"
#include "../core/film.h"
#include<mutex>

struct Hitpoint {
    Point3f pos; // position
    Vector3f dir; // incident dir
    Vector3f normal;
    //std::shared_ptr<BSDF> bsdf;
    //SurfaceInteraction isect;
    std::shared_ptr<SurfaceInteraction> isect;

    // [0]-caustic [1]-diffuse [2]-global
    float r[3]; // radius
    Spectrum energy[3];
    Spectrum energyWeight;
    int photonCount[3];
    
    float x; // screen x
    float y; // screen y

    Hitpoint() {
    }

    Hitpoint(const Hitpoint& from) {
        *this = from;
    }

    Hitpoint& operator=(const Hitpoint& from) {
        
        pos = from.pos;
        dir = from.dir;
        normal = from.normal;
        r[0] = from.r[0];
        r[1] = from.r[1];
        r[2] = from.r[2];
        energy[0] = from.energy[0];
        energy[1] = from.energy[1];
        energy[2] = from.energy[2];
        photonCount[0] = from.photonCount[0];
        photonCount[1] = from.photonCount[1];
        photonCount[2] = from.photonCount[2];
        energyWeight = from.energyWeight;
        isect = from.isect;
        y = from.y;
        x = from.x;

        return *this;
    }
};

class PPMIntegrator : public Integrator {
public:
    PPMIntegrator(std::shared_ptr<Camera> camera,
        std::shared_ptr<Sampler> sampler
        //const BBox2i& pixelBounds, float rrThreshold = 1,
        //const std::string& lightSampleStrategy = "uniform"
    );

    void Render(Scene& scene);
    void SetOptions(const json& data);
    json GetRenderStatus();

private:
    void TraceRay(Scene& scene, std::vector<MemoryBlock>& mbs);
    void TraceARay(MemoryBlock& mb, const RayDifferential& ray, Scene& scene, Point2i pixel, Point2i resolution, int depth, Spectrum energyWeight, int pool_id);

    void UpdateHitpoint(Scene& scene, int i, int pool_id);

    std::vector<Hitpoint> hitpoints; // x * y pixels

    std::shared_ptr<Camera> camera;
    std::shared_ptr<Sampler> sampler;

    //

    void EmitPhoton(Scene& scene);
    void RenderPhoton(Scene& scene);
    Spectrum Li(const RayDifferential& ray, const Scene& scene, int depth);

    
    //const float rrThreshold;
    //const std::string lightSampleStrategy;
    std::unique_ptr<LightDistribution> lightDistribution;
    int maxDepth = 10;

    int filter; // 0-none 1-cone 2-gaussian
    int totalPhotons;
    int emitPhotons;
    //int gatherPhotons;
    //float gatherPhotonsR;
    //int gatherMethod;
    float energyScale;
    //int reemitPhotons;

    int renderDirect = 1;
    //int renderSpecular = 1;
    int renderCaustic = 1;
    int renderDiffuse = 1;
    int renderGlobal = 0;

    //int specularMethod = 0; // 0-monte carlo 1-whitted
    //int specularRTSamples = 5;

    KDTree<Photon> causticPhotonMap;
    KDTree<Photon> diffusePhotonMap;
    KDTree<Photon> globalPhotonMap;

    // ppm
    int maxIterations;
    int currentIteration;
    float initalRadius;
    float alpha = 0.5;

    std::mutex hitpointsMutex;
};


#endif