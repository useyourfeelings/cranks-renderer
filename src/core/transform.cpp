#include <string.h>
#include <utility>
#include "pbr.h"
#include "transform.h"
#include "interaction.h"
#include "../base/efloat.h"

Vector3f Normalize(const Vector3f& v) {
    return v / v.Length();
}

bool SolveLinearSystem2x2(const float A[2][2], const float B[2], float* x0, float* x1) {
    float det = A[0][0] * A[1][1] - A[0][1] * A[1][0];
    if (std::abs(det) < 1e-10f)
        return false;

    *x0 = (A[1][1] * B[0] - A[0][1] * B[1]) / det;
    *x1 = (A[0][0] * B[1] - A[1][0] * B[0]) / det;
    if (std::isnan(*x0) || std::isnan(*x1))
        return false;

    return true;
}

Matrix4x4::Matrix4x4(float t00, float t01, float t02, float t03, float t10, float t11,
	float t12, float t13, float t20, float t21, float t22, float t23,
	float t30, float t31, float t32, float t33) {
    m[0][0] = t00;
    m[0][1] = t01;
    m[0][2] = t02;
    m[0][3] = t03;
    m[1][0] = t10;
    m[1][1] = t11;
    m[1][2] = t12;
    m[1][3] = t13;
    m[2][0] = t20;
    m[2][1] = t21;
    m[2][2] = t22;
    m[2][3] = t23;
    m[3][0] = t30;
    m[3][1] = t31;
    m[3][2] = t32;
    m[3][3] = t33;
}

Matrix4x4::Matrix4x4(float mat[4][4]) { memcpy(m, mat, 16 * sizeof(float)); }


Matrix4x4 Transpose(const Matrix4x4& m) {
    return Matrix4x4(m.m[0][0], m.m[1][0], m.m[2][0], m.m[3][0],
                     m.m[0][1], m.m[1][1], m.m[2][1], m.m[3][1],
                     m.m[0][2], m.m[1][2], m.m[2][2], m.m[3][2],
                     m.m[0][3], m.m[1][3], m.m[2][3], m.m[3][3]);
}

Matrix4x4 Inverse(const Matrix4x4& m) {
    int indxc[4], indxr[4];
    int ipiv[4] = { 0, 0, 0, 0 };
    float minv[4][4];
    memcpy(minv, m.m, 4 * 4 * sizeof(float));
    for (int i = 0; i < 4; i++) {
        int irow = 0, icol = 0;
        float big = 0.f;
        // Choose pivot
        for (int j = 0; j < 4; j++) {
            if (ipiv[j] != 1) {
                for (int k = 0; k < 4; k++) {
                    if (ipiv[k] == 0) {
                        if (std::abs(minv[j][k]) >= big) {
                            big = float(std::abs(minv[j][k]));
                            irow = j;
                            icol = k;
                        }
                    }
                    else if (ipiv[k] > 1)
                        Error("Singular matrix in MatrixInvert");
                }
            }
        }
        ++ipiv[icol];
        // Swap rows _irow_ and _icol_ for pivot
        if (irow != icol) {
            for (int k = 0; k < 4; ++k) std::swap(minv[irow][k], minv[icol][k]);
        }
        indxr[i] = irow;
        indxc[i] = icol;
        if (minv[icol][icol] == 0.f) Error("Singular matrix in MatrixInvert");

        // Set $m[icol][icol]$ to one by scaling row _icol_ appropriately
        float pivinv = 1. / minv[icol][icol];
        minv[icol][icol] = 1.;
        for (int j = 0; j < 4; j++) minv[icol][j] *= pivinv;

        // Subtract this row from others to zero out their columns
        for (int j = 0; j < 4; j++) {
            if (j != icol) {
                float save = minv[j][icol];
                minv[j][icol] = 0;
                for (int k = 0; k < 4; k++) minv[j][k] -= minv[icol][k] * save;
            }
        }
    }
    // Swap columns to reflect permutation
    for (int j = 3; j >= 0; j--) {
        if (indxr[j] != indxc[j]) {
            for (int k = 0; k < 4; k++)
                std::swap(minv[k][indxr[j]], minv[k][indxc[j]]);
        }
    }
    return Matrix4x4(minv);
}


// 把8个顶点都转一下再合并
BBox3f Transform::operator()(const BBox3f& b) const {
    const Transform& M = *this;
    BBox3f ret(M(Point3f(b.pMin.x, b.pMin.y, b.pMin.z)));
    ret = Union(ret, M(Point3f(b.pMax.x, b.pMin.y, b.pMin.z)));
    ret = Union(ret, M(Point3f(b.pMin.x, b.pMax.y, b.pMin.z)));
    ret = Union(ret, M(Point3f(b.pMin.x, b.pMin.y, b.pMax.z)));
    ret = Union(ret, M(Point3f(b.pMin.x, b.pMax.y, b.pMax.z)));
    ret = Union(ret, M(Point3f(b.pMax.x, b.pMax.y, b.pMin.z)));
    ret = Union(ret, M(Point3f(b.pMax.x, b.pMin.y, b.pMax.z)));
    ret = Union(ret, M(Point3f(b.pMax.x, b.pMax.y, b.pMax.z)));
    return ret;
}


// ignore error
template <typename T>
inline Point3<T> Transform::operator()(const Point3<T>& p) const {
    float x = p.x, y = p.y, z = p.z;
    // Compute transformed coordinates from point _pt_
    float xp = (m.m[0][0] * x + m.m[0][1] * y) + (m.m[0][2] * z + m.m[0][3]);
    float yp = (m.m[1][0] * x + m.m[1][1] * y) + (m.m[1][2] * z + m.m[1][3]);
    float zp = (m.m[2][0] * x + m.m[2][1] * y) + (m.m[2][2] * z + m.m[2][3]);
    float wp = (m.m[3][0] * x + m.m[3][1] * y) + (m.m[3][2] * z + m.m[3][3]);

    //Log("trans point %f %f %f %f", xp, yp, zp, wp);

    if (wp == 1)
        return Point3<T>(xp, yp, zp);
    else
        return Point3<T>(xp, yp, zp) / wp;
}

template <typename T>
inline Point3<T> Transform::operator()(const Point3<T>& p, Vector3<T>* pError) const {
    T x = p.x, y = p.y, z = p.z;
    // Compute transformed coordinates from point _pt_
    T xp = (m.m[0][0] * x + m.m[0][1] * y) + (m.m[0][2] * z + m.m[0][3]);
    T yp = (m.m[1][0] * x + m.m[1][1] * y) + (m.m[1][2] * z + m.m[1][3]);
    T zp = (m.m[2][0] * x + m.m[2][1] * y) + (m.m[2][2] * z + m.m[2][3]);
    T wp = (m.m[3][0] * x + m.m[3][1] * y) + (m.m[3][2] * z + m.m[3][3]);

    // pbrt page 228
    // Compute absolute error for transformed point
    T xAbsSum = (std::abs(m.m[0][0] * x) + std::abs(m.m[0][1] * y) + std::abs(m.m[0][2] * z) + std::abs(m.m[0][3]));
    T yAbsSum = (std::abs(m.m[1][0] * x) + std::abs(m.m[1][1] * y) + std::abs(m.m[1][2] * z) + std::abs(m.m[1][3]));
    T zAbsSum = (std::abs(m.m[2][0] * x) + std::abs(m.m[2][1] * y) + std::abs(m.m[2][2] * z) + std::abs(m.m[2][3]));
    *pError = gamma(3) * Vector3<T>(xAbsSum, yAbsSum, zAbsSum);
    //CHECK_NE(wp, 0);
    if (wp == 1)
        return Point3<T>(xp, yp, zp);
    else
        return Point3<T>(xp, yp, zp) / wp;
}

template <typename T>
inline Vector3<T> Transform::operator()(const Vector3<T>& v) const {
    float x = v.x, y = v.y, z = v.z;

    return Vector3<T>(m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z,
                      m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z,
                      m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z);
}

template <typename T>
inline Vector3<T> Transform::operator()(const Vector3<T>& v, Vector3<T>* absError) const {
    float x = v.x, y = v.y, z = v.z;

    absError->x = gamma(3) * (std::abs(m.m[0][0] * x) + std::abs(m.m[0][1] * y) + std::abs(m.m[0][2] * z));
    absError->y = gamma(3) * (std::abs(m.m[1][0] * x) + std::abs(m.m[1][1] * y) + std::abs(m.m[1][2] * z));
    absError->z = gamma(3) * (std::abs(m.m[2][0] * x) + std::abs(m.m[2][1] * y) + std::abs(m.m[2][2] * z));

    return Vector3<T>(m.m[0][0] * x + m.m[0][1] * y + m.m[0][2] * z,
                      m.m[1][0] * x + m.m[1][1] * y + m.m[1][2] * z,
                      m.m[2][0] * x + m.m[2][1] * y + m.m[2][2] * z);
}


inline Ray Transform::operator()(const Ray& r) const {
    Point3f o = (*this)(r.o);
    Vector3f d = (*this)(r.d);

    //o.LogSelf();

    return Ray(o, d, r.tMax, r.time);
}

 Ray Transform::operator()(const Ray& r, Vector3f* oError, Vector3f* dError) const {
    Point3f o = (*this)(r.o, oError);
    Vector3f d = (*this)(r.d, dError);

    // pbrt 232
    float tMax = r.tMax;
    float lengthSquared = d.LengthSquared();
    if (lengthSquared > 0) {
        float dt = Dot(Abs(d), *oError) / lengthSquared; // Dot(Abs(d), *oError) 计算oError在d方向的长度

        // 算dt不又产生误差了吗？
        // 为什么是/lengthSquared？而不是/length
        // /lengthSquared让百分比更小。。

        o += d * dt; // 延伸出去
        //        tMax -= dt;
    }
    return Ray(o, d, tMax, r.time);
}

SurfaceInteraction Transform::operator()(const SurfaceInteraction& si) const {
    SurfaceInteraction ret;
    // Transform _p_ and _pError_ in _SurfaceInteraction_
    ret.p = (*this)(si.p);

    // Transform remaining members of _SurfaceInteraction_
    const Transform& t = *this;
    ret.n = Normalize(t(si.n));
    ret.wo = Normalize(t(si.wo));
    ret.time = si.time;
    //ret.mediumInterface = si.mediumInterface;
    ret.primitive = si.primitive;
    ret.uv = si.uv;
    ret.shape = si.shape;
    ret.dpdu = t(si.dpdu);
    ret.dpdv = t(si.dpdv);
    //ret.dndu = t(si.dndu);
    //ret.dndv = t(si.dndv);
    ret.shading.n = Normalize(t(si.shading.n));
    ret.shading.dpdu = t(si.shading.dpdu);
    ret.shading.dpdv = t(si.shading.dpdv);
    /*ret.shading.dndu = t(si.shading.dndu);
    ret.shading.dndv = t(si.shading.dndv);
    ret.dudx = si.dudx;
    ret.dvdx = si.dvdx;
    ret.dudy = si.dudy;
    ret.dvdy = si.dvdy;
    ret.dpdx = t(si.dpdx);
    ret.dpdy = t(si.dpdy);
    ret.bsdf = si.bsdf;
    ret.bssrdf = si.bssrdf;*/
    
    //ret.shading.n = Faceforward(ret.shading.n, ret.n);
    //ret.faceIndex = si.faceIndex;
    
    return ret;
}

Transform Transform::operator*(const Transform& t2) const {
    return Transform(Matrix4x4::Mul(m, t2.m), Matrix4x4::Mul(t2.mInv, mInv));
}

RayDifferential Transform::operator()(const RayDifferential& r) const {
    Ray tr = (*this)(Ray(r));
    RayDifferential ret(tr.o, tr.d, tr.tMax, tr.time);
    ret.hasDifferentials = r.hasDifferentials;
    ret.rxOrigin = (*this)(r.rxOrigin);
    ret.ryOrigin = (*this)(r.ryOrigin);
    ret.rxDirection = (*this)(r.rxDirection);
    ret.ryDirection = (*this)(r.ryDirection);
    return ret;
}

void Transform::LogSelf() const {
    Log("%f %f %f %f\n%f %f %f %f\n%f %f %f %f\n%f %f %f %f",
        m.m[0][0], m.m[0][1], m.m[0][2], m.m[0][3],
        m.m[1][0], m.m[1][1], m.m[1][2], m.m[1][3],
        m.m[2][0], m.m[2][1], m.m[2][2], m.m[2][3],
        m.m[3][0], m.m[3][1], m.m[3][2], m.m[3][3]);
}

Transform Translate(const Vector3f& delta) {
    auto m = Matrix4x4(1, 0, 0, delta.x,
                       0, 1, 0, delta.y,
                       0, 0, 1, delta.z, 
                       0, 0, 0, 1);

    auto mInv = Matrix4x4(1, 0, 0, -delta.x,
                          0, 1, 0, -delta.y,
                          0, 0, 1, -delta.z,
                          0, 0, 0, 1);

    return Transform(m, mInv);
}

Transform Scale(float x, float y, float z) {
    auto m = Matrix4x4(x, 0, 0, 0, 
                       0, y, 0, 0,
                       0, 0, z, 0, 
                       0, 0, 0, 1);

    auto mInv = Matrix4x4(1/x, 0, 0, 0,
                          0, 1/y, 0, 0,
                          0, 0, 1/z, 0,
                          0, 0, 0, 1);

    return Transform(m, mInv);
}

// 绕xyz轴旋转。逆矩阵就是T。
// 这里的符号问题？应该是跟约定有关。左右手

Transform RotateX(float theta) {
    auto sint = sin(theta);
    auto cost = cos(theta);

    auto m = Matrix4x4(1, 0, 0, 0,
                       0, cost, -sint, 0,
                       0, sint, cost, 0,
                       0, 0, 0, 1);

    return Transform(m, Transpose(m));
}

Transform RotateY(float theta) {
    auto sint = sin(theta);
    auto cost = cos(theta);

    auto m = Matrix4x4(cost, 0, -sint, 0,
                       0, 1, 0, 0,
                       sint, 0, cost, 0,
                       0, 0, 0, 1);

    return Transform(m, Transpose(m));
}

Transform RotateZ(float theta) {
    auto sint = sin(theta);
    auto cost = cos(theta);

    auto m = Matrix4x4(cost, -sint, 0, 0,
                       sint, cost, 0, 0,
                       0, 0, 1, 0,
                       0, 0, 0, 1);

    return Transform(m, Transpose(m));
}


Transform LookAt(const Point3f& eye, const Point3f& look, const Vector3f& up) {
    auto dir = Normalize(look - eye);

    if (Dot(dir, up) != 0) {
        return Transform();
    }

    // right handed
    auto right = Normalize(Cross(dir, up));
    auto up_n = Cross(right, dir);

    // left handed
    //auto right = Normalize(Cross(up, dir));
    //auto up_n = Cross(dir, right);

    // y-up
    /*auto a = Matrix4x4( right.x, right.y, right.z, 0,
                        up_n.x,  up_n.y,  up_n.z,  0,
                        dir.x,   dir.y,   dir.z,   0,
                        0,       0,       0,       1);*/

    // z-up
    auto a = Matrix4x4(
        right.x, right.y, right.z, 0,
        dir.x,   dir.y,   dir.z,   0,
        up_n.x,  up_n.y,  up_n.z,  0,
        0,       0,       0,       1);

    auto b = Matrix4x4(1, 0, 0, -eye.x, 
                       0, 1, 0, -eye.y,
                       0, 0, 1, -eye.z,
                       0, 0, 0, 1);

    auto lookat = Matrix4x4::Mul(a, b);

	return Transform(lookat, Inverse(lookat));
}

Transform Orthographic(float near, float far, float width, float height) {
    auto m = Matrix4x4(2/width, 0,        0,              0,
                       0,       2/height, 0,              0,
                       0,       0,        2/(far - near), -(far + near) / (far - near),
                       0,       0,        0,              1);

    return Transform(m, Inverse(m));
}

Transform Perspective(float fov, float asp, float n, float f) {
    float inv_tan = 1 / std::tan(Radians(fov) / 2);

    Matrix4x4 persp(inv_tan, 0,             0,                 0,
                    0,       asp * inv_tan, 0,                 0,
                    0,       0,             (f + n) / (f - n), 2 * f * n / (n - f),
                    0,       0,             1,                 0);


    return Transform(persp);
}





