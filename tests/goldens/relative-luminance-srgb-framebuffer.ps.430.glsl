#version 430

layout(binding = 0) uniform sampler2D img_SPIRV_Cross_DummySampler;

layout(location = 0) out vec4 out_var_SV_TARGET;

void main()
{
    uvec2 _32 = uvec2(textureSize(img_SPIRV_Cross_DummySampler, 0));
    ivec2 _36 = ivec2(gl_FragCoord.xy);
    ivec2 _39 = ivec2(int(_32.x), int(_32.y));
    float _48 = dot(vec3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875), pow(texelFetch(img_SPIRV_Cross_DummySampler, ivec3(_36 - _39 * (_36 / _39), 0).xy, 0).xyz, vec3(2.2000000476837158203125)));
    out_var_SV_TARGET = vec4(_48, _48, _48, 1.0);
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(-1 -1) : -1
**/