#include "inc/triangle.hlsl"

//T: incomplete_entry_point_name vs:VS.Main ps:PSMain

float4 PSMain(Triangle_PSInput ps_in) : SV_TARGET {
	return ps_in.position;
}

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
	return Triangle(vid, 1.0);
}
