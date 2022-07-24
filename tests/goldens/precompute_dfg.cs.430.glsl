#version 430
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, rg32f) uniform writeonly image2D dstTexture;

void main()
{
    float _52 = float(gl_GlobalInvocationID.x) + 0.5;
    float _53 = _52 * 0.001953125;
    float _57 = fma((-0.5) - float(gl_GlobalInvocationID.y), 0.001953125, 1.0);
    vec3 _61 = vec3(sqrt(fma(_52 * (-0.001953125), _53, 1.0)), 0.0, _53);
    float _63;
    float _66;
    _63 = 0.0;
    _66 = 0.0;
    float _64;
    float _67;
    for (uint _68 = 0u; _68 < 64u; _63 = _64, _66 = _67, _68++)
    {
        uint _76 = (_68 << 16u) | (_68 >> 16u);
        uint _81 = ((_76 & 1431655765u) << 1u) | ((_76 & 2863311530u) >> 1u);
        uint _86 = ((_81 & 858993459u) << 2u) | ((_81 & 3435973836u) >> 2u);
        uint _91 = ((_86 & 252645135u) << 4u) | ((_86 & 4042322160u) >> 4u);
        float _97 = float(((_91 & 16711935u) << 8u) | ((_91 & 4278255360u) >> 8u));
        float _99 = _57 * _57;
        float _100 = float(_68) * 0.098174773156642913818359375;
        float _106 = sqrt(fma(-_97, 2.3283064365386962890625e-10, 1.0) / fma(fma(_99, _99, -1.0), _97 * 2.3283064365386962890625e-10, 1.0));
        float _109 = sqrt(fma(-_106, _106, 1.0));
        bvec3 _116 = bvec3(abs(1.0) < 0.999000012874603271484375);
        vec3 _119 = normalize(cross(vec3(_116.x ? vec3(0.0, 0.0, 1.0).x : vec3(1.0, 0.0, 0.0).x, _116.y ? vec3(0.0, 0.0, 1.0).y : vec3(1.0, 0.0, 0.0).y, _116.z ? vec3(0.0, 0.0, 1.0).z : vec3(1.0, 0.0, 0.0).z), vec3(0.0, 0.0, 1.0)));
        vec3 _126 = normalize(((_119 * (cos(_100) * _109)) + (cross(vec3(0.0, 0.0, 1.0), _119) * (sin(_100) * _109))) + (vec3(0.0, 0.0, 1.0) * _106));
        float _127 = dot(_61, _126);
        vec3 _131 = normalize((_126 * (2.0 * _127)) - _61);
        float _132 = _131.z;
        float _133 = isnan(0.0) ? _132 : (isnan(_132) ? 0.0 : max(_132, 0.0));
        float _134 = _126.z;
        float _136 = isnan(0.0) ? _127 : (isnan(_127) ? 0.0 : max(_127, 0.0));
        if (_133 > 0.0)
        {
            float _140 = isnan(0.0) ? _53 : (isnan(_53) ? 0.0 : max(_53, 0.0));
            float _141 = _99 * 0.5;
            float _143 = fma(-_99, 0.5, 1.0);
            float _151 = (((_133 / fma(_133, _143, _141)) * (_140 / fma(_140, _143, _141))) * _136) / ((isnan(0.0) ? _134 : (isnan(_134) ? 0.0 : max(_134, 0.0))) * _53);
            float _153 = pow(1.0 - _136, 5.0);
            _64 = fma(_153, _151, _63);
            _67 = fma(1.0 - _153, _151, _66);
        }
        else
        {
            _64 = _63;
            _67 = _66;
        }
    }
    imageStore(dstTexture, ivec2(gl_GlobalInvocationID.xy), vec2(_66 * 0.015625, _63 * 0.015625).xyyy);
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(-1 -1) : -1
**/