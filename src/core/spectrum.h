#ifndef CORE_SPECTRUM_H
#define CORE_SPECTRUM_H

#include <vector>
#include <algorithm>
//#include "interaction.h"
#include "transform.h"

enum class SpectrumType { 
    Reflectance, 
    Illuminant 
};

class CoefficientSpectrum {
public:
    CoefficientSpectrum(int nSpectrumSamples, float v = 0.f):
        nSpectrumSamples(nSpectrumSamples),
        c(nSpectrumSamples) {

        for (int i = 0; i < nSpectrumSamples; ++i) c[i] = v;
    }

    CoefficientSpectrum(int nSpectrumSamples, float r, float g, float b) :
        nSpectrumSamples(nSpectrumSamples),
        c(nSpectrumSamples) {

        c[0] = r;
        c[1] = g;
        c[2] = b;
    }

    CoefficientSpectrum& operator=(const CoefficientSpectrum& s) {
        //DCHECK(!s.HasNaNs());
        for (int i = 0; i < nSpectrumSamples; ++i) c[i] = s.c[i];
        return *this;
    }

    CoefficientSpectrum Clamp(float low = 0, float high = Infinity) const {
        CoefficientSpectrum ret(3);
        for (int i = 0; i < nSpectrumSamples; ++i)
            ret.c[i] = std::clamp(c[i], low, high);
        //DCHECK(!ret.HasNaNs());
        return ret;
    }

    bool IsBlack() const {
        for (int i = 0; i < nSpectrumSamples; ++i)
            if (c[i] != 0.) return false;
        return true;
    }

    CoefficientSpectrum operator+(const CoefficientSpectrum& s2) const {
        CoefficientSpectrum ret = *this;
        for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] += s2.c[i];
        return ret;
    }

    CoefficientSpectrum& operator+=(const CoefficientSpectrum& s2) {
        for (int i = 0; i < nSpectrumSamples; ++i) c[i] += s2.c[i];
        return *this;
    }

    CoefficientSpectrum operator*(float a) const {
        CoefficientSpectrum ret = *this;
        for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] *= a;
        return ret;
    }

    CoefficientSpectrum& operator*=(float a) {
        for (int i = 0; i < nSpectrumSamples; ++i) c[i] *= a;
        //DCHECK(!HasNaNs());
        return *this;
    }

    CoefficientSpectrum operator/(float a) const {
        CoefficientSpectrum ret = *this;
        for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] /= a;
        return ret;
    }

    CoefficientSpectrum operator*(const CoefficientSpectrum& sp) const {
        //DCHECK(!sp.HasNaNs());
        CoefficientSpectrum ret = *this;
        for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] *= sp.c[i];
        return ret;
    }

    CoefficientSpectrum& operator*=(const CoefficientSpectrum& sp) {
        //DCHECK(!sp.HasNaNs());
        for (int i = 0; i < nSpectrumSamples; ++i) c[i] *= sp.c[i];
        return *this;
    }

    CoefficientSpectrum operator-(const CoefficientSpectrum& s2) const {
        //DCHECK(!s2.HasNaNs());
        CoefficientSpectrum ret = *this;
        for (int i = 0; i < nSpectrumSamples; ++i) ret.c[i] -= s2.c[i];
        return ret;
    }

    friend inline CoefficientSpectrum operator*(float a,
        const CoefficientSpectrum& s) {
        //DCHECK(!std::isnan(a) && !s.HasNaNs());
        return s * a;
    }

    int nSpectrumSamples;
    std::vector<float> c;
};

class RGBSpectrum : public CoefficientSpectrum {
public:

    ~RGBSpectrum() {}
    // RGBSpectrum Public Methods
    RGBSpectrum(float v = 0.f) : CoefficientSpectrum(3, v) {}
    RGBSpectrum(float r, float g, float b) : CoefficientSpectrum(3, r, g, b) {}
    RGBSpectrum(const CoefficientSpectrum& v) : CoefficientSpectrum(v) {}
    RGBSpectrum(const RGBSpectrum& s,
        SpectrumType type = SpectrumType::Reflectance):CoefficientSpectrum(3) {
        *this = s;
    }

    // ?
    float y() const {
        const float YWeight[3] = { 0.212671f, 0.715160f, 0.072169f };
        return YWeight[0] * c[0] + YWeight[1] * c[1] + YWeight[2] * c[2];
    }

};

inline RGBSpectrum Lerp(float t, const RGBSpectrum& s1, const RGBSpectrum& s2) {
    return (1 - t) * s1 + t * s2;
}

typedef RGBSpectrum Spectrum;





#endif