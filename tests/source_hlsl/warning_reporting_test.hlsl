//T: warning_reporting_test cs:CSMain

RWBuffer<float> outbuf : register(u0);

[numthreads(1, 1, 1)]
void CSMain() 
{
  float uv = float3(1.0, 1.0, 1.0);
  outbuf[0] = uv;
}
