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

	// ��sΪ�����ϵĽ���
	// ������ ||s - c||=r
	// ����ray s = o + dt
	// ||o + dt - c|| = r
	// c = 0, 0, 0
	// (o + dt) dot (o + dt) = r*r  �Լ�����Լ��ǳ��ȵ�ƽ��
	// ������õ� oo + 2tod + ttdd = rr
	// ��t��һԪ���η���
	// a = dd
	// b = 2od
	// c = oo - rr
	// att + bt + c = 0

	int user_error = 1;

	if (user_error == 0) {
		// ray��ģ�Ϳռ䡣����ԭ�㡣
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

		// ray��ģ�Ϳռ䡣����ԭ�㡣
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
		// ������ȥ��noise��
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
		x = r sin(��) cos(��)
		y = r sin(��) sin(��)
		z = r cos(��)
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

	float u = phi / phiMax; // �հٷֱ�
	float v = (theta - thetaMin) / (thetaMax - thetaMin); // �ȴ�-1��ʼ�İٷֱȡ�����2��

	// ������İ뾶
	float zRadius = std::sqrt(hitPoint.x * hitPoint.x + hitPoint.y * hitPoint.y);

	float cosPhi = hitPoint.x / zRadius;
	float sinPhi = hitPoint.y / zRadius;

	// ƫ΢��
	// ��u��Ҳ���Ǧյİٷֱȱ仯����Ϊ΢С�����������巽���󵼡���dpdu��

	// ��Ϊʲô�ðٷֱ���Ϊ΢С��������ֱ���æգ���Ϊuv���ǰٷֱȣ�����Ҫ��԰ٷֱ����㣬����textureӳ���С�

	// ΢�ֵ����
	// �����x = r sin(��) cos(��)��u��ƫ�����õ�x�ı仯��(֮�ڦյİٷֱ�)��
	// ͬ��õ�yz�ı仯�ʡ���ôxyz�ڼ���������ʵ����һ������/����
	// ˼��һ��2d�ϵ�΢��ͬ����һ�ַ���

	/*

	dx / du
	= d(r sin(��) cos(��)) / du
	= rsin(��) d cos(u * phiMax) / du         // float u = phi / phiMax;����
	= -rsin(��)sin(��)phiMax
	= -y * phiMax

	dy / du
	= d(r sin(��) sin(��)) / du
	= rsin(��) d sin(u*phiMax) / du
	= rsin(��)cos(��)phiMax
	= x * phiMax

	dz / du
	= d(r cos(��)) / du
	= 0

	*/

	// ���õ�dp/du����ǰ���ϵ�
	Vector3f dpdu(-phiMax * hitPoint.y, phiMax * hitPoint.x, 0);

	// ����ǿ�������ͨxyz����ϵ��֤�ġ�
	// ���Կ�һ�¡��Ȳ��������£�z�ǲ���ġ��ɼ򻯳�xy��2dƽ�档
	// ����һ��Բ�����߷���ȷʵ����(-y, x)�������ܶ��ϡ�����ϸ�u����Ļ���ؽ����һ�µġ�

	// ͬ�������dp/dv��

	/*
	
	dx / dv
	= d(r sin(��) cos(��)) / dv
	= rcos(��) cos(��) d((thetaMax - thetaMin) v + thetaMin) / dv
	= rcos(��) cos(��) (thetaMax - thetaMin)
	= z * cos(��) * (thetaMax - thetaMin)

	dy / dv
	= d(r sin(��) sin(��)) / dv
	= rsin(��) cos(��) (thetaMax - thetaMin)
	= z * sin(��) * (thetaMax - thetaMin)

	dz / dv
	= d (rcos(��)) / dv
	= -r sin(��) * (thetaMax - thetaMin)

	*/

	Vector3f dpdv = Vector3f(hitPoint.z * cosPhi, hitPoint.z * sinPhi, -radius * std::sin(theta)) 
		* (thetaMax - thetaMin);

	Log("dpdu");
	dpdu.LogSelf();

	Log("dpdv");
	dpdv.LogSelf();



	// ���������Ϣ��ת��world	
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

	// ray��local���ꡣ����ԭ�㡣
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