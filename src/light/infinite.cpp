#include "infinite.h"
#include "../tool/image.h"

InfiniteAreaLight::InfiniteAreaLight(const Transform& LightToWorld,
    const Spectrum& L, float strength, int nSamples,
    const std::string& texmap)
    : Light((int)LightFlags::Infinite, LightToWorld, nSamples) {

    // todo
    // ���������
    // 

    pos = LightToWorld(Point3f(0, 0, 0));

    // create mipmap
    int res_x, res_y;
    auto image_data = ReadHDRRaw(texmap, &res_x, &res_y);

    auto spectrum_data = new RGBSpectrum[res_x * res_y];
    for (int i = 0; i < res_x * res_y; ++i) {
        // ������strength
        spectrum_data[i] = L * strength * RGBSpectrum(image_data[i * 4] / 255.f, image_data[i * 4 + 1] / 255.f, image_data[i * 4 + 2] / 255.f);
    }

    this->Lmap = std::make_unique<MIPMap<RGBSpectrum>>(Point2i(res_x, res_y), spectrum_data);

    delete[]spectrum_data;

    // Initialize sampling PDFs for infinite area light

    // pbrt page 848
    // Compute scalar-valued image _img_ from environment map
    int width = 2 * Lmap->resolution.x;
    int height = 2 * Lmap->resolution.y;

    std::unique_ptr<float[]> img(new float[width * height]);
    float fwidth = 0.5f / std::min(width, height); // �õ�-nLevels?

    // �ѳ�����ͼƬӳ�䵽����

    for (int h = 0; h < height; ++ h) { // �ӽŵ׵�ͷ��
        float vp = (h + .5f) / (float)height; // �߶Ȱٷֱȡ���0��1��

        // 0->Pi
        float sinTheta = std::sin(Pi * (h + .5f) / height); // sin(0)->sin(180)��sin(180)->sin(0)һ��

        for (int u = 0; u < width; ++u) { // 360��
            float up = (u + .5f) / (float)width; // u�ٷֱȡ���0��1��

            img[u + h * width] = Lmap->Lookup(Point2f(up, vp), fwidth).y();
            // https://www.tandfonline.com/doi/full/10.1080/15502724.2019.1684319
            // y������ɫ�ʿռ䡣hdrԭ����ء�

            img[u + h * width] *= sinTheta; // todo
        }
    }

    // Compute sampling distributions for rows and columns of image
    // ������ͼƬ��ֵ����2d�ֲ�

    distribution.reset(new Distribution2D(img.get(), width, height));

}

Spectrum InfiniteAreaLight::Le(const RayDifferential& ray) const {
    Vector3f w = Normalize(WorldToLight(ray.d));
    Point2f st(SphericalPhi(w) * Inv2Pi, SphericalTheta(w) * InvPi);

    //std::cout << "Le st " << st.x << " " << st.y <<" z = "<< w.z << " percent "<< std::acos(w.z)* InvPi << std::endl;

    //Log("Le st %f %f", st.x, st.y);

    return Spectrum(Lmap->Lookup(st), SpectrumType::Illuminant);
}

// 
Spectrum InfiniteAreaLight::Sample_Li(const Interaction& ref, const Point2f& u, Vector3f* wi, float* pdf) const {

    //ProfilePhase _(Prof::LightSample);
    // Find $(u,v)$ sample coordinates in infinite light texture
    float mapPdf;
    Point2f uv = distribution->SampleContinuous(u, &mapPdf); // �����Ǹ��ٷֱ�
    if (mapPdf == 0)
        return Spectrum(0.f);

    // Convert infinite light sample point to direction
    // �ٷֱȵ����Ƕ� / ����
    float theta = uv.y * Pi; // ͷ���ŵ�
    float phi = uv.x * 2 * Pi; // 360ת��

    float cosTheta = std::cos(theta), sinTheta = std::sin(theta);
    float sinPhi = std::sin(phi), cosPhi = std::cos(phi);

    // ������
    *wi = LightToWorld(Vector3f(sinTheta * cosPhi, sinTheta * sinPhi, cosTheta));

    // Compute PDF for sampled infinite light direction
    *pdf = mapPdf / (2 * Pi * Pi * sinTheta);
    if (sinTheta == 0) *pdf = 0;

    // Return radiance value for infinite light direction
    //*vis = VisibilityTester(ref, Interaction(ref.p + *wi * (2 * worldRadius), ref.time, mediumInterface));
    return Spectrum(Lmap->Lookup(uv), SpectrumType::Illuminant);

}