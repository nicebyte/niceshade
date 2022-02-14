#version 430

layout(binding = 0) uniform sampler2D img_SPIRV_Cross_DummySampler;

layout(location = 0) out vec4 out_var_SV_TARGET;

void main()
{
    uvec2 _31 = uvec2(textureSize(img_SPIRV_Cross_DummySampler, 0));
    ivec2 _35 = ivec2(gl_FragCoord.xy);
    ivec2 _38 = ivec2(int(_31.x), int(_31.y));
    float _47 = pow(dot(vec3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875), texelFetch(img_SPIRV_Cross_DummySampler, ivec3(_35 - _38 * (_35 / _38), 0).xy, 0).xyz), 0.454545438289642333984375);
    out_var_SV_TARGET = vec4(_47, _47, _47, 1.0);
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(-1 -1) : -1
**/