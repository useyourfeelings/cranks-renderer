#include "sphere.h"
#include "../base/efloat.h"

BBox3f Sphere::ObjectBound() const {
	return BBox3f(Point3f(-radius, -radius, zMin), Point3f(radius, radius, zMax));
}

bool Sphere::Intersect(const Ray& ray, float* tHit, SurfaceInteraction* isect) const {
	ray.LogSelf();
	WorldToObject.LogSelf();

	Ray r;
	Point3f hitPoint;

	Vector3f pError;

	// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

	// 设s为球面上的交点
	// 对于球 ||s - c||=r
	// 对于ray s = o + dt
	// ||o + dt - c|| = r
	// c = 0, 0, 0
	// (o + dt) dot (o + dt) = r*r  自己点乘自己是长度的平方
	// 算出来得到 oo + 2tod + ttdd = rr
	// 解t的一元二次方程
	// a = dd
	// b = 2od
	// c = oo - rr
	// att + bt + c = 0

	int user_error = 1;

	if (user_error == 0) {
		// ray到模型空间。球在原点。
		r = WorldToObject(ray);
		r.LogSelf();

		float t1, t2, t;

		float a = Dot(r.d, r.d);

		float b = 2 * Dot(Vector3f(r.o.x, r.o.y, r.o.z), r.d);

		float c = Dot(Vector3f(r.o.x, r.o.y, r.o.z), Vector3f(r.o.x, r.o.y, r.o.z)) - radius * radius;

		Log("Sphere wtf1 %f %f %f", a, b, c);

		if (Quadratic(a, b, c, &t1, &t2) == 0)
			return false; // no root

		Log("Sphere wtf2 %f %f", t1, t2);

		if (t1 > r.tMax || t2 <= 0) // out of range
			return false;

		Log("Sphere wtf3");

		// find nearest t and check
		t = t1;
		if (t <= 0) {
			t = t2;
			if (t > r.tMax)
				return false;
		}

		Log("Sphere wtf4 %f", t);

		hitPoint = r(float(t)); // get hit position

		*tHit = t;
	}
	else {
		Vector3f oErr, dErr;

		// ray到模型空间。球在原点。
		r = WorldToObject(ray, &oErr, &dErr);
		//r.LogSelf();

		EFloat t1, t2, t;

		EFloat dx(r.d.x, dErr.x), dy(r.d.y, dErr.y), dz(r.d.z, dErr.z);
		EFloat ox(r.o.x, oErr.x), oy(r.o.y, oErr.y), oz(r.o.z, oErr.z);

		//float a = Dot(r.d, r.d);
		EFloat a = dx * dx + dy * dy + dz * dz;

		//float b = 2 * Dot(Vector3f(r.o.x, r.o.y, r.o.z), r.d);
		EFloat b = 2.f * (ox * dx + oy * dy + oz * dz);

		//float c = Dot(Vector3f(r.o.x, r.o.y, r.o.z), Vector3f(r.o.x, r.o.y, r.o.z)) - radius * radius;
		EFloat c = ox * ox + oy * oy + oz * oz - EFloat(radius) * EFloat(radius);

		//Log("Sphere wtf1 %f %f %f", a, b, c);

		if (Quadratic(a, b, c, &t1, &t2) == 0)
			return false; // no root

		//Log("Sphere wtf2 %f %f", t1, t2);

		if (t1.UpperBound() > r.tMax || t2.LowerBound() <= 0) // out of range
			return false;

		//Log("Sphere wtf3");

		// find nearest t and check
		t = t1;
		if (t.LowerBound() <= 0) {
			t = t2;
			if (t.UpperBound() > r.tMax)
				return false;
		}

		//Log("Sphere wtf4 %f", t);

		hitPoint = r(float(t)); // get hit position

		*tHit = t;

		// pbrt page 225
		// 可显著去除noise。
		// 
		// Refine sphere intersection point
		hitPoint *= radius / Distance(hitPoint, Point3f(0, 0, 0));

		//if (hitPoint.x == 0 && hitPoint.y == 0)
		//	hitPoint.x = 1e-5f * radius;

		pError = gamma(5) * Abs((Vector3f)hitPoint);
	}

	
	Log("hitPoint");
	hitPoint.LogSelf();

	// todo 

	/*
		x = r sin(θ) cos(φ)
		y = r sin(θ) sin(φ)
		z = r cos(θ)
	*/

	// radius default = 1
	// zmin default -radius
	// zmax default radius
	// thetaMin default acos(-1)   Pi 
	// thetaMax default acos(1)   2Pi
	// phimax default = 360      Radians(360) 2pi


	float phi = std::atan2(hitPoint.y, hitPoint.x);
	if (phi < 0)
		phi += 2 * Pi;

	float theta = std::acos(Clamp(hitPoint.z / radius, -1, 1));

	float u = phi / phiMax; // φ百分比
	float v = (theta - thetaMin) / (thetaMax - thetaMin); // θ从-1开始的百分比。总量2。

	// 横切面的半径
	float zRadius = std::sqrt(hitPoint.x * hitPoint.x + hitPoint.y * hitPoint.y);

	float cosPhi = hitPoint.x / zRadius;
	float sinPhi = hitPoint.y / zRadius;

	// 偏微分
	// 以u，也就是φ的百分比变化，作为微小变量，对球体方程求导。及dpdu。

	// 他为什么用百分比作为微小量而不是直接用φ？因为uv就是百分比，就是要针对百分比来算，用在texture映射中。

	// 微分的理解
	// 比如对x = r sin(θ) cos(φ)求u的偏导。得到x的变化率(之于φ的百分比)。
	// 同理得到yz的变化率。那么xyz在几何意义上实际是一个向量/方向。
	// 思考一下2d上的微分同样是一种方向。

	/*

	dx / du
	= d(r sin(θ) cos(φ)) / du
	= rsin(θ) d cos(u * phiMax) / du         // float u = phi / phiMax;代入
	= -rsin(θ)sin(φ)phiMax
	= -y * phiMax

	dy / du
	= d(r sin(θ) sin(φ)) / du
	= rsin(θ) d sin(u*phiMax) / du
	= rsin(θ)cos(φ)phiMax
	= x * phiMax

	dz / du
	= d(r cos(θ)) / du
	= 0

	*/

	// 最后得到dp/du。当前点上的
	Vector3f dpdu(-phiMax * hitPoint.y, phiMax * hitPoint.x, 0);

	// 这个是可以在普通xyz坐标系验证的。
	// 粗略看一下。θ不变的情况下，z是不变的。可简化成xy的2d平面。
	// 看看一个圆上切线方向。确实就是(-y, x)，方向能对上。如果严格按u来算的话想必结果是一致的。

	// 同样可算出dp/dv。

	/*
	
	dx / dv
	= d(r sin(θ) cos(φ)) / dv
	= rcos(φ) cos(θ) d((thetaMax - thetaMin) v + thetaMin) / dv
	= rcos(φ) cos(θ) (thetaMax - thetaMin)
	= z * cos(φ) * (thetaMax - thetaMin)

	dy / dv
	= d(r sin(θ) sin(φ)) / dv
	= rsin(φ) cos(θ) (thetaMax - thetaMin)
	= z * sin(φ) * (thetaMax - thetaMin)

	dz / dv
	= d (rcos(θ)) / dv
	= -r sin(θ) * (thetaMax - thetaMin)

	*/

	Vector3f dpdv = Vector3f(hitPoint.z * cosPhi, hitPoint.z * sinPhi, -radius * std::sin(theta)) 
		* (thetaMax - thetaMin);

	Log("dpdu");
	dpdu.LogSelf();

	Log("dpdv");
	dpdv.LogSelf();



	// 最后生成信息并转回world	
	*isect = ObjectToWorld(SurfaceInteraction(
		hitPoint, 
		pError,
		hitPoint - Point3f(0, 0, 0), 
		Point2f(u, v), -r.d, 
		dpdu, dpdv,
		r.time, this));

	

	return true;
}

bool Sphere::Intersect(const Ray& ray) const {

	// ray到local坐标。球在原点。
	Ray r = WorldToObject(ray);

	// https://en.wikipedia.org/wiki/Line%E2%80%93sphere_intersection

	float t1, t2, t;

	float a = Dot(r.d, r.d);

	float b = 2 * Dot(Vector3f(r.o.x, r.o.y, r.o.z), r.d);

	float c = Dot(Vector3f(r.o.x, r.o.y, r.o.z), Vector3f(r.o.x, r.o.y, r.o.z)) - radius * radius;

	if (Quadratic(a, b, c, &t1, &t2) == 0)
		return false; // no root

	if (t1 > r.tMax || t2 <= 0) // out of range
		return false;

	// find nearest t and check
	t = t1;
	if (t <= 0) {
		t = t2;
		if (t > r.tMax)
			return false;
	}

	return true;
}

Interaction Sphere::Sample() {
	return Interaction();
}

float Sphere::Pdf() {
	return 0;
}