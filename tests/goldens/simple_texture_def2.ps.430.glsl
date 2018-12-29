#version 430

layout(binding = 0) uniform sampler2D tex2_samp;

layout(location = 0) in vec2 ps_in_texcoord;
layout(location = 0) out vec4 _entryPointOutput;

void main()
{
    _entryPointOutput = texture(tex2_samp, ps_in_texcoord * 2.0);
}

