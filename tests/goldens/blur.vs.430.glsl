#version 430

out gl_PerVertex
{
    vec4 gl_Position;
};

const vec4 _33[3] = vec4[](vec4(-1.0, -1.0, 0.0, 1.0), vec4(3.0, -1.0, 0.0, 1.0), vec4(-1.0, 3.0, 0.0, 1.0));
const vec2 _53[3] = vec2[](vec2(0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));

layout(location = 0) out vec2 _entryPointOutput_texcoord;

void main()
{
    uint _121 = uint(gl_VertexID) % 3u;
    gl_Position = _33[_121] * 1.0;
    _entryPointOutput_texcoord = _53[_121];
}

