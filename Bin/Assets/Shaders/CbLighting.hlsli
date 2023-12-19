#ifndef __CB_LIGHTING_HISLI__
#define __CB_LIGHTING_HISLI__

struct AmbientLight {
	float3	color;
    float	intensity;
};

struct DirectionalLight {
    float3  color;
    float   intensity;
    float3  direction;
    float   padding0;
};

struct SpotLight {
    float3 strength;
    float falloffStart;
    float3 direction;
    float falloffEnd;
    float3 position;
    float spotPower;
};

struct PointLight {
    float3 color;
    float intensity;
    float3 position;
    float range;
};


struct CbLighting {
    AmbientLight     ambientLight;
    DirectionalLight directionalLight;
};

#endif