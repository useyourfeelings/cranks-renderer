#include "whitted.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"

void WhittedIntegrator::SetOptions(const json& data) {
    sampler->SetSamplesPerPixel(data["ray_sample_no"]);
    SetRayBounceNo(data["ray_bounce_no"]);
    SetRenderThreadsCount(data["render_threads_no"]);
}

Spectrum WhittedIntegrator::Li(MemoryBlock& mb, const RayDifferential& ray, Scene& scene, Sampler& sampler, int pool_id, int depth) {
    //Log("WhittedIntegrator::Li depth = %d", depth);

    //ray.LogSelf();

    Spectrum L(0.);
    // Find closest ray intersection or return background radiance
    SurfaceInteraction isect;
    float tHit;
    if (!scene.Intersect(ray, & tHit, &isect, pool_id)) {
        return GetFakeSky(ray.d.z);
    }

    // Compute emitted and reflected light at ray intersection point

    // Initialize common variables for Whitted integrator
    //const Normal3f& n = isect.shading.n;
    const Vector3f& n = isect.n;
    Vector3f wo = isect.wo;

    // Compute scattering functions for surface interaction
    // 根据材质添加BxDF。一个材质可能包含多种bxdf。
    // isect.bsdf最终可包含多个bxdf。
    isect.ComputeScatteringFunctions(mb, ray);
    
    //if (!isect.bsdf)
    //    return Li(isect.SpawnRay(ray.d), scene, sampler, arena, depth);

    // Compute emitted light if ray hit an area light source
    //L += isect.Le(wo);

    // Add contribution of each light source
    // 计算每个灯打过来的光。直接光。
    for (const auto& light : scene.lights) {
        Vector3f wi;
        float pdf;
        //VisibilityTester visibility;

        // 光源的基本值
        Spectrum Li = light->Sample_Li(isect, sampler.Get2D(), &wi, &pdf); // for point light

        //Log("base Li %f %f %f pdf = %f", Li.c[0], Li.c[1], Li.c[2], pdf);

        // 为0的话直接跳过
        if (Li.IsBlack() || pdf == 0)
            continue;

        // 算bsdf里的每个bxdf的f
        // 即光打在点上，对wo方向的贡献。
        Spectrum f = isect.bsdf-> f(wo, wi);

        //Log("bsdf f = %f %f %f", f.c[0], f.c[1], f.c[2]);

        // 交点到光源之间是否有阻挡
        if (!f.IsBlack()) {
            //Ray temp_ray(isect.p, wi);

            //Ray temp_ray(light->pos, Normalize(isect.p - light->pos));
            //Ray temp_ray = isect.SpawnRay(Normalize(isect.p - light->pos));
            //Ray temp_ray = isect.SpawnRayThrough(light->pos);

            Ray temp_ray = isect.SpawnRayTo(light->pos);

            //temp_ray.LogSelf();

            SurfaceInteraction temp_isect;
            float temp_t;
            auto ires = scene.Intersect(temp_ray, &temp_t, &temp_isect, pool_id);

            if (!ires) {
                L += f * Li * std::abs(Dot(wi, n)) / pdf;

                //auto add = f * Li * std::abs(Dot(wi, n)) / pdf;
                //Log("light can see. add %f %f %f", add.c[0], add.c[1], add.c[2]);
            }
        }
            
    }

    if (depth + 1 < maxDepth) {
        // Trace rays for specular reflection and refraction

        // WhittedIntegrator 只能处理 BxDFType(BSDF_REFLECTION | BSDF_SPECULAR)
        // 反射光进行下一层Li
        L += SpecularReflect(mb, ray, isect, scene, sampler, pool_id, depth);

        // 传输光进行下一层Li
        L += SpecularTransmit(mb, ray, isect, scene, sampler, pool_id, depth);
    }

    // L的三个部分。直接光，反射光。传输光。
    return L;
}
