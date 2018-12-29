//T:fullscreen_triangle_crlf vs:VSMain ps:PSMain

#include "inc/triangle.hlsl"

// T: fullscreen_triangle_crlf_vertexonly vs:VSMain

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET {
	return ps_in.position;
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
	return Triangle(vid, 1.0);
}