#ifndef CORE_SAMPLER_H
#define CORE_SAMPLER_H

#include <memory>
#include <vector>
#include "geometry.h"
#include "camera.h"
#include "rng.h"

class Sampler {
public:
    // Sampler Interface
    virtual ~Sampler();
    Sampler(int samplesPerPixel);
    virtual void StartPixel(const Point2i& p);
    virtual float Get1D() = 0;
    virtual Point2f Get2D() = 0;
    CameraSample GetCameraSample(const Point2i& pRaster);
    void Request1DArray(int n);
    void Request2DArray(int n);
    virtual int RoundCount(int n) const { return n; }
    const float* Get1DArray(int n);
    const Point2f* Get2DArray(int n);
    virtual bool StartNextSample();
    //virtual std::unique_ptr<Sampler> Clone(int seed) = 0;
    virtual bool SetSampleNumber(int sampleNum);
    /*std::string StateString() const {
        return StringPrintf("(%d,%d), sample %" PRId64, currentPixel.x,
            currentPixel.y, currentPixelSampleIndex);
    }*/
    int CurrentSampleNumber() const { return currentPixelSampleIndex; }

    // Sampler Public Data
    const int samplesPerPixel;

protected:
    // Sampler Protected Data
    Point2i currentPixel;
    int64_t currentPixelSampleIndex;
    std::vector<int> samples1DArraySizes, samples2DArraySizes;
    std::vector<std::vector<float>> sampleArray1D;
    std::vector<std::vector<Point2f>> sampleArray2D;

private:
    // Sampler Private Data
    size_t array1DOffset, array2DOffset;
};


class RandomSampler : public Sampler {
public:
    RandomSampler(int ns, int seed = 0);
    void StartPixel(const Point2i&);
    float Get1D();
    Point2f Get2D();
    //std::unique_ptr<Sampler> Clone(int seed);

private:
    RNG rng;
};


#endif