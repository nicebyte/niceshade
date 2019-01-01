#include "inc/triangle.hlsl"

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET {
  return ps_in.position * 0.5 + 0.5;
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  return Triangle(vid, 1.0);
}
//T: fullscreen_triangle ps:PSMain vs:VSMain