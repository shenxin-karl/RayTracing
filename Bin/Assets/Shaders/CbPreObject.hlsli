#ifndef __OBJECT_CB_HLSLI__
#define __OBJECT_CB_HLSLI__

struct CBPreObject {
    float4x4 matWorld;
    float4x4 matInvWorld;
    float4x4 matNormal;
    float4x4 matWorldPrev;     
};

#endif