#version 430
layout(local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

layout(binding = 0, r32f) uniform writeonly imageBuffer outbuf;

void main()
{
    imageStore(outbuf, int(0u), vec4(1.0));
}

/**NGF_NATIVE_BINDING_MAP
(0 0) : 0
(-1 -1) : -1
**/
