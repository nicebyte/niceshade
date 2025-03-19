//T: unused_bindings cs:CSMain

struct buf{
  float4 foo;
};

[[vk::binding(0,0)]]
StructuredBuffer<buf> buf0;

[[vk::binding(1,0)]]
ConstantBuffer<buf> buf1;

[[vk::binding(0,1)]]
Texture2D<float4> tex0;

[[vk::binding(1,1)]]
Texture2DArray<float> tex1;

[numthreads(16, 16, 1)]
void CSMain( uint3 DTid : SV_DispatchThreadID ) {

}
