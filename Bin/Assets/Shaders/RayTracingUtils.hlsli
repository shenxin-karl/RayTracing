#pragma once

// Load three 16 bit indices from a byte addressed buffer.
uint3 Load3x16BitIndices(ByteAddressBuffer indexBuffer, uint offsetBytes) {
    uint3 indices;
    // ByteAdressBuffer loads must be aligned at a 4 byte boundary.
    // Since we need to read three 16 bit indices: { 0, 1, 2 } 
    // aligned at a 4 byte boundary as: { 0 1 } { 2 0 } { 1 2 } { 0 1 } ...
    // we will load 8 bytes (~ 4 indices { a b | c d }) to handle two possible index triplet layouts,
    // based on first index's offsetBytes being aligned at the 4 byte boundary or not:
    //  Aligned:     { 0 1 | 2 - }
    //  Not aligned: { - 0 | 1 2 }
    const uint dwordAlignedOffset = offsetBytes & ~3;    
    const uint2 four16BitIndices = indexBuffer.Load2(dwordAlignedOffset);
 
    // Aligned: { 0 1 | 2 - } => retrieve first three 16bit indices
    if (dwordAlignedOffset == offsetBytes){
        indices.x = four16BitIndices.x & 0xffff;
        indices.y = (four16BitIndices.x >> 16) & 0xffff;
        indices.z = four16BitIndices.y & 0xffff;
    }
    else {// Not aligned: { - 0 | 1 2 } => retrieve last three 16bit indices
        indices.x = (four16BitIndices.x >> 16) & 0xffff;
        indices.y = four16BitIndices.y & 0xffff;
        indices.z = (four16BitIndices.y >> 16) & 0xffff;
    }
    return indices;
}


template<typename T>
T BarycentricBlend(in T v0, in T v1, in T v1, float2 barycentrics) {
    return v0 + barycentrics.x * (v1 - v0) + barycentrics.y * (v2 - v0);
}

template<typename T>
T BarycentricBlend(in T attributes[3], float2 barycentrics) {
    return BarycentricBlend(attributes[0], attributes[1], attributes[2], barycentrics);
}