#include "integrator.h"
#include "../core/reflection.h"
#include "film.h"
#include "../tool/logger.h"
#include <iostream>

void SamplerIntegrator::Render(const Scene& scene) {
    Log("Render");
    render_status = 1;
    has_new_photo = 0;
    //Preprocess(scene, *sampler);
    // Render image tiles in parallel

    // Compute number of tiles, _nTiles_, to use for parallel rendering
    //BBox2i sampleBounds = this->pixelBounds;// camera->film->GetSampleBounds();
    BBox2i sampleBounds = BBox2i(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));
    // sampleBounds ÀàËÆ600 400

    this->render_progress_total = camera->resolutionX * camera->resolutionY;
    this->render_progress_now = 0;

    int i = sampleBounds.pMin.x;
    int j = sampleBounds.pMin.y;

    // Loop over pixels in tile to render them
    while (i < sampleBounds.pMax.x && j < sampleBounds.pMax.y) {
        if (render_status == 0)
            break;

        Log("Render %d %d", i, j);
        //std::cout << "Render " << i << " " << j << std::endl;
        Point2i pixel(i, j);

        this->sampler->StartPixel(pixel);

        this->render_progress_now++;

        // Do this check after the StartPixel() call; this keeps
        // the usage of RNG values from (most) Samplers that use
        // RNGs consistent, which improves reproducability /
        // debugging.
        //if (!InsideExclusive(pixel, pixelBounds))
        //    continue;

        do {
            // Initialize _CameraSample_ for current sample
            CameraSample cameraSample = this->sampler->GetCameraSample(pixel);

            // Generate camera ray for current sample
            RayDifferential ray;
            float rayWeight = camera->GenerateRayDifferential(cameraSample, &ray);
            ray.ScaleDifferentials(1 / std::sqrt((float)sampler->samplesPerPixel));

            //++nCameraRays;

            // Evaluate radiance along camera ray
            Spectrum L(0.f);
            if (rayWeight > 0)
                L = Li(ray, scene, *sampler);

            // Issue warning if unexpected radiance value returned
            /* if (L.HasNaNs()) {
                LOG(ERROR) << StringPrintf(
                    "Not-a-number radiance value returned "
                    "for pixel (%d, %d), sample %d. Setting to black.",
                    pixel.x, pixel.y,
                    (int)tileSampler->CurrentSampleNumber());
                L = Spectrum(0.f);
            }
            else if (L.y() < -1e-5) {
                LOG(ERROR) << StringPrintf(
                    "Negative luminance value, %f, returned "
                    "for pixel (%d, %d), sample %d. Setting to black.",
                    L.y(), pixel.x, pixel.y,
                    (int)tileSampler->CurrentSampleNumber());
                L = Spectrum(0.f);
            }
            else if (std::isinf(L.y())) {
                LOG(ERROR) << StringPrintf(
                    "Infinite luminance value returned "
                    "for pixel (%d, %d), sample %d. Setting to black.",
                    pixel.x, pixel.y,
                    (int)tileSampler->CurrentSampleNumber());
                L = Spectrum(0.f);
            }
            VLOG(1) << "Camera sample: " << cameraSample << " -> ray: " <<
                ray << " -> L = " << L;
            */

            // Add camera ray's contribution to image
            //filmTile->AddSample(cameraSample.pFilm, L, rayWeight);

            camera->AddSample(Point2f(pixel.x, pixel.y), L, rayWeight, sampler->samplesPerPixel);

            // Free _MemoryArena_ memory from computing image sample
            // value
            //arena.Reset();
        } while (sampler->StartNextSample());

        if (++i >= sampleBounds.pMax.x) {
            i = 0;
            ++j;
        }
    }

    // Save final image after rendering
    camera->film->WriteImage();
    render_status = 0;
    has_new_photo = 1;
}

Spectrum SamplerIntegrator::SpecularReflect(
    const RayDifferential& ray, const SurfaceInteraction& isect,
    const Scene& scene, Sampler& sampler, int depth) const {

    Log("SpecularReflect");

    // Compute specular reflection direction _wi_ and BSDF value
    Vector3f wo = isect.wo;
    Vector3f wi;
    float pdf;
    BxDFType type = BxDFType(BSDF_REFLECTION | BSDF_SPECULAR);
    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler.Get2D(), &pdf, type);

    // Return contribution of specular reflection
    const Vector3f& ns = isect.shading.n;
    if (pdf > 0.f && !f.IsBlack() && std::abs(Dot(wi, ns)) != 0.f) {
        Log("SpecularReflect continue");

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