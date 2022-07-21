#ifndef CORE_SAMPLING_H
#define CORE_SAMPLING_H

#include "geometry.h"
#include <vector>

Point2f ConcentricSampleDisk(const Point2f& u);
Vector3f CosineSampleHemisphere(const Point2f& u);

class Distribution1D {
public:

    // Distribution1D Public Methods
    // 
    // pbrt page 754
    // 输入一串值(权重)，算出每一级对应的概率[0, 1]放在cdf里。以后可输入概率值取出对应的权重。
    // 例如输入        [3,   2,  1]
    // cdf最终形态 [0, 0.5, 5/6, 1]
    // funcInt为平均值
    // func为原始值

    Distribution1D(const std::vector<float> &f, int n) :
        func(f), 
        cdf(n + 1) {
        // Compute integral of step function at $x_i$
        cdf[0] = 0;
        for (int i = 1; i < n + 1; ++i) 
            cdf[i] = cdf[i - 1] + func[i - 1] / n;

        // Transform step function integral into CDF
        funcInt = cdf[n];
        if (funcInt == 0) {
            for (int i = 1; i < n + 1; ++i) 
                cdf[i] = float(i) / float(n);
        }
        else {
            for (int i = 1; i < n + 1; ++i) 
                cdf[i] /= funcInt;
        }
    }

    int GetSampleIndex(float u) const {
        int start = 0, end = cdf.size() - 1;
        int mid;
        for (;;) {
            if (start == end)
                return start;

            mid = (start + end) / 2;

            if (u <= cdf[mid])
                end = mid;
            else {
                if (start == mid)
                    return start; // for final 2
                start = mid;
            }
                
        }

    }

    int SampleDiscrete(float u, float* pdf = nullptr, float* uRemapped = nullptr) const {
        // Find surrounding CDF segments and _offset_
        // int offset = FindInterval((int)cdf.size(), [&](int index) { return cdf[index] <= u; });
        int offset = GetSampleIndex(u);

        if (pdf)
            *pdf = (funcInt > 0) ? func[offset] / (funcInt * func.size()) : 0;

        /*if (uRemapped)
            *uRemapped = (u - cdf[offset]) / (cdf[offset + 1] - cdf[offset]);
        if (uRemapped)
            CHECK(*uRemapped >= 0.f && *uRemapped <= 1.f);*/

        return offset;
    }



    std::vector<float> func, cdf;
    float funcInt;
};

#endif