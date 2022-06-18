#include "integrator.h"
#include "film.h"
#include "../tool/logger.h"

void SamplerIntegrator::Render(const Scene& scene) {
    Log("Render");
    //Preprocess(scene, *sampler);
    // Render image tiles in parallel

    // Compute number of tiles, _nTiles_, to use for parallel rendering
    //BBox2i sampleBounds = this->pixelBounds;// camera->film->GetSampleBounds();
    BBox2i sampleBounds = BBox2i(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));
    // sampleBounds ¿‡À∆600 400

    int i = sampleBounds.pMin.x;
    int j = sampleBounds.pMin.y;

    // Loop over pixels in tile to render them
    while (i < sampleBounds.pMax.x && j < sampleBounds.pMax.y) {
        Log("Render %d %d", i, j);
        Point2i pixel(i, j);

        this->sampler->StartPixel(pixel);

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

            camera->AddSample(Point2f(pixel.x, pixel.y), L, rayWeight);

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
}