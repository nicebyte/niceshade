# niceshade

A wrapper for the Microsoft DirectXShaderCompiler and SPIRV-Cross

[![Build status](https://ci.appveyor.com/api/projects/status/ny9j8k6869artsrd?svg=true)](https://ci.appveyor.com/project/nicebyte/niceshade)

See the reference documentation for `libniceshade` at https://wiki.gpfault.net/docs/niceshade/

# User Manual

## Table of Contents

* [Introduction](#intro)
* [Obtaining the Source Code and Building](#building)
* [Running](#running)
* [Defining Techniques](#techniques)
* [Generated Header File](#header-file)
* [Pipeline Metadata](#pipeline-metadata)
* [Using Vulkan Features From HLSL](#vk-hlsl)
* [Pipeline Metadata File Format](#metadata-format)

<a name="intro"></a>
## Introduction

**niceshade** is a library and a command-line tool that transforms HLSL code into shaders for various graphics APIs. Presently, the following APIs can be targeted:

* OpenGL 4.3+
* OpenGL ES 3.1+
* Metal 1.0+
* Vulkan 1.0+

The input HLSL files may contain definitions of several entry points for different shader stages. The entry points can be configured into a single rendering pipeline (with additional options, if desired) using a special directive. For each of these configurations (called *techniques*), the tool will generate platform-specific shaders.

As an example, here is a shader that calculates the relative luminance of each pixel in an image. For demonstration purposes, it allows to optionally apply gamma correction to input and/or output.

```cpp
// The comments below are recognized by niceshade as technique definitions.
//T: relative-luminance vs:VSMain ps:PSMain define:OUTPUT_NEEDS_GAMMA_CORRECTION=1 define:INPUT_NEEDS_GAMMA_CORRECTION=1
//T: relative-luminance-srgb-texture vs:VSMain ps:PSMain define:OUTPUT_NEEDS_GAMMA_CORRECTION=1
//T: relative-luminance-srgb-framebuffer vs:VSMain ps:PSMain define:INPUT_NEEDS_GAMMA_CORRECTION=1
//T: relative-luminance-srgb-texture-and-framebuffer vs:VSMain ps:PSMain

float4 VSMain(uint vid : SV_VertexID) : SV_POSITION { // vertex shader
  const float2 fullscreen_triangle_verts[] = {
    float2(-1.0, -1.0), float2(3.0, -1.0), float2(-1.0,  3.0)
  };
  return  float4(fullscreen_triangle_verts[vid % 3], 0.0, 1.0);
}

uniform Texture2D img;

float4 PSMain(float4 frag_coord : SV_POSITION) : SV_TARGET { // pixel shader
  const float gamma = 2.2;
  uint img_width, img_height;
  img.GetDimensions(img_width, img_height);
  float3 color = img.Load(int3(int2(frag_coord.xy) % int2(img_width, img_height), 0)).rgb;
#if defined(INPUT_NEEDS_GAMMA_CORRECTION)
  color = pow(color, gamma);
#endif
  float relative_luminance = dot(float3(0.2126, 0.7152, 0.0722), color);
#if defined(OUTPUT_NEEDS_GAMMA_CORRECTION)
  relative_luminance = pow(relative_luminance, 1.0 / gamma);
#endif
  return float4(float3(relative_luminance, relative_luminance, relative_luminance), 1.0);
}
```

In addition to generating shaders, the tool captures and writes out the information about resources (textures, buffers, etc.) used by each technique defined in the input file. This information can be used by the application for various purposes, such as streamlining Vulkan pipeline layout creation.

This tool is powered by [Microsoft DirectXShaderCompiler](https://github.com/microsoft/DirectXShaderCompiler) and [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross).

<a name="building"></a>
## Obtaining the Source Code and Building 

You will need to have `git` and `cmake` installed on your system.  On Windows, building with compilers other than MSVC may not work.

Execute the following command to clone the project's repository:

`git clone https://github.com/nicebyte/niceshade.git`

Once the cloning process is complete, execute the following commands from the root of the repository:

```
mkdir build
cd build
cmake .. -Ax64
```
This will generate project files specific to your system in the `build` folder. After building the generated project, the `niceshade` binary can be found in the repository's root folder.

<a name="running"></a>
## Running

To transform an input HLSL file to platform-specific shaders, execute:

`niceshade <input file name> <options>`

Valid command line options are:

 * `-O <path>` - specifies the folder to store the output files in. By default, the output files are written to the current working directory.
 * `-t <target>` - specifies a target to generate shaders for.  Accepted values are:
      * `gl430` for OpenGL;
      * `gles310`, `gles320` for OpenGL ES;
      * `msl10`, `msl11`, `msl12`, `msl20` for Metal on macOS;
      * `msl10ios`, `msl11ios`, `msl12ios`, `msl20ios` for Metal on iOS;
      * `spv` for SPIR-V.
 * `-o <level>` - Set SPIR-V optimization level. `1` will apply the same optimizations as
   `spirv-opt -O`. `0` will turn off all optimizations. The default value is `0`. SPIR-V
   optimizations have an effect on the output for non-SPIR-V targets. Enabling them will
   result in less readable output that doesn't match the input HLSL as closely. Using
   code generated from optimized SPIR-V may result in performance wins on some platforms,
   but, as always, measure.
 * `-m <version>` - HLSL shader model version to use. Valid values are: 6_0, 6_1, 6_2, 6_3, 6_4, 6_5, 6_6. Default is 6_0.
 * `-h <path>` - Path (relative to the output folder) for the generated
      header file with descriptor binding and set numbers. If not specified, no
      header file will be generated.
 * `-n <identifier>` - Namespace for the generated shader file. If not specified,
     the global namespace is used.
 * `-D <name>=<value>` - Add a preprocessor definition `name` with the value `value` to
     techniques.

Shaders will be generated for each of the techniques specified in the input file and each of the targets specified in the command line options.

For example, the following line will produce OpenGL 4.3 and Metal 1.2 shaders for each technique defined in `input.hlsl`, in the `generated_shaders/` subfolder:

`niceshade input.hlsl -O generated_shaders/ -t gl430 -t msl12`

<a name="techniques"></a>
## Defining Techniques

Techniques are defined using a special comment:

`//T: <technique name> <tags>`

The technique name may include alphanumeric characters, underscores (`_`) and dashes (`-`).

A tag is a name-value pair separated by a colon. For example, `vs:VSMain` is a tag, `vs` is the tag name and `VSMain` is the value.

The following tag names are valid:
* `vs` - the tag value specifies the entry point for the vertex shader stage;
* `ps` - the tag value specifies the entry point for the pixel shader stage;
* `cs` - the tag value specifies the entry point for a compute shader;
* `define` - the tag value specifies an additional preprocessor definition;
* `meta` - the tag value specifies an additional metadata entry. It should be a name-value pair separated by a `=` sign, i.e.: `meta:enable_depth_testing=1`. These values get stored as part of the pipeline metadata file (see below) and users are free to interpret them as they wish. 

<a name="header-file"></a>
## Generated Header File

Using the `-h` command line option, you may specify a special C++ header file to be generated as part of the shader compilation process. The said file shall contain named constants for all descriptor bindings and sets used by different techniques defined in the input. The constants for each technique are put into their own namespace, named after the technique (hyphens in tenchnique names are replaced by underscores to get valid C++ identifiers). Additionally, you may put the entire contents of the generated header into another namespace,
specified by the `-n` command line option.

Below is an example of an input file and the generated header it produces.

Input file:
```cpp
//T: imgui ps:PSMain vs:VSMain

struct ImGuiVSInput {
  float2 position : ATTR0;
  float2 uv : TEXCOORD0;
  float4 color : COLOR0;
};

struct ImGuiPSInput {
  float4 position : SV_POSITION;
  float2 uv : TEXCOORD0;
  float4 color : COLOR0;
};

cbuffer MatUniformBuffer : register(b0){
  float4x4 u_Projection;
}

[[vk::binding(1, 0)]] uniform Texture2D u_Texture;
[[vk::binding(2, 0)]] uniform sampler u_Sampler;

ImGuiPSInput VSMain(ImGuiVSInput input) {
  ImGuiPSInput output = {
    mul(u_Projection, float4(input.position, 0.0, 1.0)),
    input.uv,
    input.color
  };
  return output;
}

float4 PSMain(ImGuiPSInput input) : SV_TARGET {
  return input.color * u_Texture.Sample(u_Sampler, input.uv);
}
```

Generated header:
```cpp
/*auto-generated, do not edit*/
#pragma once
namespace shader_consts {
namespace imgui {
  static constexpr int u_Sampler_Binding = 2;
  static constexpr int u_Sampler_Set = 0;
  static constexpr int u_Texture_Binding = 1;
  static constexpr int u_Texture_Set = 0;
  static constexpr int MatUniformBuffer_Binding = 0;
  static constexpr int MatUniformBuffer_Set = 0;
}
}
```

<a name="pipeline-metadata"></a>
## Pipeline Metadata

For each technique defined in the input file, **niceshade** will produce a corresponding `.pipeline` file, which contains the following information:

* List of all resources consumed by the technique, including their types, bindings and which pipeline stages they are used by;
* A mapping from separate image and sampler bindings to auto-generated combined image/sampler bindings (relevant for targets which don't have full separation between textures and samplers at the shader level, i.e. OpenGL);
* Any additional metadata provided by the user in the technique specification using the `meta:` tag.

`.pipeline` files are binary. Code for parsing the binary format is provided in the `metadata_parser` subfolder of the source code repository. Alternatively, `.pipeline` files can be converted to human-readable JSON using the `display_metadata` utility, the source code for which is provided in the `samples` subfolder of the repository. A detailed description of the metadata file format is provided [below](#metadata-format).

<a name="vk-hlsl"></a>
## Using Vulkan features from HLSL

You may choose to use the Vulkan binding model explicitly and assign descriptor sets and bindings like this:

```
[[vk::binding(1, 0)]] uniform Texture2D tex; // assign tex to set 0 binding 1
[[vk::binding(2, 0)]] uniform sampler samp;  // assign tex to set 0 binding 2
```

You may use specialization constants as well:

```
[[vk::constant_id(1)]] const float specConstFloat = 1.5;
```

See [here](https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst) for more details.

<a name="metadata-format"></a>
## Pipeline Metadata File Format

For each technique described in the input file, `niceshade` emits a file containing information that can be leveraged to simplify pipeline creation. This data includes:

* Description of the pipeline layout;
* Mapping from separate image and sampler bindings to their corresponding auto-generated combined image/sampler bindings (for platforms that don't have full separation between textures and samplers, i.e. OpenGL);
* Any additional metadata specified by the user using the `meta:` tag in the technique description.

A detailed description of the file's format follows.

### General Conventions

A pipeline metadata file (referred to simply as "file" henceforth) is broken into **records**. A **record** is a sequence of **fields** and **raw byte blocks**. A **field** is a 4-byte unsigned integer in network byte order. A **raw byte block** consists of a header and a body. The **body of a raw byte block** is an arbitrary sequence of bytes. The length of the body is always a multiple of 4 bytes. The **header** of a raw byte block is two 32-bit unsigned integers in network byte order. The first is always equal to `0xffffffff` and signifies the beginning of a raw byte block. The second specifies the length of the block's body divided by 4.

A record's type is defined by its layout. The following types of records are defined:

* `HEADER`;
* `ENTRYPOINTS`;
* `PIPELINE_LAYOUT`;
* `SEPARATE_TO_COMBINED_MAP`;
* `USER_METADATA`.

A detailed description of each record type follows.

### The `HEADER` Record Type

The very first record in a file is always of the `HEADER` type. There is always only one instance of a `HEADER` record in a file.

A `HEADER` record contains the following fields, in this exact order:

* `magic_number` - valid files must always have this field set to `0xdeadbeef`;
* `header_size` - total size of the header record in bytes (including the `header_size` and `magic_number` fields);
* `version_maj` - major version number of the metadata format in use;
* `version_min` - minor version number of the metadata format in use;
* `entrypoints_offset` - offset, in bytes, from the beginning of the file, at which the `ENTRYPOINTS` record is stored;
* `pipeline_layout_offset` - offset, in bytes, from the beginning of the file, at which the `PIPELINE_LAYOUT` record is stored;
* `image_to_cis_map_offset` - offset, in bytes, from the beginning of the file, at which a `SEPARATE_TO_COMBINED_MAP` record is stored, which maps separate *image* bindings to the corresponding auto-generated combined image/sampler bindings;
* `sampler_to_cis_map_offset` - offset, in bytes, from the beginning of the file, at which a `SEPARATE_TO_COMBINED_MAP` record is stored, which maps separate *sampler* bindings to the corresponding auto-generated combined image/sampler bindings;
* `user_metadata_offset` - offset, in bytes, from the beginning of the file, at which the `USER_METADATA` record is stored;

### The `ENTRYPOINTS` Record Type

This record type provides the names of entry points for individual shader stages involved in the technique.

The first field in this record, `num_entrypoints`, contains the number of entrypoints, one per shader stage. It is followed by a sequence of `num_entrypoints` entrypoint descriptions.

An entrypoint description consists of a field, `type` which indicates the type of the shader stage the entrypoint is for (`0` for vertex shader, `1` for fragment shader), and a raw byte block containing a null-terminated entrypoint name.

### The `PIPELINE_LAYOUT` Record Type

This record type provides information about descriptor sets, descriptor types and bindings used by the pipeline. There is always only one instance of a `PIPELINE_LAYOUT` record in a file.

A `PIPELINE_LAYOUT` record contains the following fields, in this exact order:

* `num_descriptor_sets` - total number of descriptor sets in the layout;
* For each of the `num_descriptor_sets` descriptor sets:
  * `num_descriptors` - number of descriptors in the set;
  * For each of the `num_descriptors` descriptors:
    * `binding_id` - bindning number of the descriptor;
    * `descriptor_type` - an integer indicating the type of the descriptor. The following values are valid:
      * `0x00` - indicates a uniform buffer;
	  * `0x01` - indicates a storage buffer;
	  * `0x02` - indicates a load/store image;
	  * `0x03` - indicates a texture;
	  * `0x04` - indicates a sampler;
	  * `0x05` - indicates a combined texture/sampler.
    * `0x06` - indicates a uniform texel buffer.

### The `SEPARATE_TO_COMBINED_MAP` Record Type

Some platforms do not have full separation between textures and samplers. For example, in OpenGL, in order to sample a texture with a particular sampler, both the texture and the sampler need to be bound to the same texture unit by the CPU-side code. This is in contrast to HLSL, which allows specifying the sampler to use directly from the shader code.

To address this discrepancy, each unique texture-sampler pair used by the source HLSL generates a "synthetic" combined texture/sampler in the output. Each separate texture and sampler is then mapped to a set of auto-generated combined texture/samplers that it is used in.

A `SEPARATE_TO_COMBINED_MAP` record contains the following fields, in this exact order:

* `num_entries` - number of entries in the record;
* For each entry:
  * `set_id` - descriptor set id of the separate image or sampler;
  * `binding_id` - binding id of the separate image or sampler;
  * `num_combineds` - number of combined texture/samplers that the separate image or sampler is being used by;
  * For each of the `num_combineds` combined texture/samplers:
    * `combined_binding_id` - binding id of the auto-generated combined image/sampler;

### The `USER_METADATA` Record Type

This record stores any additional user metadata specified by `meta:` tags in the technique description.

The `USER_METADATA` record has only one field, `num_metas`. Following the field are `num_metas` pairs of raw byte blocks. The first block in a pair stores the user-provided key, and the second stores the user-provided value (both are null-terminated strings).

### The `THREADGROUP_SIZE` Record Type

This record has meaning only for compute shaders. It stores the threadgroup size declared by the shader. For other shader types, this record is present, but contains zeros.
The record has 3 fields, each corresponding to the threadgroup size in X, Y and Z dimensions accordingly.

____
