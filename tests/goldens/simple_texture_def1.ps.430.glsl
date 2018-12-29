#version 430

layout(binding = 0) uniform sampler2D tex1_samp;

layout(location = 0) in vec2 ps_in_texcoord;
layout(location = 0) out vec4 _entryPointOutput;

void main()
{
    _entryPointOutput = texture(tex1_samp, ps_in_texcoord * 1.0);
}

