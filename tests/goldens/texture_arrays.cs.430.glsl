#version 430
layout(local_size_x = 16, local_size_y = 16, local_size_z = 4) in;

layout(binding = 5, r32f) uniform writeonly image2D _output;
layout(binding = 0) uniform sampler2D inTexArr_samp[4];
layout(binding = 1) uniform sampler2D inTex_samp;

void main()
{
    vec2 _41 = vec2(gl_GlobalInvocationID.xy) * vec2(0.5);
    imageStore(_output, ivec2(gl_GlobalInvocationID.xy), vec4(textureLodOffset(inTexArr_samp[gl_GlobalInvocationID.z], _41, 0.0, ivec2(0)).x * textureLodOffset(inTex_samp, _41, 0.0, ivec2(0)).x));
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(0 1) : 0
(0 2) : 4
(0 3) : 5
(-1 -1) : -1
**/
