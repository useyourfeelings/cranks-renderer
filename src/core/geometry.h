#ifndef CORE_GEOMETRY_H
#define CORE_GEOMETRY_H

#include "pbr.h"
#include <string>
#include "../base/efloat.h"

template <typename T>
class Point3;

template <typename T>
class Vector3 {
public:
	Vector3() {
		x = y = z = 0.0;
	}

	Vector3(T x, T y, T z):x(x), y(y), z(z) {
	}

	explicit Vector3(const Point3<T>& p) 
		: x(p.x), y(p.y), z(p.z) {
	}

	Vector3(const Vector3<T>& v) {
		//DCHECK(!v.HasNaNs());
		x = v.x;
		y = v.y;
		z = v.z;
	}

	Vector3<T> operator-() const { 
		return Vector3<T>(-x, -y, -z);
	}

	Vector3<T> operator+(const Vector3<T>& v) const {
		return Vector3<T>(x + v.x, y + v.y, z + v.z);
	}

	Vector3<T> operator-(const Vector3<T>& v) const {
		return Vector3<T>(x - v.x, y - v.y, z - v.z);
	}

	Vector3<T> operator/(float a) const {
		return Vector3<T>(x / a, y / a, z / a);
	}

	template <typename U>
	Vector3<T> operator*(U s) const {
		return Vector3<T>(s * x, s * y, s * z);
	}

	template <typename U>
	Vector3<T> operator/(U s) const {
		return Vector3<T>(x / s, y / s, z / s);
	}

	Vector3<T>& operator=(const Vector3<T>& v) {
		//DCHECK(!v.HasNaNs());
		x = v.x;
		y = v.y;
		z = v.z;
		return *this;
	}

	float Length() const {
		return sqrt(x * x + y * y + z * z);
	}

	float LengthSquared() const {
		return x * x + y * y + z * z; 
	}

	void LogSelf(const std::string &msg = std::string()) {
		Log("%s Vector3 %f %f %f", msg, x, y, z);
	}

	float x, y, z;
};

template <typename T>
class Point3 {
public:
	Point3() {
		x = y = z = 0;
	}

	Point3(T x, T y, T z) :x(x), y(y), z(z) {
	}

	Point3(const Point3<T>& p) {
		//DCHECK(!p.HasNaNs());
		x = p.x;
		y = p.y;
		z = p.z;
	}

	//template <typename U>
	//explicit Point3(const Point3<U>& p)
	//	: x((T)p.x), y((T)p.y), z((T)p.z) {
	//	//DCHECK(!HasNaNs());
	//}

	Vector3<T> operator-(const Point3<T>& p) const {
		return Vector3<T>(x - p.x, y - p.y, z - p.z);
	}

	template <typename U>
	Vector3<T> operator*(U s) const {
		return Vector3<T>(s * x, s * y, s * z);
	}

	template <typename U>
	Point3<T> operator/(U f) const {
		return Point3<T>(x / f, y / f, z / f);
	}

	Point3<T> operator+(const Vector3<T>& v) const {
		return Point3<T>(x + v.x, y + v.y, z + v.z);
	}

	Point3<T>& operator+=(const Vector3<T>& v) {
		//DCHECK(!v.HasNaNs());
		x += v.x;
		y += v.y;
		z += v.z;
		return *this;
	}

	Point3<T> operator-(const Vector3<T>& v) const {
		return Point3<T>(x + v.x, y + v.y, z + v.z);
	}

	void LogSelf() {
		Log("Point3 %f %f %f", x, y, z);
	}

	T x, y, z;
};


template <typename T>
class Point2 {
public:
	explicit Point2(const Point3<T>& p) : x(p.x), y(p.y) { }
	Point2() { x = y = 0; }
	Point2<T>(T xx, T yy) : x(xx), y(yy) {  }

	Point2<T> operator+(const Point2<T>& p) const {
		return Point2<T>(x + p.x, y + p.y);
	}

	T x, y;
};



template <typename T>
class BBox2 {
public:
	BBox2() {
		T minNum = std::numeric_limits<T>::lowest();
		T maxNum = std::numeric_limits<T>::max();
		pMin = Point2<T>(maxNum, maxNum);
		pMax = Point2<T>(minNum, minNum);
	}
	explicit BBox2(const Point2<T>& p) : pMin(p), pMax(p) {}
	BBox2(const Point2<T>& p1, const Point2<T>& p2) {
		pMin = Point2<T>(std::min(p1.x, p2.x), std::min(p1.y, p2.y));
		pMax = Point2<T>(std::max(p1.x, p2.x), std::max(p1.y, p2.y));
	}

	Point2<T> pMin, pMax;
};

template <typename T>
class BBox3 {
public:
	// Bounds3 Public Methods
	BBox3() {
		T minNum = std::numeric_limits<T>::lowest();
		T maxNum = std::numeric_limits<T>::max();
		pMin = Point3<T>(maxNum, maxNum, maxNum);
		pMax = Point3<T>(minNum, minNum, minNum);
	}
	explicit BBox3(const Point3<T>& p) : pMin(p), pMax(p) {}
	BBox3(const Point3<T>& p1, const Point3<T>& p2)
		: pMin(std::min(p1.x, p2.x), std::min(p1.y, p2.y),
			std::min(p1.z, p2.z)),
		pMax(std::max(p1.x, p2.x), std::max(p1.y, p2.y),
			std::max(p1.z, p2.z)) {}

	Point3<T> pMin, pMax;
};

//typedef Vector2<float> Vector2f;
//typedef Vector2<int> Vector2i;
typedef Vector3<float> Vector3f;
typedef Vector3<int> Vector3i;
typedef Point2<float> Point2f;
typedef Point2<int> Point2i;
typedef Point3<float> Point3f;
typedef Point3<int> Point3i;

typedef BBox2<float> BBox2f;
typedef BBox2<int> BBox2i;
typedef BBox3<float> BBox3f;
typedef BBox3<int> BBox3i;

template <typename T>
inline Vector3<T> Cross(const Vector3<T>& v1, const Vector3<T>& v2) {
	return Vector3<T>(v1.y * v2.z - v1.z * v2.y,
		v1.z * v2.x - v1.x * v2.z,
		v1.x * v2.y - v1.y * v2.x);
}

template <typename T>
Vector3<T> Abs(const Vector3<T>& v) {
	return Vector3<T>(std::abs(v.x), std::abs(v.y), std::abs(v.z));
}

template <typename T>
inline float Dot(const Vector3<T>& v1, const Vector3<T>& v2) {
	return v1.x * v2.x + v1.y * v2.y + v1.z * v2.z;
}

template <typename T>
inline T AbsDot(const Vector3<T>& v1, const Vector3<T>& n2) {
	//DCHECK(!v1.HasNaNs() && !n2.HasNaNs());
	return std::abs(v1.x * n2.x + v1.y * n2.y + v1.z * n2.z);
}

template <typename T>
inline float DistanceSquared(const Point3<T>& p1, const Point3<T>& p2) {
	return (p1 - p2).LengthSquared();
}

template <typename T, typename U>
inline Vector3<T> operator*(U s, const Vector3<T>& v) {
	return v * s;
}

template <typename T>
inline Vector3<T> Faceforward(const Vector3<T>& v, const Vector3<T>& v2) {
	return (Dot(v, v2) < 0.f) ? -v : v;
}

class Ray {
public:
	Ray() : tMax(Infinity), time(0.f) {}
	Ray(const Point3f& o, const Vector3f& d, float tMax = Infinity,
		float time = 0.f) :o(o), d(d), tMax(tMax), time(time) {
	}

	Point3f operator()(float t) const { return o + d * t; }

	void LogSelf() const {
		Log("Ray %f %f %f, %f %f %f", o.x, o.y, o.z, d.x, d.y, d.z);
	}

	Point3f o;
	Vector3f d;
	float tMax; // restrict ray length
	float time; // for animation
};

class RayDifferential : public Ray {
public:
	// RayDifferential Public Methods
	RayDifferential() { hasDifferentials = false; }
	RayDifferential(const Point3f& o, const Vector3f& d, float tMax = Infinity,
		float time = 0.f)
		: Ray(o, d, tMax, time) {
		hasDifferentials = false;
	}
	RayDifferential(const Ray& ray) : Ray(ray) { hasDifferentials = false; }

	// Ã»¶®
	void ScaleDifferentials(float s) {
		rxOrigin = o + (rxOrigin - o) * s;
		ryOrigin = o + (ryOrigin - o) * s;
		rxDirection = d + (rxDirection - d) * s;
		ryDirection = d + (ryDirection - d) * s;
	}

	void LogSelf() const {
		Log("RayDifferential %f %f %f, %f %f %f", o.x, o.y, o.z, d.x, d.y, d.z);
	}
	
	// RayDifferential Public Data
	bool hasDifferentials;
	Point3f rxOrigin, ryOrigin;
	Vector3f rxDirection, ryDirection;
};


inline Point3f OffsetRayOrigin(const Point3f& p, const Vector3f& pError, const Vector3f& n, const Vector3f& w) {
	float d = Dot(Abs(n), pError);
	Vector3f offset = d * Vector3f(n);
	if (Dot(w, n) < 0) offset = -offset;
	Point3f po = p + offset;
	// Round offset point _po_ away from _p_
	/*for (int i = 0; i < 3; ++i) {
		if (offset[i] > 0)
			po[i] = NextFloatUp(po[i]);
		else if (offset[i] < 0)
			po[i] = NextFloatDown(po[i]);
	}*/

	if (offset.x > 0)
		po.x = NextFloatUp(po.x);
	else if (offset.x < 0)
		po.x = NextFloatDown(po.x);

	if (offset.y > 0)
		po.y = NextFloatUp(po.y);
	else if (offset.y < 0)
		po.y = NextFloatDown(po.y);

	if (offset.z > 0)
		po.z = NextFloatUp(po.z);
	else if (offset.z < 0)
		po.z = NextFloatDown(po.z);


	return po;
}


#endif // !CORE_GEOMETRY_H