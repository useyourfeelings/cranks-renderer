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
    // ����һ��ֵ(Ȩ��)�����ÿһ����Ӧ�ĸ���[0, 1]����cdf��Ժ���������ֵȡ����Ӧ��Ȩ�ء�
    // ��������        [3,   2,  1]
    // cdf������̬ [0, 0.5, 5/6, 1]
    // funcIntΪƽ��ֵ2
    // funcΪԭʼֵ

    Distribution1D(const float *f, int n) :
        func(f, f + n), 
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

    // uΪ[0, 1)
    // �ҳ�������һ���䣬����ǰһ��index��
    // �����ȣ�����0.5������0��index��
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

    // uΪ[0, 1)
    // ����������һ��
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

    // uΪ[0, 1)
    // ��������һ����ֵ��
    // ��������cdf֮�䣬��һ�����ԵĲ�ֵ����ν������
    float SampleContinuous(float u, float* pdf, int* off = nullptr) const {
        // Find surrounding CDF segments and _offset_
        //int offset = FindInterval((int)cdf.size(), [&](int index) { return cdf[index] <= u; });
        int offset = GetSampleIndex(u);

        if (off) *off = offset;
        // Compute offset along CDF segment
        // uΪ����ֵ������ֵ��cdf[offset]Ϊ������ĳһ���������в�ࡣ
        // ��������0.4��du = 0.4 - 0 = 0.4
        float du = u - cdf[offset];

        if ((cdf[offset + 1] - cdf[offset]) > 0) {
            //CHECK_GT(cdf[offset + 1], cdf[offset]);

            // ʵ��ֵ�Ĳ�/����֮��Ĳ�=��������֮��İٷֱ�
            du /= (cdf[offset + 1] - cdf[offset]);
        }
        //DCHECK(!std::isnan(du));

        // Compute PDF for sampled offset
        // pdf����λ�㡣��λ/ƽ������
        if (pdf)
            *pdf = (funcInt > 0) ? func[offset] / funcInt : 0;

        // Return $x\in{}[0,1)$ corresponding to sample
        // offset+du�õ���offsetǰ������һ���İٷֱȡ�����indexΪ2ǰ����30%������2.3��
        // �ٳ�size���õ������İٷֱȡ�
        return (offset + du) / Count();
    }

    int Count() const {
        return (int)func.size();
    }

    std::vector<float> func, cdf;
    float funcInt;
};



class Distribution2D {
public:
    // Distribution2D Public Methods
    Distribution2D(const float* data, int nu, int nv);

    // ����2d���ֵ����y����ȡֵ����x����ȡֵ��
    Point2f SampleContinuous(const Point2f& u, float* pdf) const {
        float pdfs[2];
        int v;
        float d1 = pMarginal->SampleContinuous(u.y, &pdfs[1], &v);
        float d0 = pConditionalV[v]->SampleContinuous(u.x, &pdfs[0]);
        *pdf = pdfs[0] * pdfs[1];
        return Point2f(d0, d1);
    }

    float Pdf(const Point2f& p) const {
        int iu = Clamp(int(p.x * pConditionalV[0]->Count()), 0,
            pConditionalV[0]->Count() - 1);
        int iv =
            Clamp(int(p.y * pMarginal->Count()), 0, pMarginal->Count() - 1);
        return pConditionalV[iv]->func[iu] / pMarginal->funcInt;
    }

private:
    // Distribution2D Private Data
    std::vector<std::unique_ptr<Distribution1D>> pConditionalV;
    std::unique_ptr<Distribution1D> pMarginal;
};

inline float PowerHeuristic(int nf, float fPdf, int ng, float gPdf) {
    float f = nf * fPdf, g = ng * gPdf;
    return (f * f) / (f * f + g * g);
}

#endif