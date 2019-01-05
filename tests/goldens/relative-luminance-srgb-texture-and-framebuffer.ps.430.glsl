#version 430

layout(binding = 0) uniform sampler2D img_SPIRV_Cross_DummySampler;

layout(location = 0) out vec4 _entryPointOutput;

void main()
{
    uvec2 _95 = uvec2(textureSize(img_SPIRV_Cross_DummySampler, 0));
    float _137 = dot(vec3(0.2125999927520751953125, 0.715200006961822509765625, 0.072200000286102294921875), texelFetch(img_SPIRV_Cross_DummySampler, ivec3(ivec2(gl_FragCoord.xy) % ivec2(int(_95.x), int(_95.y)), 0).xy, 0).xyz);
    _entryPointOutput = vec4(_137, _137, _137, 1.0);
}

