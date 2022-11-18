#include "pm.h"
#include "../core/interaction.h"
#include "../core/reflection.h"
#include "../tool/logger.h"
#include "../base/events.h"
#include <random>

PMIntegrator::PMIntegrator(std::shared_ptr<Camera> camera,
    std::shared_ptr<Sampler> sampler
    //const BBox2i& pixelBounds, float rrThreshold,
    //const std::string& lightSampleStrategy
)
    : camera(camera),
    sampler(sampler),
    filter(1),
    emitPhotons(3000),
    gatherPhotons(30),
    gatherPhotonsR(0.1),
    gatherMethod(0),
    energyScale(10000),
    reemitPhotons(1),
    renderDirect(1)
    //rrThreshold(rrThreshold),
    //lightSampleStrategy(lightSampleStrategy) 
    {
    }

void PMIntegrator::SetOptions(const json& data) {
    emitPhotons = data["emitPhotons"];
    gatherPhotons = data["gatherPhotons"];
    gatherPhotonsR = data["gatherPhotonsR"];
    gatherMethod = data["gatherMethod"];
    filter = data["filter"];
    energyScale = data["energyScale"];
    reemitPhotons = data["reemitPhotons"];

    renderDirect = data["renderDirect"];
    renderSpecular = data["renderSpecular"];
    renderCaustic = data["renderCaustic"];
    renderDiffuse = data["renderDiffuse"];
    renderGlobal = data["renderGlobal"];

    specularMethod = data["specularMethod"];
    specularRTSamples = data["specularRTSamples"];
}

json PMIntegrator::GetRenderStatus() {
    json status;

    if (render_status == 1)
        render_duration = (float)(std::chrono::duration<double, std::milli>(std::chrono::high_resolution_clock::now() - render_start_time).count());

    status["render_duration"] = render_duration;

    return status;
}

void PMIntegrator::EmitPhoton(Scene& scene) {
    std::cout << "PMIntegrator::EmitPhoton() emitPhotons = "<< emitPhotons <<" gatherPhotons = "<< gatherPhotons << std::endl;
    
    std::vector<Photon> globalPhotons;
    std::vector<Photon> causticPhotons;
    std::vector<Photon> diffusePhotons;

    MemoryBlock mb;

    // 每个灯发射光子
    for (auto light: scene.lights) {
        std::cout << "light Emit Photon " << std::endl;

        float x, y, z;

        for (int ne = 0; ne < emitPhotons; ++ ne) {
            for (;;) {
                //x = (dis(e) - 0.5) * 2;
                x = (sampler->Get1D() - 0.5) * 2;
                y = (sampler->Get1D() - 0.5) * 2;
                z = (sampler->Get1D() - 0.5) * 2;

                if (x * x + y * y + z * z <= 1)
                    break;
            }

            Vector3f dir = Normalize(Vector3f(x, y, z));

            //std::cout << "photon # " << ne << std::endl;

            //std::cout << "dir = " << dir.x <<" "<< dir.y<<" "<< dir.z << std::endl;

            bool caustic = false;
            RayDifferential ray(light->pos, dir);

            //std::cout << "light pos = " << light->pos.x << " " << light->pos.y << " " << light->pos.z << std::endl;

            Spectrum photonEnergy = light->Le(ray) / emitPhotons;

            // emit to scene
            for (int pathLen = 0; pathLen < 100; ++pathLen) {
                //std::cout << "pathLen " << pathLen << std::endl;

                SurfaceInteraction isect;
                float tHit;
                bool hitScene = scene.Intersect(ray, &tHit, &isect);

                if (!hitScene)
                    break;

                // 根据material添加bxdf
                isect.ComputeScatteringFunctions(mb, ray);

                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION)) > 0) { // BSDF_GLOSSY | BSDF_REFLECTION
                //if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR)) > 0 ||
                //    isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY)) > 0) {

                    // 一旦碰到specular，就开始一个caustic。
                    // 一整条路径中可包含多个caustic。目前这样实现。
                    caustic = true;
                }
                else {
                    // 碰到diffuse，形成一个子路径。检查是否为caustic。
                    if (caustic) {
                        causticPhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    }
                    else {
                        diffusePhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    }

                    globalPhotons.push_back(Photon({ isect.p, dir, photonEnergy }));
                    //std::cout << "globalPhotons push " << photonEnergy.c[0] << " " << photonEnergy.c[1] << " " << photonEnergy.c[2] << std::endl;

                    caustic = false; // 重新开始
                }

                //if (sampler->Get1D() < 0.1667) {
                //    //std::cout << "rr break" << std::endl;
                //    break;
                //}
                    

                Vector3f wo = -ray.d, wi;
                float pdf;
                BxDFType flags;

                // 以wo采一束入射光，算出f。
                // 同时得到一个随机方向，作为下一层的path。
                Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                if (f.IsBlack() || pdf == 0.f)
                    break;

                // 更新光子能量
                photonEnergy *= (f * AbsDot(wo, isect.shading.n) / pdf);

                // rr
                float q = (1 - photonEnergy.MaxComponentValue() * emitPhotons);
                if (q > 0.8) {
                    if (sampler->Get1D() < q) {
                        //std::cout << "rr break q = " << q << " pathLen = " << pathLen << std::endl;
                        break;
                    }

                    photonEnergy /= 1 - q;
                }

                ray = isect.SpawnRay(wi);

                dir = Normalize(wi);
            }

        }
    }

    std::cout << "globalPhotons size = "<< globalPhotons.size() << std::endl;
    std::cout << "causticPhotons size = " << causticPhotons.size() << std::endl;
    std::cout << "diffusePhotons size = " << diffusePhotons.size() << std::endl;

    globalPhotonMap.Build(globalPhotons);
    causticPhotonMap.Build(causticPhotons);
    diffusePhotonMap.Build(diffusePhotons);
}

void PMIntegrator::RenderPhoton(Scene& scene) {
    std::cout << "PMIntegrator::RenderPhoton() emitPhotons = " << emitPhotons << " gatherPhotons = " << gatherPhotons << std::endl;

    //Log("Render");
    render_status = 1;
    has_new_photo = 0;
    //Preprocess(scene, *sampler);

    BBox2i sampleBounds = BBox2i(Point2i(0, 0), Point2i(camera->resolutionX, camera->resolutionY));
    // sampleBounds 类似600 400

    this->render_progress_now.resize(render_threads_count);
    this->render_progress_total.resize(render_threads_count);
    for (int i = 0; i < render_threads_count; ++i) {
        render_progress_now[i] = 0;
    }

    MultiTaskArg tast_args;
    tast_args.task_count = render_threads_count;
    tast_args.x_start = sampleBounds.pMin.x;
    tast_args.y_start = sampleBounds.pMin.y;
    tast_args.x_end = sampleBounds.pMax.x - 1;
    tast_args.y_end = sampleBounds.pMax.y - 1;

    // https://stackoverflow.com/questions/4324763/can-we-have-functions-inside-functions-in-c
    // https://en.cppreference.com/w/cpp/language/lambda

    // 改为竖着分像素。一般会更均匀。
    /*auto manager_func = [&](int task_index, json args) {
        int x_interval = (int(args["x_end"]) - args["x_start"] + 1) / args["task_count"];
        if ((int(args["x_end"]) - args["x_start"] + 1) % args["task_count"] != 0)
            x_interval++;

        int x_start = args["x_start"] + x_interval * task_index;
        int x_end = std::min(int(args["x_end"]), int(x_start + x_interval - 1));

        return json({
            { "task_index", task_index },
            { "task_progress_total", (x_end - x_start + 1) * (int(args["y_end"]) - args["y_start"] + 1)},
            { "y_start", args["y_start"]},
            { "x_start", x_start },
            { "y_end", args["y_end"] },
            { "x_end", x_end} });
    };*/

    auto manager_func = [](int task_index, MultiTaskArg args) {
        int x_interval = (args.x_end - args.x_start + 1) / args.task_count;
        if ((args.x_end - args.x_start + 1) % args.task_count != 0)
            x_interval++;

        int x_start = args.x_start + x_interval * task_index;
        int x_end = std::min(args.x_end, __int64(x_start + x_interval - 1));

        args.task_index = task_index;
        args.task_progress_total = (x_end - x_start + 1) * args.y_end - args.y_start + 1;
        args.y_start = args.y_start;
        args.x_start = x_start;
        args.y_end = args.y_end;
        args.x_end = x_end;

        return args;

        /*return json({
            { "task_index", task_index },
            { "task_progress_total", (x_end - x_start + 1) * (int(args["y_end"]) - args["y_start"] + 1)},
            { "y_start", args["y_start"]},
            { "x_start", x_start },
            { "y_end", args["y_end"] },
            { "x_end", x_end} });*/
    };

    auto render_task = [&](MultiTaskArg args) {
        int i = args.x_start;
        int j = args.y_start;
        int task_index = args.task_index;

        std::cout << "--- render_task " << task_index << " " << i << " " << j << std::endl;
        //std::cout << args << std::endl;

        MemoryBlock mb;

        this->render_progress_total[task_index] = args.task_progress_total;

        //RandomSampler sampler(1);

        int progress_interval = args.task_progress_total / 100;
        int progress = 0;

        // Loop over pixels in tile to render them
        //while (i < sampleBounds.pMax.x && j < sampleBounds.pMax.y) {
        for (int j = args.y_start; j <= args.y_end; ++j) {
            for (int i = args.x_start; i <= args.x_end; ++i) {

                if (render_status == 0)
                    break;

                progress++;

                //Log("Render %d %d", i, j);
                //std::cout << "Render " << i << " " << j << std::endl;
                Point2i pixel(i, j);

                sampler->StartPixel(pixel);

                if (progress % progress_interval == 0) {
                    this->render_progress_now[task_index] = progress;
                }

                do {
                    // Initialize _CameraSample_ for current sample
                    CameraSample cameraSample = sampler->GetCameraSample(pixel);

                    // Generate camera ray for current sample
                    RayDifferential ray;
                    float rayWeight = camera->GenerateRayDifferential(cameraSample, &ray);
                    //ray.ScaleDifferentials(1 / std::sqrt((float)local_sampler->samplesPerPixel));

                    Spectrum L(0.f);
                    if (rayWeight > 0)
                        L = Li(mb, ray, scene, 0);

                    camera->AddSample(Point2f(pixel.x, pixel.y), L, rayWeight, sampler->samplesPerPixel);

                    mb.Reset();
                } while (sampler->StartNextSample());
            }
        }
    };

    StartMultiTask(render_task, manager_func, tast_args);

    // Save final image after rendering
    camera->film->WriteImage();
    camera->films.push_back(camera->film);
    render_status = 0;
    has_new_photo = 1;
}


Spectrum PMIntegrator::Li(MemoryBlock &mb, const RayDifferential& ray, Scene& scene, int depth, int samplingFlag) {
    Spectrum L(0.f);

    SurfaceInteraction isect;
    float tHit;
    bool hitScene = scene.Intersect(ray, &tHit, &isect);

    if (hitScene) {
        isect.ComputeScatteringFunctions(mb, ray);

        // direct
        if (renderDirect) {
            // Sample illumination from lights to find path contribution.
            // (But skip this for perfectly specular BSDFs.)
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY))) > 0) {
                // 计算直接光。
                // 对所有光源采样，选出一个光源，计算bsdf。得到最后的光能。

                auto distrib = lightDistribution->Lookup(isect.p);
                Spectrum Ld = UniformSampleOneLight(isect, scene, *(sampler->Clone()), false, distrib);
                L += Ld;
            }
        }

        // specular
        if (renderSpecular && depth < maxDepth) { // && sampler->Get1D() > 0.1667
            if (specularMethod == 0 && depth >= 3) {
                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0 ||
                    isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION))
                    ) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    if (!samplingFlag) {
                        for (int i = 0; i < specularRTSamples; ++i) {
                            // 以wo采一束入射光，算出f。
                            // 同时得到一个随机方向，作为下一层的path。
                            Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL), &flags);

                            if (f.IsBlack() || pdf == 0.f) {
                            }
                            else {
                                Ls += f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, depth + 1, 1) / pdf;
                            }
                        }

                        L += (Ls / specularRTSamples);
                    }
                    else {
                        // 以wo采一束入射光，算出f。
                            // 同时得到一个随机方向，作为下一层的path。
                        Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL), &flags);

                        if (f.IsBlack() || pdf == 0.f) {
                        }
                        else {
                            Ls += f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, depth + 1, 1) / pdf;
                        }

                        L += Ls;
                    }
                    
                }
            }
            else {
                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_REFLECTION)) > 0) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    // 以wo采一束入射光，算出f。
                    // 同时得到一个随机方向，作为下一层的path。
                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_SPECULAR | BSDF_REFLECTION), &flags);

                    if (f.IsBlack() || pdf == 0.f) {
                    }
                    else {
                        Ls = f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, depth + 1, samplingFlag) / pdf;
                    }

                    L += Ls;
                }

                if (isect.bsdf->NumComponents(BxDFType(BSDF_GLOSSY | BSDF_REFLECTION)) > 0) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    // 以wo采一束入射光，算出f。
                    // 同时得到一个随机方向，作为下一层的path。
                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_GLOSSY | BSDF_REFLECTION), &flags);

                    if (f.IsBlack() || pdf == 0.f) {
                    }
                    else {
                        Ls = f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, depth + 1, samplingFlag) / pdf;
                    }

                    L += Ls;
                }

                if (isect.bsdf->NumComponents(BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION)) > 0) {

                    Spectrum Ls(0.f);

                    Vector3f wo = -ray.d, wi;
                    float pdf;
                    BxDFType flags;

                    // 以wo采一束入射光，算出f。
                    // 同时得到一个随机方向，作为下一层的path。
                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_SPECULAR | BSDF_TRANSMISSION), &flags);

                    if (f.IsBlack() || pdf == 0.f) {
                    }
                    else {
                        Ls = f * AbsDot(wi, isect.shading.n) * Li(mb, isect.SpawnRay(wi), scene, depth + 1, samplingFlag) / pdf;
                    }

                    L += Ls;
                }
            }
        }

        // caustic
        if (renderCaustic) {
            Spectrum L_caustic(0.f);

            int near = gatherPhotons;
            if (gatherMethod == 1)
                near = int(float(near) * causticPhotonMap.Size() / 1000);

            auto knnResult = causticPhotonMap.KNN(isect.p, near, gatherPhotonsR * gatherPhotonsR);

            // for cone filter
            float r = std::sqrt(knnResult.resultMaxDistance2);
            float k = 1.1f;

            for (auto photon : knnResult.payloads) {
                Vector3f wo = -photon.dir, wi;
                float pdf;
                BxDFType flags;

                Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY)), &flags); // BSDF_ALL

                if (f.IsBlack() || pdf == 0.f)
                    continue;

                auto photonEnergy = f * AbsDot(wi, isect.shading.n) * photon.energy * energyScale / pdf;

                if (filter == 1) {
                    photonEnergy *= (1.f - Distance(photon.pos, isect.p) / (k * r));
                }

                L_caustic += photonEnergy;
            }

            if (filter == 0) {
                L_caustic /= (Pi * knnResult.resultMaxDistance2);
            }
            else if (filter == 1) {
                L_caustic /= (Pi * knnResult.resultMaxDistance2 * (1.f - 2.f / (k * 3)));
            }

            L += L_caustic;

        }

        // diffuse
        if (renderDiffuse) {
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL & ~(BSDF_SPECULAR | BSDF_GLOSSY))) > 0) {

                Spectrum L_caustic(0.f);

                int near = gatherPhotons;
                if (gatherMethod == 1)
                    near = int(float(near) * causticPhotonMap.Size() / 1000);

                auto knnResult = diffusePhotonMap.KNN(isect.p, near, gatherPhotonsR * gatherPhotonsR);

                // for cone filter
                float r = std::sqrt(knnResult.resultMaxDistance2);
                float k = 1.1f;

                for (auto photon : knnResult.payloads) {
                    Vector3f wo = -photon.dir, wi;
                    float pdf;
                    BxDFType flags;

                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                    if (f.IsBlack() || pdf == 0.f)
                        continue;

                    auto photonEnergy = f * AbsDot(wi, isect.shading.n) * photon.energy * energyScale / pdf;

                    if (filter == 1) {
                        photonEnergy *= (1.f - Distance(photon.pos, isect.p) / (k * r));
                    }

                    L_caustic += photonEnergy;
                }

                if (filter == 0) {
                    L_caustic /= (Pi * knnResult.resultMaxDistance2);
                }
                else if (filter == 1) {
                    L_caustic /= (Pi * knnResult.resultMaxDistance2 * (1.f - 2.f / (k * 3)));
                }

                L += L_caustic;
            }
        }

        // global
        if (renderGlobal) {
            if (isect.bsdf->NumComponents(BxDFType(BSDF_ALL)) > 0) { //  & ~(BSDF_SPECULAR | BSDF_GLOSSY

                Spectrum L_caustic(0.f);

                int near = gatherPhotons;
                if (gatherMethod == 1)
                    near = int(float(near) * causticPhotonMap.Size() / 1000);

                auto knnResult = globalPhotonMap.KNN(isect.p, near, gatherPhotonsR * gatherPhotonsR);

                // for cone filter
                float r = std::sqrt(knnResult.resultMaxDistance2);
                float k = 1.1f;

                //std::cout << knnResult.payloads.size() << " "<< knnResult.resultMaxDistance2<< std::endl;

                for (auto photon : knnResult.payloads) {
                    Vector3f wo = -photon.dir, wi;
                    float pdf;
                    BxDFType flags;

                    Spectrum f = isect.bsdf->Sample_f(wo, &wi, sampler->Get2D(), &pdf, BSDF_ALL, &flags);

                    if (f.IsBlack() || pdf == 0.f)
                        continue;

                    auto photonEnergy = f * AbsDot(wi, isect.shading.n) * photon.energy * energyScale / pdf;

                    if (filter == 1) {
                        photonEnergy *= (1.f - Distance(photon.pos, isect.p) / (k * r));
                    }

                    L_caustic += photonEnergy;
                }

                if (filter == 0) {
                    L_caustic /= (Pi * knnResult.resultMaxDistance2);
                }
                else if (filter == 1) {
                    L_caustic /= (Pi * knnResult.resultMaxDistance2 * (1.f - 2.f / (k * 3)));
                }

                L += L_caustic;
            }
        }

        //std::cout << i << " " << j << " " << L.c[0] <<" "<< L.c[1] << " " << L.c[2] << std::endl;
    }

    return L;
}

void PMIntegrator::Render(Scene& scene) {
    std::cout << "PMIntegrator::Render() " << std::endl;

    render_start_time = std::chrono::high_resolution_clock::now();

    lightDistribution = CreateLightSampleDistribution("uniform", scene);

    if (reemitPhotons) {
        EmitPhoton(scene);
    }
    
    RenderPhoton(scene);
}