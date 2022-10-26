#include "sampler.h"
#include "camera.h"
#include <random>

Sampler::~Sampler() {}

Sampler::Sampler(int samplesPerPixel) : samplesPerPixel(samplesPerPixel) {}

CameraSample Sampler::GetCameraSample(const Point2i& pRaster) {
    CameraSample cs;
    cs.pFilm = Point2f(pRaster.x, pRaster.y) + Get2D(); // 0-1·¶Î§Ëæ»úÈ¡Öµ
    cs.time = Get1D();
    cs.pLens = Get2D();
    return cs;
}

void Sampler::StartPixel(const Point2i& p) {
    currentPixel = p;
    currentPixelSampleIndex = 0;
    // Reset array offsets for next pixel sample
    array1DOffset = array2DOffset = 0;
}

bool Sampler::StartNextSample() {
    // Reset array offsets for next pixel sample
    array1DOffset = array2DOffset = 0;
    return ++currentPixelSampleIndex < samplesPerPixel;
}

bool Sampler::SetSampleNumber(int sampleNum) {
    // Reset array offsets for next pixel sample
    array1DOffset = array2DOffset = 0;
    currentPixelSampleIndex = sampleNum;
    return currentPixelSampleIndex < samplesPerPixel;
}


RandomSampler::RandomSampler(int ns, int seed) : Sampler(ns), rng() {
}

float RandomSampler::Get1D() {
    //ProfilePhase _(Prof::GetSample);
    //CHECK_LT(currentPixelSampleIndex, samplesPerPixel);
    return rng.UniformFloat();
}

Point2f RandomSampler::Get2D() {
    //ProfilePhase _(Prof::GetSample);
    //CHECK_LT(currentPixelSampleIndex, samplesPerPixel);

    // return { rng.UniformFloat() * 2 - 1, rng.UniformFloat() * 2 -1 };
    return { rng.UniformFloat(), rng.UniformFloat() };
}

std::unique_ptr<Sampler> RandomSampler::Clone() {
    RandomSampler* rs = new RandomSampler(*this);
    //rs->rng.SetSequence(seed);
    return std::unique_ptr<Sampler>(rs);
}

void RandomSampler::StartPixel(const Point2i& p) {
    //ProfilePhase _(Prof::StartPixel);
    for (size_t i = 0; i < sampleArray1D.size(); ++i)
        for (size_t j = 0; j < sampleArray1D[i].size(); ++j)
            sampleArray1D[i][j] = rng.UniformFloat();

    for (size_t i = 0; i < sampleArray2D.size(); ++i)
        for (size_t j = 0; j < sampleArray2D[i].size(); ++j)
            sampleArray2D[i][j] = { rng.UniformFloat(), rng.UniformFloat() };
    Sampler::StartPixel(p);
}
