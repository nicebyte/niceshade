// T: simple_texture vs:VSMain ps:PSMain

#include "inc/triangle.hlsl"

[[vk::binding(0, 0)]] uniform Texture2D tex;
[[vk::binding(0, 0)]] uniform sampler samp;

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET {
  return tex.Sample(samp, ps_in.texcoord);
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  return Triangle(vid, 1.0);
}
