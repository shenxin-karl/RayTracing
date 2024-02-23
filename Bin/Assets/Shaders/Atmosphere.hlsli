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
float3 RayleighCoefficient(in AtomsphereParameter param, float h) {
    const float sigma = float3(5.802, 13.558, 33.1) * 1e-6;
    float H_R = param.rayleighScatteringScalarHeight;
    float rho_h = exp(-(h / H_R));
    return sigma * rho_h;
}

// 瑞丽散射相位函数
float Rayleigh(float cosTheta) {
    return (3.0 /  (16.0 * 3.1415926f)) * (1.0 + cosTheat * cosTheta);
}

// Mie 散射系数函数
float3 MieCoefficient(float h) {
    const float3 sigma = (3.996 * 1e-6);
    float rho_H = 1200;
    float rho_h = exp(- (h / rho_H));
    return sigma * rho_h;
}

// Mie 散射相位函数
float MiePhase(in AtomsphereParameter param, float cosTheta) {
    float g = param.mieAnisotropy;
    float a = 3.0 / (8.0 * PI);
    float b = (1.0 - g*g) / (2.0 + g+g);
    float c = 1.0 + cosTheta * cosTheta;
    float d = pow(1.0 + g*g - 2 * g * cosTheta, 1.5);
    return a * b * (c / d);
}

// 散射函数: 结合 Rayleigh 散射和 Mie 散射
// 瑞利相位 * 瑞利散射系数 + Mie散射相位 * Mie散射系数
float3 Scattering(in AtomsphereParameter param, 
    float3 p, 
    float3 inDir, 
    float3 outDir)
{
    float cosTheta = dot(inDir, outDir);
    float h = length(p) - param.planeRadius;
    float3 rayleigh = RayleighCoefficient(h) * Rayleigh(cosTheta);
    float3 mie = MieCoefficient(h) * MiePhase(cosTheta);
    return rayleigh + mie;
}

// Mie 散射的吸收
float3 MieAbsorption(in AtomsphereParameter param, float h) {
    const float3 sigma = float3(4.4 * 1e-6);
    float H_M = param.MieScatteringScalarHeight;
    float rho_h = exp(-(h / H_M));
    return sigma * rho_h;
}

// 臭氧的吸收
float3 OzoneAbsorption(in AtomsphereParameter param, float h) {
    const float3 signma = float3(0.650, 1.881, 0.085) * 1e-6;
    float center = param.ozoneLevelCenterHeight;
    float width = param.ozoneLevelWidth;
    float rho = max(0, 1 - (abs(h - center) / width));
    return sigma * rho;
}

// 计算湮灭
// 湮灭 = 散射系数(瑞利系数 + Mie系数) + 吸收(臭氧吸收 + Mie散射吸收)
float3 GetExtinction(in AtomsphereParameter param, float h) {
    float3 scattering = RayleighCoeffcient(param, h) + MieCoefficient(param, h);
    float3 absorption = OzoneAbsorption(param, h) + MieAbsorption(param, h);
    return scattering + absorption;
}
