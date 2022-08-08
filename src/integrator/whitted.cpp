#include "whitted.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"

Spectrum WhittedIntegrator::Li(const RayDifferential& ray, const Scene& scene, Sampler& sampler, int depth) const {
    Log("WhittedIntegrator::Li depth = %d", depth);

    ray.LogSelf();

    Spectrum L(0.);
    // Find closest ray intersection or return background radiance
    SurfaceInteraction isect;
    float tHit;
    if (!scene.Intersect(ray, & tHit, &isect)) {
        return GetFakeSky(ray.d.z);
    }

    Log("scene Intersect");
    //isect.p.LogSelf();
    //Log("primitive = %s", isect.primitive->name.c_str());
    isect.LogSelf();


    if (0) {
        // draw normal

        Spectrum test_spectrum;

        isect.n.LogSelf();

        test_spectrum.c[0] = 0.5 * (isect.n.x + 1); // map to [0, 1]
        test_spectrum.c[1] = 0.5 * (isect.n.y + 1);
        test_spectrum.c[2] = 0.5 * (isect.n.z + 1);

        return test_spectrum;
    }

    // Compute emitted and reflected light at ray intersection point

    // Initialize common variables for Whitted integrator
    //const Normal3f& n = isect.shading.n;
    const Vector3f& n = isect.n;
    Vector3f wo = isect.wo;

    isect.n.LogSelf("n");


    // Compute scattering functions for surface interaction
    // 根据材质添加BxDF。一个材质可能包含多种bxdf。
    // isect.bsdf最终可包含多个bxdf。
    isect.ComputeScatteringFunctions(ray);
    
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


        Log("base Li %f %f %f pdf = %f", Li.c[0], Li.c[1], Li.c[2], pdf);

        
        wo.LogSelf("wo");
        wi.LogSelf("wi");
        

        // 为0的话直接跳过
        if (Li.IsBlack() || pdf == 0)
            continue;

        // 算bsdf里的每个bxdf的f
        // 即光打在点上，对wo方向的贡献。
        Spectrum f = isect.bsdf-> f(wo, wi);

        Log("bsdf f = %f %f %f", f.c[0], f.c[1], f.c[2]);

        // 交点到光源之间是否有阻挡

        if (!f.IsBlack()) {
            //Ray temp_ray(isect.p, wi);

            //Ray temp_ray(light->pos, Normalize(isect.p - light->pos));
            //Ray temp_ray = isect.SpawnRay(Normalize(isect.p - light->pos));
            //Ray temp_ray = isect.SpawnRayThrough(light->pos);

            Ray temp_ray = isect.SpawnRayTo(light->pos);

            Log("temp_ray");
            temp_ray.LogSelf();

            SurfaceInteraction temp_isect;
            float temp_t;
            auto ires = scene.Intersect(temp_ray, &temp_t, &temp_isect);

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
        auto reflect = SpecularReflect(ray, isect, scene, sampler, depth);
        L += reflect;

        // 传输光进行下一层Li
        L += SpecularTransmit(ray, isect, scene, sampler, depth);
    }

    // L的三个部分。直接光，反射光。传输光。
    return L;
}
