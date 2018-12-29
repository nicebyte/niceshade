//T: polygon ps:PSMain vs:VSMain
//T: polygon_animated ps:PSMain vs:VSMain define:ANIMATE=1

struct PolygonVertex {
  float2 position : SV_POSITION;
  float3 color : ATTR0;
};

PolygonVertex VSMain(PolygonVertex input) {
  PolygonVertex output = input;
  return output;
}

float4 PSMain(PolygonVertex input) : SV_TARGET {
  return float4(input.color, 1.0);
}
