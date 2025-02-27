#version 430

out gl_PerVertex
{
    vec4 gl_Position;
};

const vec4 _32[3] = vec4[](vec4(-1.0, -1.0, 0.0, 1.0), vec4(3.0, -1.0, 0.0, 1.0), vec4(-1.0, 3.0, 0.0, 1.0));
const vec2 _33[3] = vec2[](vec2(0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));

layout(location = 0) out vec2 out_var_ATTRIBUTE0;

void main()
{
    uint _38 = uint(gl_VertexID) % 3u;
    gl_Position = _32[_38] * 1.0;
    out_var_ATTRIBUTE0 = _33[_38];
}

/**NGF_NATIVE_BINDING_MAP
(0 1) : 0
(0 2) : 0
(-1 -1) : -1
**/
