#version 430

const uint _38[7] = uint[](1u, 4u, 9u, 18u, 29u, 46u, 63u);
#ifndef SPIRV_CROSS_CONSTANT_ID_0
#define SPIRV_CROSS_CONSTANT_ID_0 1u
#endif
const uint kernelRadius = SPIRV_CROSS_CONSTANT_ID_0;

layout(binding = 0, std140) uniform BlurData
{
    vec4 samples[63];
} _49;

layout(binding = 0) uniform sampler2D tex_bilinearSamp;

layout(location = 0) in vec2 input_uv;
layout(location = 0) out vec4 _entryPointOutput;

void main()
{
    vec4 _146;
    _146 = vec4(0.0);
    vec4 _133;
    for (uint _145 = 0u; _145 < _38[kernelRadius]; _146 = _133, _145++)
    {
        _133 = _146 + (texture(tex_bilinearSamp, input_uv + _49.samples[_145].xy) * _49.samples[_145].z);
    }
    _entryPointOutput = _146;
}

