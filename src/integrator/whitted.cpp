#include "whitted.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"

Spectrum WhittedIntegrator::Li(const RayDifferential& ray, const Scene& scene, Sampler& sampler, int depth) const {
    Log("WhittedIntegrator::Li");

    ray.LogSelf();

    Spectrum L(0.);
    // Find closest ray intersection or return background radiance
    SurfaceInteraction isect;
    float tHit;
    if (!scene.Intersect(ray, & tHit, &isect)) {
        Spectrum sky_spectrum;
        /*sky_spectrum.c[0] = 0.6;
        sky_spectrum.c[1] = 0.7;
        sky_spectrum.c[2] = 0.9;*/

        // https://raytracing.github.io/books/RayTracingInOneWeekend.html
        // gradient sky

        float t = (ray.d.y + 1) * 0.5; // map to [0, 1]
        /*sky_spectrum.c[0] = 1 - t + t * 0.5;
        sky_spectrum.c[1] = 1 - t + t * 0.7;
        sky_spectrum.c[2] = 1 - t + t * 1;*/

        sky_spectrum.c[0] = 1 - t * 0.5;
        sky_spectrum.c[1] = 1 - t * 0.3;
        sky_spectrum.c[2] = 1;

        return sky_spectrum;


        for (const auto& light : scene.lights) L += light->Le(ray);
        return L;
    }

    Log("scene Intersect at");
    isect.p.LogSelf();
    Log("primitive = %s", isect.primitive->name.c_str());

    // draw normal
    Spectrum test_spectrum;
    /*test_spectrum.c[0] = 0.8;
    test_spectrum.c[1] = 0.3;
    test_spectrum.c[2] = 0.3;*/

    isect.n.LogSelf();

    test_spectrum.c[0] = 0.5 * (isect.n.x + 1); // map to [0, 1]
    test_spectrum.c[1] = 0.5 * (isect.n.y + 1);
    test_spectrum.c[2] = 0.5 * (isect.n.z + 1);

    return test_spectrum;

    // Compute emitted and reflected light at ray intersection point

    // Initialize common variables for Whitted integrator
    //const Normal3f& n = isect.shading.n;
    const Vector3f& n = isect.n;
    Vector3f wo = isect.wo;

    // Compute scattering functions for surface interaction
    isect.ComputeScatteringFunctions(ray);
    
    //if (!isect.bsdf)
    //    return Li(isect.SpawnRay(ray.d), scene, sampler, arena, depth);

    // Compute emitted light if ray hit an area light source
    //L += isect.Le(wo);

    // Add contribution of each light source
    for (const auto& light : scene.lights) {
        Vector3f wi;
        float pdf;
        //VisibilityTester visibility;


        Spectrum Li = light->Sample_Li(isect, &wi, &pdf);
        if (Li.IsBlack() || pdf == 0) continue;
        Spectrum f = isect.bsdf->f(wo, wi);

        // 交点到光源之间是否有阻挡

        if (!f.IsBlack()) {
            Ray temp_ray(isect.p, wi);
            SurfaceInteraction temp_isect;
            float temp_t;
            scene.Intersect(temp_ray, &temp_t, &temp_isect);

            // todo: two sides. compare roots
            if(temp_isect.primitive->id == isect.primitive->id)
                L += f * Li * std::abs(Dot(wi, n)) / pdf;
        }
            
    }
    if (depth + 1 < maxDepth) {
        // Trace rays for specular reflection and refraction
        L += SpecularReflect(ray, isect, scene, sampler, depth);
        //L += SpecularTransmit(ray, isect, scene, sampler, depth);
    }
    return L;
}


Spectrum SamplerIntegrator::SpecularReflect(
    const RayDifferential& ray, const SurfaceInteraction& isect,
    const Scene& scene, Sampler& sampler, int depth) const {
    // Compute specular reflection direction _wi_ and BSDF value
    Vector3f wo = isect.wo, wi;
    float pdf;
    BxDFType type = BxDFType(BSDF_REFLECTION | BSDF_SPECULAR);
    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, type);

    // Return contribution of specular reflection
    const Vector3f& ns = isect.shading.n;
    if (pdf > 0.f && !f.IsBlack() && std::abs(Dot(wi, ns)) != 0.f) {
        // Compute ray differential _rd_ for specular reflection
        RayDifferential rd = isect.SpawnRay(wi);
        //if (ray.hasDifferentials) {
        //    rd.hasDifferentials = true;
        //    rd.rxOrigin = isect.p + isect.dpdx;
        //    rd.ryOrigin = isect.p + isect.dpdy;
        //    // Compute differential reflected directions
        //    Vector3f dndx = isect.shading.dndu * isect.dudx +
        //        isect.shading.dndv * isect.dvdx;
        //    Vector3f dndy = isect.shading.dndu * isect.dudy +
        //        isect.shading.dndv * isect.dvdy;
        //    Vector3f dwodx = -ray.rxDirection - wo,
        //        dwody = -ray.ryDirection - wo;
        //    float dDNdx = Dot(dwodx, ns) + Dot(wo, dndx);
        //    float dDNdy = Dot(dwody, ns) + Dot(wo, dndy);
        //    rd.rxDirection =
        //        wi - dwodx + 2.f * Vector3f(Dot(wo, ns) * dndx + dDNdx * ns);
        //    rd.ryDirection =
        //        wi - dwody + 2.f * Vector3f(Dot(wo, ns) * dndy + dDNdy * ns);
        //}
        return f * Li(rd, scene, sampler, depth + 1) * std::abs(Dot(wi, ns)) / pdf;
    }
    else
        return Spectrum(0.f);
}