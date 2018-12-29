#version 430

layout(binding = 0) uniform sampler2D tex_samp;

layout(location = 0) in vec2 ps_in_texcoord;
layout(location = 0) out vec4 _entryPointOutput;

void main()
{
    _entryPointOutput = texture(tex_samp, ps_in_texcoord);
}

