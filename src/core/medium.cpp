#include "medium.h"
#include "interaction.h"

float HenyeyGreenstein::Sample_p(const Vector3f& wo, Vector3f* wi, const Point2f& u) const {
    //ProfilePhase _(Prof::PhaseFuncSampling);
    // Compute $\cos \theta$ for Henyey--Greenstein sample
    float cosTheta;
    if (std::abs(g) < 1e-3)
        cosTheta = 1 - 2 * u[0];
    else {
        float sqrTerm = (1 - g * g) / (1 - g + 2 * g * u[0]);
        cosTheta = (1 + g * g - sqrTerm * sqrTerm) / (2 * g);
    }

    // Compute direction _wi_ for Henyey--Greenstein sample
    float sinTheta = std::sqrt(std::max((float)0, 1 - cosTheta * cosTheta));
    float phi = 2 * Pi * u[1];
    Vector3f v1, v2;
    CoordinateSystem(wo, &v1, &v2);
    *wi = SphericalDirection(sinTheta, cosTheta, phi, v1, v2, -wo);
    return PhaseHG(-cosTheta, g);
}

float HenyeyGreenstein::p(const Vector3f& wo, const Vector3f& wi) const {
    //ProfilePhase _(Prof::PhaseFuncEvaluation);
    return PhaseHG(Dot(wo, wi), g);
}

// transmittance
// sigma为常数。积分就是直接乘
Spectrum HomogeneousMedium::Tr(const Ray& ray, Sampler& sampler) const {
    //ProfilePhase _(Prof::MediumTr);
    return Exp(-sigma_t * std::min(ray.tMax * ray.d.Length(), MaxFloat));
}

Spectrum HomogeneousMedium::Sample(const Ray& ray, Sampler& sampler,  MemoryBlock& mb, MediumInteraction* mi, std::shared_ptr<Medium> this_medium) {
    // 采样一个通道
    int channel = std::min((int)(sampler.Get1D() * Spectrum::nSamples), Spectrum::nSamples - 1);

    /*
    float s = 0;
    std::cout << sigma_t[channel] << std::endl;
    for (int i = 0; i < 1000000; ++ i) {
        float dist = -std::log(1 - sampler.Get1D()) / sigma_t[channel];
        s += dist;
    }

    std::cout <<"avg "<< sigma_t[channel]<<" " << s / 1000000 << " "  << std::endl;*/

    // 对transmittance函数采样得到dist。sigma取2的话，采出来平均值为0.5。
    float dist = -std::log(1 - sampler.Get1D()) / sigma_t[channel];

    float t = std::min(dist * ray.d.Length(), ray.tMax);

    // tMax为起始点和交点的间距。如果采样在间距内，走介质的采样。ray marching走一步。
    // 否则当作直接打到了面上，直接算衰减。为什么做平均，还没仔细看。
    // pbrt 892
    bool sampledMedium = t < ray.tMax;
    if (sampledMedium)
        *mi = MediumInteraction(ray(t), -ray.d, ray.time, this_medium, MB_ALLOC(mb, HenyeyGreenstein)(g));
    
    // 算transmittance。一定是在[0, 1]。
    Spectrum Tr = Exp(-sigma_t * std::min(t, MaxFloat) * ray.d.Length());

    // 对于介质。density = sigma_t * Tr即为normalize之后的pdf。
    // 否则，pdf实际是落在tMax之后的概率和。最后算出来等于Tr。
    Spectrum density = sampledMedium ? (sigma_t * Tr) : Tr;

    // pdf取rgb平均值
    float pdf = 0;
    for (int i = 0; i < Spectrum::nSamples; ++i)
        pdf += density[i];
    pdf *= 1 / (float)Spectrum::nSamples;
    if (pdf == 0) {
        //CHECK(Tr.IsBlack());
        pdf = 1;
    }

    // 最后得到的就是Tr的蒙特卡洛积分。也就是衰减的一个代表值。对于medium，再带上in-scattering的sigma_s。

    return sampledMedium ? (Tr * sigma_s / pdf) : (Tr / pdf);
}


std::shared_ptr<Medium> MakeHomogeneousMedium(const json& config) {
    // float sig_a_rgb[3] = { .0011f, .0024f, .014f }, sig_s_rgb[3] = { 2.55f, 3.21f, 3.77f };
    //Spectrum sig_a(sig_a_rgb[0], sig_a_rgb[1], sig_a_rgb[2]);
    //Spectrum sig_s(sig_s_rgb[0], sig_s_rgb[1], sig_s_rgb[2]);

    //std::string preset = paramSet.FindOneString("preset", "");
    //bool found = GetMediumScatteringProperties(preset, &sig_a, &sig_s);

    //float g = 0.3;// paramSet.FindOneFloat("g", 0.0f);

    //sig_a = paramSet.FindOneSpectrum("sigma_a", sig_a) * scale;
    //sig_s = paramSet.FindOneSpectrum("sigma_s", sig_s) * scale;
    
    auto sig_a = config["sigma_a"];
    auto sig_s = config["sigma_s"];

    Medium* m = new HomogeneousMedium(config, Spectrum(sig_a[0], sig_a[1], sig_a[2]), Spectrum(sig_s[0], sig_s[1], sig_s[2]), config["g"]);

    return std::shared_ptr<Medium>(m);
}