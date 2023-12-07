#ifndef __OBJECT_CB_HLSLI__
#define __OBJECT_CB_HLSLI__

struct CBPreObject {
    float4x4 gMatWorld;
    float4x4 gMatInvWorld;
    float4x4 gMatNormal;
    float4x4 gMatInvNormal;
};

#endif