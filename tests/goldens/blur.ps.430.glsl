#version 430

#ifndef SPIRV_CROSS_CONSTANT_ID_0
#define SPIRV_CROSS_CONSTANT_ID_0 1u
#endif
const uint kernelRadius = SPIRV_CROSS_CONSTANT_ID_0;
const uint _45[7] = uint[](1u, 4u, 9u, 18u, 29u, 46u, 63u);

layout(binding = 0, std140) uniform type_BlurData
{
    vec4 samples[63];
} BlurData;

layout(binding = 0) uniform sampler2D tex_bilinearSamp;

layout(location = 0) in vec2 in_var_ATTR0;
layout(location = 0) out vec4 out_var_SV_TARGET;

void main()
{
    vec4 _49;
    _49 = vec4(0.0);
    for (uint _52 = 0u; _52 < _45[kernelRadius]; )
    {
        _49 += (texture(tex_bilinearSamp, in_var_ATTR0 + BlurData.samples[_52].xy) * BlurData.samples[_52].z);
        _52++;
        continue;
    }
    out_var_SV_TARGET = _49;
}

/**NGF_NATIVE_BINDING_MAP
(0 1) : 0
(0 2) : 0
(0 3) : 0
(-1 -1) : -1
**/