#version 430
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

layout(binding = 0, rg32f) uniform writeonly image2D dstTexture;

void main()
{
    float _50 = (float(gl_GlobalInvocationID.x) + 0.5) * 0.001953125;
    float _55 = 1.0 - ((float(gl_GlobalInvocationID.y) + 0.5) * 0.001953125);
    vec3 _59 = vec3(sqrt(1.0 - (_50 * _50)), 0.0, _50);
    float _61;
    float _64;
    _61 = 0.0;
    _64 = 0.0;
    float _62;
    float _65;
    for (uint _66 = 0u; _66 < 64u; _61 = _62, _64 = _65, _66++)
    {
        uint _74 = (_66 << 16u) | (_66 >> 16u);
        uint _79 = ((_74 & 1431655765u) << 1u) | ((_74 & 2863311530u) >> 1u);
        uint _84 = ((_79 & 858993459u) << 2u) | ((_79 & 3435973836u) >> 2u);
        uint _89 = ((_84 & 252645135u) << 4u) | ((_84 & 4042322160u) >> 4u);
        float _96 = float(((_89 & 16711935u) << 8u) | ((_89 & 4278255360u) >> 8u)) * 2.3283064365386962890625e-10;
        float _97 = _55 * _55;
        float _98 = float(_66) * 0.098174773156642913818359375;
        float _105 = sqrt((1.0 - _96) / (1.0 + (((_97 * _97) - 1.0) * _96)));
        float _108 = sqrt(1.0 - (_105 * _105));
        bvec3 _115 = bvec3(abs(1.0) < 0.999000012874603271484375);
        vec3 _118 = normalize(cross(vec3(_115.x ? vec3(0.0, 0.0, 1.0).x : vec3(1.0, 0.0, 0.0).x, _115.y ? vec3(0.0, 0.0, 1.0).y : vec3(1.0, 0.0, 0.0).y, _115.z ? vec3(0.0, 0.0, 1.0).z : vec3(1.0, 0.0, 0.0).z), vec3(0.0, 0.0, 1.0)));
        vec3 _125 = normalize(((_118 * (cos(_98) * _108)) + (cross(vec3(0.0, 0.0, 1.0), _118) * (sin(_98) * _108))) + (vec3(0.0, 0.0, 1.0) * _105));
        float _126 = dot(_59, _125);
        vec3 _130 = normalize((_125 * (2.0 * _126)) - _59);
        float _131 = _130.z;
        float _132 = isnan(0.0) ? _131 : (isnan(_131) ? 0.0 : max(_131, 0.0));
        float _133 = _125.z;
        float _135 = isnan(0.0) ? _126 : (isnan(_126) ? 0.0 : max(_126, 0.0));
        if (_132 > 0.0)
        {
            float _139 = isnan(0.0) ? _50 : (isnan(_50) ? 0.0 : max(_50, 0.0));
            float _140 = _97 * 0.5;
            float _141 = 1.0 - _140;
            float _151 = (((_132 / ((_132 * _141) + _140)) * (_139 / ((_139 * _141) + _140))) * _135) / ((isnan(0.0) ? _133 : (isnan(_133) ? 0.0 : max(_133, 0.0))) * _50);
            float _153 = pow(1.0 - _135, 5.0);
            _62 = _61 + (_153 * _151);
            _65 = _64 + ((1.0 - _153) * _151);
        }
        else
        {
            _62 = _61;
            _65 = _64;
        }
    }
    imageStore(dstTexture, ivec2(gl_GlobalInvocationID.xy), vec2(_64 * 0.015625, _61 * 0.015625).xyyy);
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(-1 -1) : -1
**/
