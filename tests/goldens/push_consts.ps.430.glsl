#version 430

layout(binding = 1, std140) uniform type_ConstantBuffer_pc
{
    float scale;
} buf1;

layout(location = 0) out vec4 out_var_SV_TARGET;

void main()
{
    out_var_SV_TARGET = (gl_FragCoord * 0.5) + vec4(0.5 * buf1.scale);
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(0 2) : 1
(-1 -1) : -1
2
**/
