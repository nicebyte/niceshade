#include "inc/triangle.hlsl"

//T: blur ps:PSMain vs:VSMain

struct VS_Output
{
    float4 pos : SV_POSITION;
    float2 uv : ATTR0;
};

 cbuffer ScreenParams : register( b0 )
{
    float2 pixelSize;
    float u_phase;
    uint flip;
};

Texture2D< float4 > tex : register( t2 );
sampler bilinearSamp : register( s3 );

[[vk::constant_id( 0 )]] const uint kernelRadius = 1;

static const uint maxKernelRadius = 6;
static const uint kernelSizes[] = { 1, 4, 9, 18, 29, 46, 63 };
static const uint maxKernelSize = kernelSizes[ maxKernelRadius ];

cbuffer BlurData : register( b1 )
{
    float4 samples[ 63 ];
};

Triangle_PSInput VSMain(uint vid : SV_VertexID) {
  return Triangle(vid, 1.0);
}

float4 PSMain( VS_Output input ) : SV_TARGET
{
    float4 c = 0;

     for( uint i = 0; i < kernelSizes[ kernelRadius ]; i++ )
    {
        c += samples[ i ].z * tex.Sample( bilinearSamp, input.uv + samples[ i ].xy );
    }

     return c;
}



