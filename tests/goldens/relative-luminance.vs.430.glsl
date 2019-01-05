#version 430

out gl_PerVertex
{
    vec4 gl_Position;
};

const vec2 _22[3] = vec2[](vec2(-1.0), vec2(3.0, -1.0), vec2(-1.0, 3.0));

void main()
{
    gl_Position = vec4(_22[uint(gl_VertexID) % 3u], 0.0, 1.0);
}

