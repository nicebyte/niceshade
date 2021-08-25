// T: texel_buf vs:VSMain

[[vk::binding(0, 0)]] uniform Buffer<float> texel_buf;


float4 VSMain() : SV_POSITION {
  return texel_buf.Load(0);
}