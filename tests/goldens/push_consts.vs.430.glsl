#version 430

out gl_PerVertex
{
    vec4 gl_Position;
};

const vec4 _42[3] = vec4[](vec4(-1.0, -1.0, 0.0, 1.0), vec4(3.0, -1.0, 0.0, 1.0), vec4(-1.0, 3.0, 0.0, 1.0));
const vec2 _43[3] = vec2[](vec2(0.0), vec2(2.0, 0.0), vec2(0.0, 2.0));

layout(binding = 0, std140) uniform type_ConstantBuffer_pc
{
    float scale;
} buf0;

struct type_PushConstant_pc
{
    float scale;
};

uniform type_PushConstant_pc pcbuf;

layout(location = 0) out vec2 out_var_ATTRIBUTE0;

void main()
{
    uint _48 = uint(gl_VertexID) % 3u;
    vec4 _51 = _42[_48] * 1.0;
    vec2 _60 = _51.xy * (pcbuf.scale * buf0.scale);
    gl_Position = vec4(_60.x, _60.y, _51.z, _51.w);
    out_var_ATTRIBUTE0 = _43[_48];
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(0 2) : 1
(-1 -1) : -1
2
**/
