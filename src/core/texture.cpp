#include "texture.h"

float Lanczos(float x, float tau) {
    x = std::abs(x);
    if (x < 1e-5f) return 1;
    if (x > 1.f) return 0;
    x *= Pi;
    float s = std::sin(x * tau) / (x * tau);
    float lanczos = std::sin(x) / x;
    return s * lanczos;
}