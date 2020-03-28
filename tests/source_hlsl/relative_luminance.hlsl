//T: relative-luminance vs:VSMain ps:PSMain define:OUTPUT_NEEDS_GAMMA_CORRECTION=1 define:INPUT_NEEDS_GAMMA_CORRECTION=1
//T: relative-luminance-srgb-texture vs:VSMain ps:PSMain define:OUTPUT_NEEDS_GAMMA_CORRECTION=1
//T: relative-luminance-srgb-framebuffer vs:VSMain ps:PSMain define:INPUT_NEEDS_GAMMA_CORRECTION=1
//T: relative-luminance-srgb-texture-and-framebuffer vs:VSMain ps:PSMain

float4 VSMain(uint vid : SV_VertexID) : SV_POSITION{
  const float2 fullscreen_triangle_verts[] = {
    float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0,  3.0)
  };
  return  float4(fullscreen_triangle_verts[vid % 3], 0.0, 1.0);
}

uniform Texture2D img;

float4 PSMain(float4 frag_coord : SV_POSITION) : SV_TARGET{
  const float gamma = 2.2;
  uint img_width, img_height;
  img.GetDimensions(img_width, img_height);
  float3 color = img.Load(int3(int2(frag_coord.xy) % int2(img_width, img_height), 0)).rgb;
#if defined(INPUT_NEEDS_GAMMA_CORRECTION)
  color = pow(color, gamma);
#endif
  float relative_luminance = dot(float3(0.2126, 0.7152, 0.0722), color);
#if defined(OUTPUT_NEEDS_GAMMA_CORRECTION)
  relative_luminance = pow(relative_luminance, 1.0 / gamma);
#endif
  return float4(float3(relative_luminance, relative_luminance, relative_luminance), 1.0);
}