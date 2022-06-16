#ifndef CORE_SPECTRUM_H
#define CORE_SPECTRUM_H

#include <vector>
//#include "interaction.h"
#include "transform.h"


class CoefficientSpectrum {
public:
    CoefficientSpectrum(int nSpectrumSamples, float v = 0.f):
        nSpectrumSamples(nSpectrumSamples),
        c(nSpectrumSamples) {

        for (int i = 0; i < nSpectrumSamples; ++i) c[i] = v;
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

    int nSpectrumSamples;
    std::vector<float> c;
};

class RGBSpectrum : public CoefficientSpectrum {
public:
    // RGBSpectrum Public Methods
    RGBSpectrum(float v = 0.f) : CoefficientSpectrum(3, v) {}
    RGBSpectrum(const CoefficientSpectrum& v) : CoefficientSpectrum(v) {}
};

typedef RGBSpectrum Spectrum;





#endif