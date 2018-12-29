# nicegraf-shaderc

**nicegraf-shaderc** is a command-line tool that transforms HLSL code into shaders for various graphics APIs. Presently, the following targets are supported:

* OpenGL 4.3
* OpenGL ES 3.1+
* Metal 1.0+
* SPIR-V

The input HLSL files may contain definitions of several entry points for different shader stages. The entry points can be configured into a single rendering pipeline (with additional options, if desired) using a special directive. For each of these configurations (called *techniques*), the tool will generate platform-specific shaders.

In addition to generating shaders, the tool captures and writes out the information about resources (textures, samplers, etc.) used by each technique defined in the input file. This information can be used by the application for various purposes, such as streamlining Vulkan pipeline layout creation.

 This tool is powered by [shaderc](https://github.com/google/shaderc) and [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross).


## Project Status

This project is under active development. Some features may change significantly.

## Basic Usage

`nicegraf_shaderc <input file> <options>`

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

## Specifying Techniques

Techniques are specified using a special comment:

`//T: <technique name> <tags>`

The technique name may include alphanumeric characters and underscores.

A tag is a name-value pair separated by a colon. For example, `vs:VSMain` is a tag, `vs` is the tag name and `VSMain` is the value.

The following tag names are valid:
* `vs` - the tag value specifies the entry point for the vertex shader stage;
* `ps` - the tag value specifies the entry point for the pixel shader stage;
* `define` - the tag value specifies an additional preprocessor definition;
* `meta` - the tag value specifies an additional metadata entry.

A valid technique definition must at least specify an entry point for the vertex stage.

## Pipeline Metadata

For each technique specified in the input file, **nicegraf_shaderc** will produce a corresponding `.pipeline` file, which contains the following information:

* List of all resources consumed by the technique, including their types, bindings and which pipeline stages they are used by;
* A mapping from separate image and sampler bindings to auto-generated combined image/sampler bindings (relevant for targets which don't have full separation between textures and samplers at the shader level, i.e. OpenGL);
* Any additional metadata provided by the user in the technique specification using the `meta:` tag.

`.pipeline` files are binary, and can be converted to human-readable JSON using the `display_metadata` utility, source code for which is provided in the `samples` subfolder of the `nicegraf-shaderc` repo.
