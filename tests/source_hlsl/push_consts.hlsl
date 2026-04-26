#include "inc/triangle.hlsl"

struct pc {
  float scale;
};

[[vk::binding(0)]] ConstantBuffer<pc> buf0;
[[vk::binding(2)]] ConstantBuffer<pc> buf1;

[[vk::push_constant]]
pc pcbuf;

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET {
  return ps_in.position * 0.5 + 0.5 * buf1.scale;
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  Triangle_PSInput r = Triangle(vid, 1.0);
  r.position.xy *= pcbuf.scale * buf0.scale;
  return r;

}
//T: push_consts ps:PSMain vs:VSMain
