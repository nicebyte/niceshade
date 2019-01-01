#version 430

layout(location = 0) in vec2 ps_in_texcoord;
layout(location = 0) out vec4 _entryPointOutput;

void main()
{
    _entryPointOutput = (gl_FragCoord * 0.5) + vec4(0.5);
}

