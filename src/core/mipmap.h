#ifndef CORE_MIPMAP_H
#define CORE_MIPMAP_H

#include "pbr.h"
#include "texture.h"
#include "spectrum.h"
#include <cmath>
#include <cstdlib>

enum class ImageWrap { Repeat, Black, Clamp };

struct ResampleWeight {
	int firstTexel;
	float weight[4];
};

// Material Declarations
template <typename T>
class MIPMap {
private:
	const bool doTrilinear;
	
	const ImageWrap wrapMode;

	std::vector<std::vector<T>> pyramid;

	std::unique_ptr<ResampleWeight[]> resampleWeights(int oldRes, int newRes);
	

public:
	MIPMap(const Point2i& resolution, const T* data, bool doTri = false,
		float maxAniso = 8.f, ImageWrap wrapMode = ImageWrap::Repeat);

	int getMipmapImage(int index, std::vector<unsigned char>& data, int& x, int& y);

	const T& Texel(int level, int s, int t) const;
	T bilinear(int level, const Point2f& st) const;
	T Lookup(const Point2f& st, float width = 0.f) const;

	int Levels() const {
		return pyramid.size();
	}

	Point2i resolution;
};

template <typename T>
MIPMap<T>::MIPMap(const Point2i& res, const T* img, bool doTrilinear,
	float maxAnisotropy, ImageWrap wrapMode)
	: doTrilinear(doTrilinear),
	//maxAnisotropy(maxAnisotropy),
	wrapMode(wrapMode),
	resolution(res) {

	std::cout << "MIPMap::MIPMap " << res.x <<" " << res.y << std::endl;

	std::unique_ptr<float[]> resampledImage = nullptr;

	// 如果不是2的次方。做resampling。
	if (!IsPowerOf2(resolution.x) || !IsPowerOf2(resolution.y)) {
		Point2i resPow2(RoundUpPow2(resolution.x), RoundUpPow2(resolution.y));

		std::unique_ptr<ResampleWeight[]> sWeights = resampleWeights(resolution.x, resPow2.y);
		resampledImage.reset(new float[resPow2.x * resPow2.y]);

		/*for (auto i : sWeights.get()) {
			std::cout << i.weight[0] << std::endl;
		}*/
	}


	int nLevels = 1 + int(std::log2(std::max(resolution.x, resolution.y)));

	pyramid.resize(nLevels);

	// build pyramid
	//pyramid[0] = (T*)_aligned_malloc(resolution.x * resolution.y * sizeof(T), 64);
	//pyramid[0] = (T*)new unsigned char[resolution.x * resolution.y * sizeof(T)];
	pyramid[0].resize(resolution.x * resolution.y);
	
	/*for (int i = 0; i < resolution.x; += i) {
		for (int j = 0; j < resolution.y; += j) {
			pyramid[0][i + j * resolution.x] = T(img);
		}
	}*/

	//memcpy(pyramid[0], img, resolution.x * resolution.y * sizeof(T));   will cause access error related to vector

	for (int i = 0; i < resolution.x * resolution.y; ++i) {
		//std::cout << i << std::endl;
		//auto a = pyramid[0][i];// = T(img[i]);

		pyramid[0][i] = img[i];
	}

	int current_res_x = resolution.x / 2;
	int current_res_y = resolution.y / 2;

	for (int level = 1; level < nLevels; ++level) {
		std::cout << "level = "<< level <<  " nLevels = "<< nLevels << std::endl;


		//auto t = _aligned_malloc(current_res_x * current_res_y * sizeof(T), 64);
		//pyramid[level] = (T*)new unsigned char[current_res_x * current_res_y * sizeof(T)];
		pyramid[level].resize(current_res_x * current_res_y);

		//pyramid[level] = (T*)t;

		for (int j = 0; j < current_res_y; ++ j) {
			for (int i = 0; i < current_res_x; ++ i) {
				if (level == 3) {
					//std::cout << level << " " << i << " " << j << " " << current_res_x << " " << current_res_y << std::endl;

					/*std::cout << i * 2 + j * current_res_x * 4 << std::endl;
					std::cout << i * 2 + j * current_res_x * 4 + 1 << std::endl;
					std::cout << i * 2 + j * current_res_x * 6 << std::endl;
					std::cout << i * 2 + j * current_res_x * 6 + 1 << std::endl;*/

					/*auto a = pyramid[level - 1][i * 2 + 4 * j * current_res_x];
					a = pyramid[level - 1][i * 2 + 4 * j * current_res_x + 1];
					a = pyramid[level - 1][i * 2 + (4 * j + 2) * current_res_x];
					a = pyramid[level - 1][i * 2 + (4 * j + 2) * current_res_x + 1];*/

					//pyramid[level][i + j * current_res_x] = T();
				}

				

				/*auto a = pyramid[level - 1][i * 2 + j * current_res_x * 4];
				a = pyramid[level - 1][i * 2 + j * current_res_x * 4 + 1];
				a = pyramid[level - 1][i * 2 + j * current_res_x * 6];
				a = pyramid[level - 1][i * 2 + j * current_res_x * 6 + 1];*/

				pyramid[level][i + j * current_res_x] = (
					pyramid[level - 1][i * 2 + 4 * j * current_res_x] +
					pyramid[level - 1][i * 2 + 4 * j * current_res_x + 1] +
					pyramid[level - 1][i * 2 + (4 * j + 2) * current_res_x] +
					pyramid[level - 1][i * 2 + (4 * j + 2) * current_res_x + 1]) * 0.25f;
			}
		}

		current_res_x /= 2;
		current_res_y /= 2;
	}

}

template <typename T>
int MIPMap<T>::getMipmapImage(int index, std::vector<unsigned char>& data, int& x, int& y) {
	x = resolution.x / std::pow(2, index);
	y = resolution.y / std::pow(2, index);

	data.resize(x * y * 4);

	std::cout << "getMipmapImage "<< x <<" "<<y<<" index = " << index << std::endl;
	std::cout << x << " " << y << std::endl;

	for (int i = 0; i < x * y; ++i) {
		//std::cout << i << std::endl;
		//auto a = pyramid[index][i];
		//std::cout << pyramid[index][i].c[0] << " " << pyramid[index][i].c[1] << " " << pyramid[index][i].c[2] << std::endl;
		
		data[i * 4] =     (unsigned char)(pyramid[index][i].c[0] * 255.f);
		data[i * 4 + 1] = (unsigned char)(pyramid[index][i].c[1] * 255.f);
		data[i * 4 + 2] = (unsigned char)(pyramid[index][i].c[2] * 255.f);
		data[i * 4 + 3] = 255;

		//std::cout << i << std::endl;
		//std::cout << int(data[i * 4]) <<" "<< int(data[i * 4 + 1]) <<" "<< int(data[i * 4 + 2])<<" "<< int(data[i * 4 + 3]) << std::endl;
	}

	return 0;
}

template <typename T>
std::unique_ptr<ResampleWeight[]> MIPMap<T>::resampleWeights(int oldRes, int newRes) {
	//CHECK_GE(newRes, oldRes);

	// 起weight数据
	std::unique_ptr<ResampleWeight[]> wt(new ResampleWeight[newRes]);

	float filterwidth = 2.f;
	for (int i = 0; i < newRes; ++i) {
		// Compute image resampling weights for _i_th texel
		// 
		// 每一个新点对应的老的index作为center。
		float center = (i + .5f) * oldRes / newRes;
		wt[i].firstTexel = std::floor((center - filterwidth) + 0.5f);
		for (int j = 0; j < 4; ++j) {
			float pos = wt[i].firstTexel + j + .5f;

			// center为中心的4个点，算weight。
			// 按说结果是一个周期形式，不用一个个都算出来的。
			wt[i].weight[j] = Lanczos((pos - center) / filterwidth);
		}

		// Normalize filter weights for texel resampling
		float invSumWts = 1 / (wt[i].weight[0] + wt[i].weight[1] +
			wt[i].weight[2] + wt[i].weight[3]);
		for (int j = 0; j < 4; ++j) {
			wt[i].weight[j] *= invSumWts;
		}

		std::cout << wt[i].weight[0] <<" " << wt[i].weight[1]<<" "<< wt[i].weight[2]<<" "<< wt[i].weight[3] << std::endl;
	}
	return wt;
}

template <typename T>
const T& MIPMap<T>::Texel(int level, int s, int t) const {
	int x = resolution.x / std::pow(2, level);
	int y = resolution.y / std::pow(2, level);

	switch (wrapMode) {
	case ImageWrap::Repeat:
		s = Mod(s, x);
		t = Mod(t, y);
		break;
	case ImageWrap::Clamp:
		s = Clamp(s, 0, x - 1);
		t = Clamp(t, 0, y - 1);
		break;
	}

	//std::cout << "s t " << s << " " << t << std::endl;

	return pyramid[level][s + x * t];
}

template <typename T>
T MIPMap<T>::bilinear(int level, const Point2f& st) const {
	level = Clamp(level, 0, Levels() - 1);

	int x = resolution.x / std::pow(2, level);
	int y = resolution.y / std::pow(2, level);

	//std::cout << "bilinear level = "<<level<<" s t = " << st.x << " " << st.y <<" x y = "<<x<<" "<<y << std::endl;

	float s = st.x * x - 0.5f;
	float t = st.y * y - 0.5f;
	int s0 = std::floor(s), t0 = std::floor(t);
	float ds = s - s0, dt = t - t0;
	return (1 - ds) * (1 - dt) * Texel(level, s0, t0) +
		(1 - ds) * dt * Texel(level, s0, t0 + 1) +
		ds * (1 - dt) * Texel(level, s0 + 1, t0) +
		ds * dt * Texel(level, s0 + 1, t0 + 1);
}

// 以xy百分比查找
template <typename T>
T MIPMap<T>::Lookup(const Point2f& st, float width) const {
	//++nTrilerpLookups;
	//ProfilePhase p(Prof::TexFiltTrilerp);
	// 
	// Compute MIPMap level for trilinear filtering
	float level = Levels() - 1 + std::log2(std::max(width, (float)1e-8));

	// Perform trilinear interpolation at appropriate MIPMap level
	if (level < 0)
		return bilinear(0, st);
	else if (level >= Levels() - 1)
		return Texel(Levels() - 1, 0, 0);
	else {
		int iLevel = std::floor(level);
		float delta = level - iLevel;
		return Lerp(delta, bilinear(iLevel, st), bilinear(iLevel + 1, st));
	}
}

/////////////////////////////////////////




#endif