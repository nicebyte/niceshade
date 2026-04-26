#version 430
layout(local_size_x = 16, local_size_y = 16, local_size_z = 1) in;

struct buf
{
    vec4 foo;
};

layout(binding = 0, std430) readonly buffer type_StructuredBuffer_buf
{
    buf _m0[];
} buf0;

layout(binding = 1, std140) uniform type_ConstantBuffer_buf
{
    vec4 foo;
} buf1;

void main()
{
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(0 1) : 1
(1 0) : 0
(1 1) : 1
(-1 -1) : -1
-1
**/
