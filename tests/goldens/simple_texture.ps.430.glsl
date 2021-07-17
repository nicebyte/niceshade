#version 430

layout(binding = 0) uniform sampler2D tex_samp;

layout(location = 0) in vec2 in_var_ATTRIBUTE0;
layout(location = 0) out vec4 out_var_SV_TARGET;

void main()
{
    out_var_SV_TARGET = texture(tex_samp, in_var_ATTRIBUTE0);
}

/**NGF_NATIVE_BINDING_MAP
(1 0) : 0
(2 0) : 0
(-1 -1) : -1
**/
