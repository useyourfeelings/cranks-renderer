#ifndef BASE_EFLOAT_H
#define BASE_EFLOAT_H

#include<iostream>
#include<limits>
#include"../core/pbr.h"

inline uint32_t FloatToBits(float f) {
    uint32_t ui;
    memcpy(&ui, &f, sizeof(float));
    return ui;
}

inline float BitsToFloat(uint32_t ui) {
    float f;
    memcpy(&f, &ui, sizeof(uint32_t));
    return f;
}

// 如果是正数，加一个ulp。否则减一个ulp。
inline float NextFloatUp(float v) {
    // Handle infinity and negative zero for _NextFloatUp()_
    if (std::isinf(v) && v > 0.) return v;
    if (v == -0.f) v = 0.f;

    // Advance _v_ to next higher float
    uint32_t ui = FloatToBits(v);
    if (v >= 0)
        ++ui;
    else
        --ui;
    return BitsToFloat(ui);
}

// 如果是正数，减一个ulp。否则加一个ulp。
inline float NextFloatDown(float v) {
    // Handle infinity and positive zero for _NextFloatDown()_
    if (std::isinf(v) && v < 0.) return v;
    if (v == 0.f) v = -0.f;
    uint32_t ui = FloatToBits(v);
    if (v > 0)
        --ui;
    else
        ++ui;
    return BitsToFloat(ui);
}

inline float gamma(int n) {
    return (n * MachineEpsilon) / (1 - n * MachineEpsilon);
}

class EFloat {
public:
	EFloat() {}

    EFloat(float v, float err = 0.f) : v(v) {
        if (err == 0.)
            low = high = v;
        else {
            // Compute conservative bounds by rounding the endpoints away
            // from the middle. Note that this will be over-conservative in
            // cases where v-err or v+err are exactly representable in
            // floating-point, but it's probably not worth the trouble of
            // checking this case.
            low = NextFloatDown(v - err);
            high = NextFloatUp(v + err);
        }
    }

    operator float() const { 
        return v; 
    }

    EFloat operator+(EFloat ef) const {
        EFloat r;
        r.v = v + ef.v;

        // Interval arithemetic addition, with the result rounded away from
        // the value r.v in order to be conservative.
        r.low = NextFloatDown(LowerBound() + ef.LowerBound()); // 算可能的最小值
        r.high = NextFloatUp(UpperBound() + ef.UpperBound());  // 算可能的最大值
        //r.Check();
        return r;
    }

    EFloat operator-(EFloat ef) const {
        EFloat r;
        r.v = v - ef.v;

        r.low = NextFloatDown(LowerBound() - ef.UpperBound()); // 算可能的最小值  low-up
        r.high = NextFloatUp(UpperBound() - ef.LowerBound());  // 算可能的最大值  up-low
        //r.Check();
        return r;
    }

    EFloat operator*(EFloat ef) const {
        EFloat r;
        r.v = v * ef.v;

        // 上限下限一定落在四种组合里
        float prod[4] = {
            LowerBound() * ef.LowerBound(), UpperBound() * ef.LowerBound(),
            LowerBound() * ef.UpperBound(), UpperBound() * ef.UpperBound() };
        r.low = NextFloatDown(
            std::min(std::min(prod[0], prod[1]), std::min(prod[2], prod[3])));
        r.high = NextFloatUp(
            std::max(std::max(prod[0], prod[1]), std::max(prod[2], prod[3])));
        //r.Check();
        return r;
    }
    EFloat operator/(EFloat ef) const {
        EFloat r;
        r.v = v / ef.v;

        // 跨过0的话上下限就是无穷
        if (ef.low < 0 && ef.high > 0) {
            // Bah. The interval we're dividing by straddles zero, so just
            // return an interval of everything.
            r.low = -Infinity;
            r.high = Infinity;
        }
        else {
            // 上限下限一定落在四种组合里

            float div[4] = {
                LowerBound() / ef.LowerBound(), UpperBound() / ef.LowerBound(),
                LowerBound() / ef.UpperBound(), UpperBound() / ef.UpperBound() };
            r.low = NextFloatDown(
                std::min(std::min(div[0], div[1]), std::min(div[2], div[3])));
            r.high = NextFloatUp(
                std::max(std::max(div[0], div[1]), std::max(div[2], div[3])));
        }
        //r.Check();
        return r;
    }

    EFloat operator-() const {
        EFloat r;
        r.v = -v;

        r.low = -high;
        r.high = -low;
        //r.Check();
        return r;
    }

    inline bool operator==(EFloat fe) const { 
        return v == fe.v; 
    }

    EFloat(const EFloat& ef) {
        //ef.Check();
        v = ef.v;
        low = ef.low;
        high = ef.high;
    }
    EFloat& operator=(const EFloat& ef) {
        //ef.Check();
        if (&ef != this) {
            v = ef.v;
            low = ef.low;
            high = ef.high;
        }
        return *this;
    }


    float UpperBound() const { return high; }
    float LowerBound() const { return low; }

    friend inline EFloat sqrt(EFloat fe);
    friend inline EFloat abs(EFloat fe);
    friend inline int Quadratic(EFloat A, EFloat B, EFloat C, EFloat* t0,
        EFloat* t1);

private:
    float v, low, high;

};

inline EFloat operator*(float f, EFloat fe) { return EFloat(f) * fe; }

inline EFloat operator/(float f, EFloat fe) { return EFloat(f) / fe; }

inline EFloat operator+(float f, EFloat fe) { return EFloat(f) + fe; }

inline EFloat operator-(float f, EFloat fe) { return EFloat(f) - fe; }

inline EFloat sqrt(EFloat fe) {
    EFloat r;
    r.v = std::sqrt(fe.v);

    r.low = NextFloatDown(std::sqrt(fe.low));
    r.high = NextFloatUp(std::sqrt(fe.high));
    //r.Check();
    return r;
}

inline EFloat abs(EFloat fe) {
    if (fe.low >= 0)
        // The entire interval is greater than zero, so we're all set.
        return fe;
    else if (fe.high <= 0) {
        // The entire interval is less than zero.
        EFloat r;
        r.v = -fe.v;

        r.low = -fe.high;
        r.high = -fe.low;
        //r.Check();
        return r;
    }
    else {
        // The interval straddles zero.
        EFloat r;
        r.v = std::abs(fe.v);

        r.low = 0;
        r.high = std::max(-fe.low, fe.high);
        //r.Check();
        return r;
    }
}

// efloat版本
inline int Quadratic(EFloat a, EFloat b, EFloat c, EFloat* r1, EFloat* r2) {
    float discriminant = b.v * b.v - a.v * c.v * 4;

    if (discriminant < 0)
        return 0;

    if (discriminant == 0) {
        *r1 = *r2 = (-b) / (2.f * a);
        return 1;
    }

    float sqrtd = std::sqrt(discriminant);

    EFloat efloat_sqrtd(sqrtd, MachineEpsilon * sqrtd);

    EFloat q;

    //*r1 = (-b + sqrtd) / (2 * a);
    //*r2 = (-b - sqrtd) / (2 * a);
    // 避免趋近于0

    // <<Accuracy and Stability of Numerical Algorithms>> 1.7 1.8
    // 避免可能的相减为0

    //*r1 = (b - sqrtd) / (-2 * a);
    //*r2 = (b + sqrtd) / (-2 * a);

    // 如果b<0就挑减法，如果大于0就挑加法。算出一个误差小的解，再算另一个。
    if (b.v < 0)
        q = -0.5f * (b - efloat_sqrtd);
    else
        q = -0.5f * (b + efloat_sqrtd);

    // r1 * r2 = c/a
    *r1 = q / a;
    *r2 = c / q;

    if (r1->v > r2->v)
        std::swap(*r1, *r2);

    return 2;
}

#endif