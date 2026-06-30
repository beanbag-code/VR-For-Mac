#include <metal_stdlib>
using namespace metal;

struct VertexOut {
    float4 position [[position]];
    float2 uv;
};

struct StereoUniforms {
    float eyeSeparation;
    float2 cropCenter;
    float2 cropScale;
};

struct LensUniforms {
    float k1;
    float k2;
};

vertex VertexOut vertex_main(
    const device float* vertexData [[buffer(0)]],
    uint vid [[vertex_id]]
) {
    VertexOut out;
    uint base = vid * 4;
    out.position = float4(vertexData[base + 0], vertexData[base + 1], 0.0, 1.0);
    out.uv = float2(vertexData[base + 2], vertexData[base + 3]);
    return out;
}

fragment float4 fragment_main(
    VertexOut in [[stage_in]],
    texture2d<float> tex [[texture(0)]],
    sampler sam [[sampler(0)]],
    constant StereoUniforms& stereo [[buffer(0)]],
    constant LensUniforms& lens [[buffer(1)]]
) {
    float2 uv = in.uv;
    bool leftEye = uv.x < 0.5;

    float eyeU = leftEye ? uv.x * 2.0 : (uv.x - 0.5) * 2.0;
    float2 eyeUV = float2(eyeU, uv.y);

    float2 croppedUV = stereo.cropCenter + (eyeUV - 0.5) * stereo.cropScale;
    croppedUV.x += leftEye ? stereo.eyeSeparation : -stereo.eyeSeparation;

    float2 centeredUV = (croppedUV - 0.5) * 2.0;
    float r2 = dot(centeredUV, centeredUV);
    float r4 = r2 * r2;
    float factor = 1.0 + lens.k1 * r2 + lens.k2 * r4;
    float2 distortedUV = 0.5 + 0.5 * centeredUV * factor;
    distortedUV = clamp(distortedUV, 0.0, 1.0);

    return tex.sample(sam, distortedUV);
}
