#version 430

layout(location = 0) out vec4 out_var_SV_TARGET;

void main()
{
    out_var_SV_TARGET = gl_FragCoord;
}

/**NGF_NATIVE_BINDING_MAP
(-1 -1) : -1
**/
