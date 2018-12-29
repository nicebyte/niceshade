struct Triangle_PSInput {
  float4 position : SV_POSITION;
  float2 texcoord : ATTRIBUTE0;
};

Triangle_PSInput Triangle(uint vid, float scale) {
  float4 pos[] = {
    float4(-1.0, -1.0, 0.0, 1.0),
    float4( 3.0, -1.0, 0.0, 1.0),
    float4(-1.0,  3.0, 0.0, 1.0)
  };
  const float2 texcoords[] = {
    float2(0.0, 0.0), float2(2.0, 0.0), float2(0.0, 2.0)
  };
  Triangle_PSInput ps_in;
  vid = vid % 3;
  ps_in.position = pos[vid] * scale;
  ps_in.texcoord = texcoords[vid];
  return ps_in;
}