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
float Rayleigh(float cosTheta) {
    return (3.0 /  (16.0 * 3.1415926f)) * (1.0 + cosTheta * cosTheta);
}

// Mie 散射系数函数
float3 MieCoefficient(float h) {
    const float3 sigma = (3.996 * 1e-6);
    float rho_H = 1200;
    float rho_h = exp(- (h / rho_H));
    return sigma * rho_h;
}

// Mie 散射相位函数
float MiePhase(in AtmosphereParameter param, float cosTheta) {
    float g = param.mieAnisotropy;
    float a = 3.0 / (8.0 * 3.1415926);
    float b = (1.0 - g*g) / (2.0 + g+g);
    float c = 1.0 + cosTheta * cosTheta;
    float d = pow(1.0 + g*g - 2 * g * cosTheta, 1.5);
    return a * b * (c / d);
}

/**
 * \brief 散射函数: 结合 Rayleigh 散射和 Mie 散射
 *        瑞利相位 * 瑞利散射系数 + Mie散射相位 * Mie散射系数
 */
float3 CalcScattering(in AtmosphereParameter param, 
                      float3 pos, 
                      float3 lightToPointDir, 
                      float3 pointToCameraDir)
{
    float cosTheta = dot(lightToPointDir, pointToCameraDir);
    float h = length(pos) - param.planetRadius;
    float3 rayleigh = RayleighCoefficient(param, h) * Rayleigh(cosTheta);
    float3 mie = MieCoefficient(h) * MiePhase(param, cosTheta);
    return rayleigh + mie;
}

// Mie 散射的吸收
float3 MieAbsorption(in AtmosphereParameter param, float h) {
    const float3 sigma = 4.4 * 1e-6;
    float H_M = param.mieScatteringScalarHeight;
    float rho_h = exp(-(h / H_M));
    return sigma * rho_h;
}

// 臭氧的吸收
float3 OzoneAbsorption(in AtmosphereParameter param, float h) {
    const float3 sigma = float3(0.650, 1.881, 0.085) * 1e-6;
    float center = param.ozoneLevelCenterHeight;
    float width = param.ozoneLevelWidth;
    float rho = max(0, 1 - (abs(h - center) / width));
    return sigma * rho;
}

// 计算湮灭
// 湮灭 = 散射系数(瑞利系数 + Mie系数) + 吸收(臭氧吸收 + Mie散射吸收)
float3 GetExtinction(in AtmosphereParameter param, float h) {
    float3 scattering = RayleighCoefficient(param, h) + MieCoefficient(h);
    float3 absorption = OzoneAbsorption(param, h) + MieAbsorption(param, h);
    return scattering + absorption;
}

float3 CalcTransmittance(in AtmosphereParameter param, float3 startPos, float3 endPos) {
    const int SampleCount = 32;
    float3 dir = normalize(endPos - startPos);
    float distance = length(endPos - startPos);
    float ds = distance / float(SampleCount);
    float3 sum = 0.f;
    float3 p = startPos + (ds * 0.5 * dir) ;
    for (int i = 0; i < SampleCount; ++i) {
        float h = length(p) - param.planetRadius;
        float3 extinction = GetExtinction(param, h);
        sum += extinction * ds;
        p += dir * ds;
    }
    return exp(-sum);
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
    float t = (t1 < 0) ? t2 : t1;
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
    float t = RayIntersectSphere(0.f, sphereRadius, p0, viewDir);
    float3 p1 = p0 + viewDir * t;
    float3 d = (p1 - p0) * INV_N_SAMPLE;
    float3 p = p1;

    float color = 0.f;
    for (int i = 0; i < N_SAMPLE; ++i) {
		float disToAtmosphere = RayIntersectSphere(0.f, sphereRadius, p, lightDir);
        float3 p2 = p + lightDir * disToAtmosphere;
        float3 t1 = CalcTransmittance(param, p2, p);
        float3 s = CalcScattering(param, p, -lightDir, -viewDir);
        float3 t2 = CalcTransmittance(param, p, p0);
        float3 inScattering = t1 * s * t2;
        color += inScattering;
        p += d;
    }

    color *= param.sunLightColor * param.sunLightIntensity * INV_N_SAMPLE;
    return color; 
}