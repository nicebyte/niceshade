//T:texture_arrays cs:CSMain


[[vk::binding(0,0)]] Texture2D<float> inTexArr[4];
[[vk::binding(1,0)]] sampler samp;
[[vk::binding(2,0)]] Texture2D<float> inTex;
[[vk::binding(3,0)]] RWTexture2D<float> output;

[numthreads(16,16,4)]
void CSMain(uint3 dtid : SV_DispatchThreadID) {
  float x = inTexArr[dtid.z].SampleLevel(samp, dtid.xy / 2.0f, 0, 0).x;
  float y = inTex.SampleLevel(samp, dtid.xy / 2.0f, 0, 0).x;
  output[dtid.xy] = x * y;
}

