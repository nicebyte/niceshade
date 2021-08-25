#version 430

out gl_PerVertex
{
    vec4 gl_Position;
};

layout(binding = 0) uniform samplerBuffer texel_buf;

void main()
{
    gl_Position = vec4(texelFetch(texel_buf, 0).x);
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(-1 -1) : -1
**/
