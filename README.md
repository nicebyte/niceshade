nicegraf-shaderc
================

 This is a wrapper over [shaderc](https://github.com/google/shaderc/) and [SPIRV-Cross](https://github.com/KhronosGroup/SPIRV-Cross), a command-line tool for
transforming the Vulkan dialect of GLSL into shaders for various other
graphics APIs. Currently supported targets include: desktop OpenGL,
OpenGL ES >= 3.0, Metal and SPIR-V.

##Usage

`ngf_shaderc [options]`

Options:

  * `-f <filename>` - Specifies an input file name to be processed. Shader stage is
    determined from the file name extension.
      * `.vert.glsl` - corrseponds to vertex shader
      * `.frag.glsl` - fragment shader
      * `.geom.glsl` - geometry shader
      * `.tese.glsl` - tess evaluation shader
      * `.tesc.glsl` - tess control shader

  * `-o <filename>` - Name (excluding parent folder and extension) for the output
    file. By default the name of the input file is used.

  * `-D <name>=<value>` - If coming after an `-f` option, specifies an additional
    definition to add when processing the corresponding file.  If coming before
    any of the `-f` options, the definition will be added for all files.

  * `-t <target>` - Generate shader for the given target.  Accepted values are:
      * `gl430`
      * `gles310`
      * `gles300`
      * `msl10`
      * `msl11`
      * `msl12`
      * `msl20`
      * `msl10ios`
      * `msl11ios`
      * `msl12ios`
      * `msl20ios`
      * spv 
    If specified multiple times, shaders for all of the mentioned targets will
    be generated.

  * `-O <path>` - Folder to store output files in. Default is the current working
    directory.

##Status

 This project is under active development. Features that are planned, but not implemented yet:
 
 * Support for GL ES 2
 * Generating of shader metadata files and documenting the format
 * Some way of combining multiple shader stages into a single file