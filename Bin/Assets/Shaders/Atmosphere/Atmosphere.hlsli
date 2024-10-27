#pragma once
#include "../Math.hlsli"

namespace Atmosphere {

struct AtmosphereParameter {
    float   seaLevel;                           // 海平面高度
    float   planetRadius;                       // 星球的半径
    float   atmosphereHeight;                   // 大气的高度
    float   sunLightIntensity;
    float3  sunLightColor;
    float   sunDiskAngle;
    float   rayleighScatteringScalarHeight;     // 瑞利散射标高,通常是 8500
    float   mieAnisotropy;                      // Mie 散射的各项异性 (0 ~ 1)
    float   mieScatteringScalarHeight;          // Mie 散射标高, 通常是 1200
    float   ozoneLevelCenterHeight;             // 臭氧的中心, 通常是 25km
    float   ozoneLevelWidth;                    // 臭氧的宽度, 通常是 15km
};

// 瑞丽散射系数函数
float3 RayleighCoefficient(in AtmosphereParameter param, float h) {
    const float3 sigma = float3(5.802, 13.558, 33.1) * 1e-6;
    float H_R = param.rayleighScatteringScalarHeight;
    float rho_h = exp(-(h / H_R));
    return sigma * rho_h;
}

// 瑞丽散射相位函数
float RayleighPhase(float cosTheta) {
    return (3.0 /  (16.0 * Math::PI)) * (1.0 + cosTheta * cosTheta);
}

// Mie 散射系数函数
float3 MieCoefficient(in AtmosphereParameter param, float h) {
    const float3 sigma = (3.996 * 1e-6);
    float rho_H = param.mieScatteringScalarHeight;
    float rho_h = exp(- (h / rho_H));
    float result = sigma * rho_h;
    return result;
}

// Mie 散射相位函数
float MiePhase(in AtmosphereParameter param, float cosTheta) {
    float g = param.mieAnisotropy;
    float a = 3.0 / (8.0 * Math::PI);
    float b = (1.0 - g*g) / (2.0 + g+g);
    float c = 1.0 + cosTheta * cosTheta;
    float d = pow(1.0 + g*g - 2 * g * cosTheta, 1.5);
    float result = a * b * (c / d);
    return result;
}

/**
 * \brief 散射函数: 结合 Rayleigh 散射和 Mie 散射
 *        瑞利相位 * 瑞利散射系数 + Mie散射相位 * Mie散射系数
 */
float3 CalcScattering(in AtmosphereParameter param, 
                      float3 pos, 
                      float cosTheta)
{
    float posVecLen = length(pos);
    float h = posVecLen - param.planetRadius;
    float3 rayleighCoefficient = RayleighCoefficient(param, h);
    float rayleighPhase = RayleighPhase(cosTheta);
    float3 rayleigh = rayleighCoefficient * rayleighPhase;

    float3 mieCoefficient = MieCoefficient(param, h);
    float miePhase = MiePhase(param, cosTheta);
    float3 mie = mieCoefficient * miePhase;
    float3 result = rayleigh + mie;
    return result;
}

// Mie 散射的吸收
float3 MieAbsorption(in AtmosphereParameter param, float h) {
    const float3 sigma = 4.4 * 1e-6;
    float H_M = param.mieScatteringScalarHeight;
    float rho_h = exp(-(h / H_M));
    float3 result = sigma * rho_h;
    return result;
}

// 臭氧的吸收
float3 OzoneAbsorption(in AtmosphereParameter param, float h) {
    const float3 sigma = float3(0.650, 1.881, 0.085) * 1e-6;
    float center = param.ozoneLevelCenterHeight;
    float width = param.ozoneLevelWidth;
    float rho = max(0, 1 - (abs(h - center) / width));
    float3 result = sigma * rho;
    return result;
}

// 计算湮灭
// 湮灭 = 散射系数(瑞利系数 + Mie系数) + 吸收(臭氧吸收 + Mie散射吸收)
float3 GetExtinction(in AtmosphereParameter param, float h) {
	float3 rayleighCoefficient = RayleighCoefficient(param, h);
    float3 mieCoefficient = MieCoefficient(param, h);
    float3 scattering = rayleighCoefficient + mieCoefficient;
    float3 ozoneAbsorption = OzoneAbsorption(param, h);
    float3 mieAbsorption = MieAbsorption(param, h);
    float3 absorption = ozoneAbsorption + mieAbsorption;
    float3 result = scattering + absorption;
    return result;
}

float3 CalcTransmittance(in AtmosphereParameter param, float3 startPos, float3 endPos) {
    const int SampleCount = 32;
    float3 dir = normalize(endPos - startPos);
    float distance = length(endPos - startPos);
    float ds = distance / float(SampleCount);
    float3 sum = 0.f;
    float3 p = startPos + (ds * 0.5 * dir) ;
    for (int i = 0; i < SampleCount; ++i) {
        float posVecLen = length(p);
        float h = posVecLen - param.planetRadius;
        float3 extinction = GetExtinction(param, h);
        sum += extinction * ds;
        p += dir * ds;
    }
    float3 result = exp(-sum);
    return result;
}

float RayIntersectSphere(float3 center, float radius, float3 rayStart, float3 rayDir) {
    float OS = length(center - rayStart);
    float SH = dot(center - rayStart, rayDir);
    float OH = sqrt(OS*OS - SH*SH);
    float PH = sqrt(radius*radius - OH*OH);

    // ray miss sphere
    if(OH > radius) {
		return -1;
    }

    // use min distance
    float t1 = SH - PH;
    float t2 = SH + PH;
    float t = (t1 < 0.f) ? t2 : t1;
    return t;
}

float3 IntegrateSignalScatting(in AtmosphereParameter param,
	in float3 cameraWorldPos,
    in float3 lightDir,
    in float3 viewDir)
{
	const int N_SAMPLE = 32;
    const float INV_N_SAMPLE = 1.f / float(N_SAMPLE);
    // 假设相机一直处于星球的中心, 但是可以允许高度不同
    float3 p0 = float3(0.f, cameraWorldPos.y - param.seaLevel + param.planetRadius, 0.f);

    /*
     *          p2
     *           \
     *            \
     *   p1<-------p--------p0
     *   p2 是大气层中的交点. p0 是相机的位置, p1 是摄像机看向大气层的交点, p 是沿途需要积分的点
     */
    const float sphereRadius = param.planetRadius + param.atmosphereHeight;
    float disToAtmosphereSphere = RayIntersectSphere(0.f, sphereRadius, p0, viewDir);
    if (disToAtmosphereSphere < 0.f) {
	    return 0.f;
    }

    float t = disToAtmosphereSphere;
    float disToPlanetSphere = RayIntersectSphere(0.f, param.planetRadius, p0, viewDir);
    if (disToPlanetSphere > 0) {
	    t = min(t, disToPlanetSphere);
    }

    float ds = t * INV_N_SAMPLE;
    float3 p1 = p0 + viewDir * t;
    float3 d = (p1 - p0) * INV_N_SAMPLE;
    float3 p = p0 + (viewDir * ds) * 0.5;
    float cosTheta = dot(viewDir, lightDir);
    float3 sunLuminance = param.sunLightColor * param.sunLightIntensity;

    float3 color = 0.f;
    for (int i = 0; i < N_SAMPLE-1; ++i) {
		float disToAtmosphere = RayIntersectSphere(0.f, sphereRadius, p, lightDir);
        float3 p2 = p + lightDir * disToAtmosphere;
        float3 t1 = CalcTransmittance(param, p2, p);
        float3 s = CalcScattering(param, p, cosTheta);
        float3 t2 = CalcTransmittance(param, p, p0);
        float3 inScattering = t1 * s * t2 * ds * sunLuminance;
        color += inScattering;
        p += d;
    }
    return color; 
}


float3 UVToViewDir(float2 uv) {
    float theta = (1.f - uv.y) * Math::PI;
    float phi = (uv.x * 2.f - 1.f) * Math::PI;
    float sinTheta = sin(theta);
    float x = sinTheta * cos(phi);
    float z = sinTheta * sin(phi);
    float y = cos(theta);
    return float3(x, y, z);
}

float2 ViewDirToUV(float3 v) {
    float2 uv = float2(atan2(v.z, v.x), asin(v.y));
    uv /= float2(2.f * Math::PI, Math::PI);
    uv += float2(0.5f, 0.5f);
    return uv; 
}

}
