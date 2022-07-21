#ifndef CORE_SPECTRUM_H
#define CORE_SPECTRUM_H

#include <vector>
#include <algorithm>
//#include "interaction.h"
#include "transform.h"


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

    int nSpectrumSamples;
    std::vector<float> c;
};

class RGBSpectrum : public CoefficientSpectrum {
public:
    // RGBSpectrum Public Methods
    RGBSpectrum(float v = 0.f) : CoefficientSpectrum(3, v) {}
    RGBSpectrum(float r, float g, float b) : CoefficientSpectrum(3, r, g, b) {}
    RGBSpectrum(const CoefficientSpectrum& v) : CoefficientSpectrum(v) {}
};

typedef RGBSpectrum Spectrum;





#endif