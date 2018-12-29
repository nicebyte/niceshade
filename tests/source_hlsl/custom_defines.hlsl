// T: simple_texture_def1 vs:VSMain ps:PSMain define:TEXNAME=tex1 define:UVSCALE=1.0

#include "inc/triangle.hlsl"

[vk::binding(1, 0)] uniform Texture2D TEXNAME;
[vk::binding(2, 0)] uniform sampler samp;

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET { //T:simple_texture_def2 vs:VSMain define:TEXNAME=tex2 define:UVSCALE=2.0 ps:PSMain 
  return TEXNAME.Sample(samp, UVSCALE * ps_in.texcoord);
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  return Triangle(vid, 1.0);
}