#ifndef CORE_TRANSFORM_H
#define CORE_TRANSFORM_H

#include "geometry.h"

class Matrix4x4 {
public:
    Matrix4x4(float mat[4][4]);

    Matrix4x4() {
        m[0][0] = m[1][1] = m[2][2] = m[3][3] = 1.f;
        m[0][1] = m[0][2] = m[0][3] = m[1][0] = 
        m[1][2] = m[1][3] = m[2][0] = m[2][1] = 
        m[2][3] = m[3][0] = m[3][1] = m[3][2] = 0.f;
    }

    Matrix4x4(float t00, float t01, float t02, float t03, float t10, float t11,
        float t12, float t13, float t20, float t21, float t22, float t23,
        float t30, float t31, float t32, float t33);

    static Matrix4x4 Mul(const Matrix4x4& m1, const Matrix4x4& m2) {
        Matrix4x4 r;
        for (int i = 0; i < 4; ++i)
            for (int j = 0; j < 4; ++j)
                r.m[i][j] = m1.m[i][0] * m2.m[0][j] + m1.m[i][1] * m2.m[1][j] +
                m1.m[i][2] * m2.m[2][j] + m1.m[i][3] * m2.m[3][j];
        return r;
    }


	float m[4][4];
};

Matrix4x4 Transpose(const Matrix4x4& m);
Matrix4x4 Inverse(const Matrix4x4& m);

class Transform {
public:
	Transform() {}
    Transform(const Matrix4x4& m) : m(m), mInv(Inverse(m)) {}
    Transform(const Matrix4x4& m, const Matrix4x4& mInv) : m(m), mInv(mInv) {}

    inline Ray operator()(const Ray& r) const;
    Ray operator()(const Ray& r, Vector3f* oError, Vector3f* dError) const;

    RayDifferential operator()(const RayDifferential& r) const;

    BBox3f operator()(const BBox3f& b) const;

    template <typename T>
    inline Point3<T> operator()(const Point3<T>& pt) const;

    template <typename T>
    inline Point3<T> operator()(const Point3<T>& pt, Vector3<T>* absError) const;

    template <typename T>
    inline Vector3<T> operator()(const Vector3<T>& v) const;

    template <typename T>
    inline Vector3<T> operator()(const Vector3<T>& v, Vector3<T>* absError) const;

    Transform operator*(const Transform& t2) const;

    SurfaceInteraction operator()(const SurfaceInteraction& si) const;

    void LogSelf() const;

    Matrix4x4 m, mInv; // Õý·´
};




inline Transform Inverse(const Transform& t) {
    return Transform(t.mInv, t.m);
}





Vector3f Normalize(const Vector3f& v);
Transform Translate(const Vector3f& delta);
Transform Scale(float x, float y, float z);
Transform RotateX(float theta);
Transform RotateY(float theta);
Transform RotateZ(float theta);
Transform LookAt(const Point3f& pos, const Point3f& look, const Vector3f& up);
Transform Orthographic(float near, float far, float width, float height);
Transform Perspective(float fov, float asp, float znear, float zfar);
bool SolveLinearSystem2x2(const float A[2][2], const float B[2], float* x0, float* x1);



#endif