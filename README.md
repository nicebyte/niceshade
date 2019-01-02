<img src="https://github.com/nicebyte/nicegraf-shaderc/blob/master/nicegraf-shaderc.png?raw=true"/>

[![Build status](https://ci.appveyor.com/api/projects/status/ny9j8k6869artsrd?svg=true)](https://ci.appveyor.com/project/nicebyte/nicegraf-shaderc)

# User Manual

**nicegraf-shaderc** is a command-line tool that transforms HLSL code into shaders for various graphics APIs. Presently, the following APIs can be targeted:

* OpenGL 4.3
* OpenGL ES 3.1+
* Metal 1.0+
* SPIR-V

The input HLSL files may contain definitions of several entry points for different shader stages. The entry points can be configured into a single rendering pipeline (with additional options, if desired) using a special directive. For each of these configurations (called *techniques*), the tool will generate platform-specific shaders.

In addition to generating shaders, the tool captures and writes out the information about resources (textures, buffers, etc.) used by each technique defined in the input file. This information can be used by the application for various purposes, such as streamlining Vulkan pipeline layout creation.

This tool is powered by [shaderc](https://github.com/google/shaderc) and [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross).

## Table of Contents

* [Project Status](#project-status)
* [Obtaining the Source Code and Building](#building)
* [Running](#running)
* [Defining Techniques](#techniques)
* [Pipeline Metadata](#pipeline-metadata)
* [Using Vulkan Features From HLSL](#vk-hlsl)
* [Pipeline Metadata File Format](#metadata-format)

<a name="project-status"></a>
## Project Status 

This project is under active development. Some features may change significantly.

<a name="building"></a>
## Obtaining the Source Code and Building 

You will need to have `git` and `cmake` installed on your system.  On Windows, building with compilers other than MSVC may not work.

Execute the following command to clone the project's repository:

`git clone https://github.com/nicebyte/nicegraf-shaderc.git`

Once the cloning process is complete, execute the following commands from the root of the repository:

```
mkdir build
cd build
cmake .. -Ax64
```
This will generate project files specific to your system in the `build` folder. After building the generated project, the `nicegraf_shaderc` binary can be found in the repository's root folder.

<a name="running"></a>
## Running

To transform an input HLSL file to platform-specific shaders, execute:

`nicegraf_shaderc <input file name> <options>`

Valid command line options are:

 * `-O <path>` - specifies the folder to store the output files in. By default, the output files are written to the current working directory.
 * `-t <target>` - specifies a target to generate shaders for.  Accepted values are:
      * `gl430` for OpenGL;
      * `gles310`, `gles320` for OpenGL ES;
      * `msl10`, `msl11`, `msl12`, `msl20` for Metal on macOS;
      * `msl10ios`, `msl11ios`, `msl12ios`, `msl20ios` for Metal on iOS;
      * `spv` for SPIR-V.

Shaders will be generated for each of the techniques specified in the input file and each of the targets specified in the command line options.

For example, the following line will produce OpenGL 4.3 and Metal 1.2 shaders for each technique defined in `input.hlsl`, in the `generated_shaders/` subfolder:

`nicegraf_shaderc input.hlsl -O generated_shaders/ -t gl430 -t msl12`

<a name="techniques"></a>
## Defining Techniques

Techniques are defined using a special comment:

`//T: <technique name> <tags>`

The technique name may include alphanumeric characters, underscores (`_`) and dashes (`-`).

A tag is a name-value pair separated by a colon. For example, `vs:VSMain` is a tag, `vs` is the tag name and `VSMain` is the value.

The following tag names are valid:
* `vs` - the tag value specifies the entry point for the vertex shader stage;
* `ps` - the tag value specifies the entry point for the pixel shader stage;
* `define` - the tag value specifies an additional preprocessor definition;
* `meta` - the tag value specifies an additional metadata entry.

A valid technique definition must at least specify an entry point for the vertex stage.

<a name="pipeline-metadata"></a>
## Pipeline Metadata

For each technique defined in the input file, **nicegraf_shaderc** will produce a corresponding `.pipeline` file, which contains the following information:

* List of all resources consumed by the technique, including their types, bindings and which pipeline stages they are used by;
* A mapping from separate image and sampler bindings to auto-generated combined image/sampler bindings (relevant for targets which don't have full separation between textures and samplers at the shader level, i.e. OpenGL);
* Any additional metadata provided by the user in the technique specification using the `meta:` tag.

`.pipeline` files are binary. Code for parsing the binary format is provided in the `metadata_parser` subfolder of the source code repository. Alternatively, `.pipeline` files can be converted to human-readable JSON using the `display_metadata` utility, source code for which is provided in the `samples` subfolder of the repository. A detailed description of the metadata file format is provided [below](#metadata-format).

<a name="vk-hlsl"></a>
## Using Vulkan features from HLSL

You may choose to use the Vulkan binding model explicitly and assign descriptor sets and bindings like this:

```
[vk::binding(1, 0)] uniform Texture2D tex; // assign tex to set 0 binding 1
[vk::binding(2, 0)] uniform sampler samp;  // assign tex to set 0 binding 2
```

You may use specialization constants as well:

```
[[vk::constant_id(1)]] const float specConstFloat = 1.5;
```

See [here](https://github.com/Microsoft/DirectXShaderCompiler/blob/master/docs/SPIR-V.rst) for more details.

<a name="metadata-format"></a>
## Pipeline Metadata File Format

For each technique described in the input file, `nicegraf-shaderc` emits a file containing information that can be leveraged to simplify pipeline creation. This data includes:

* Description of the pipeline layout;
* Mapping from separate image and sampler bindings to their corresponding auto-generated combined image/sampler bindings (for platforms that don't have full separation between textures and samplers, i.e. OpenGL);
* Any additional metadata specified by the user using the `meta:` tag in the technique description.

A detailed description of the file's format follows.

### General Conventions

A pipeline metadata file (referred to simply as "file" henceforth) is broken into **records**. A **record** is a sequence of **fields** and **raw byte blocks**. A **field** is a 4-byte unsigned integer in network byte order. A **raw byte block** consists of a header and a body. The **body of a raw byte block** is an arbitrary sequence of bytes. The length of the body is always a multiple of 4 bytes. The **header** of a raw byte block is two 32-bit unsigned integers in network byte order. The first is always equal to `0xffffffff` and signifies the beginning of a raw byte block. The second specifies the length of the block's body divided by 4.

A record's type is defined by its layout. The following types of records are defined:

* `HEADER`;
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
* `pipeline_layout_offset` - offset, in bytes, from the beginning of the file, at which the `PIPELINE_LAYOUT` record is stored;
* `image_to_cis_map_offset` - offset, in bytes, from the beginning of the file, at which a `SEPARATE_TO_COMBINED_MAP` record is stored, which maps separate *image* bindings to the corresponding auto-generated combined image/sampler bindings;
* `sampler_to_cis_map_offset` - offset, in bytes, from the beginning of the file, at which a `SEPARATE_TO_COMBINED_MAP` record is stored, which maps separate *sampler* bindings to the corresponding auto-generated combined image/sampler bindings;
* `user_metadata_offset` - offset, in bytes, from the beginning of the file, at which the `USER_METADATA` record is stored;

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
