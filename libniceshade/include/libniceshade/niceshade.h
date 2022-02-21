/**
 * Copyright (c) 2022 nicegraf contributors
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

#pragma once

/**
 * @file
 * @brief
 */

/**
 * \mainpage
 *
 * This is the reference documentation for the <a
 * href="http://github.com/nicebyte/niceshade">niceshade</a> library, which can be used by
 * applications to transform HLSL code into shaders for one of the supported backends. See the
 * niceshade <a href="http://github.com/nicebyte/niceshade/blob/master/README.md">readme file</a>
 * for more information about the capabilities, as well as documentation for the accompanying CLI
 * tool.
 *
 * Read below for a brief tutorial, or skip ahead to the detailed documentation of the \ref
 * niceshade namespace members.
 *
 * \subsection build Building and Linking Against the Library
 *
 * `libniceshade` is continuously built and tested under Windows, macOS and Linux. The only
 * supported build system is <a href="http://cmake.org">CMake</a>, and the only build-time
 * dependency is <a href="http://github.com/KhronosGroup/SPIRV-Cross">SPIRV-Cross</a>.
 * `libniceshade` comes bundled with a copy of SPIRV-Cross that it has been tested to work with.  In
 * order to use `libniceshade` from your application, link your own CMake target against the
 * `libniceshade` target defined in <a
 * href="http://github.com/nicebyte/niceshade/blob/master/libniceshade/CMakeLists.txt">`libniceshade/CMakeLists.txt`</a>.
 *
 * \subsection rtdeps Runtime Dependencies
 *
 * At runtime, `libniceshade` depends on the Microsoft <a
 * href="http://github.com/microsoft/DirectXShaderCompiler">DirectXShaderCompiler</a> shared
 * library. The DirectXShaderCompiler shared library file should be present in a location known
 * to your application. `libniceshade` comes with pre-built Windows, macOS and Linux
 * versions of the DirectXShaderCompiler shared library that it has been tested to work with.
 *
 * \subsection usage Using the Library
 * \subsubsection hdrs Headers
 * To be able to use `libniceshade` types and routines in your application, you must add the
 * following line to your includes:
 *
 * ```
 * #include "libniceshade/niceshade.h"
 * ```
 * \subsubsection instance Creating Objects
 *
 * All classes in `libniceshade` that can be instantiated by the client application and have
 * non-trivial initialization logic, should be instantiated using their corresponding `create`
 * static method. For example, to instantiate \ref niceshade::instance,
 * \ref niceshade::instance::create should be used.  This rule does not apply to structs (which have
 * trivial initialization).
 *
 * \subsubsection errs Error Handling
 *
 * All client-facing methods that may fail return values wrapped into \ref
 * niceshade::value_or_error. No client-facing method in `libniceshade` ever throws an exception.
 *
 * \subsubsection inst Creating a Compiler Instance
 *
 * Start by populating a \ref niceshade::instance::options structure. You will need to supply:
 *
 *  - The shader model that your source HLSL uses (e.g. 6_2);
 *  - the path to the folder where the Microsoft DirectXShaderCompiler shared library can be found
 * at runtime;
 *  - a string containing additional parameters that shall be passed directly to Microsoft
 * DirectXShaderCompiler.
 *
 * Below is an example of what code populating the instance options could look like:
 *
 * ```
 * niceshade::instance::options ns_opts {
 *   "6_2",   // use SM 6.2
 *   "-Zpc",  // additional DirectXShaderCompiler option: forbid overriding explicit matrix
 * orientation
 *   "./"     // tells niceshade to look for the DirectXShaderCompiler shared library in the current
 * working directory
 * };
 * ```
 *
 * After that, create a new compiler instance like so:
 *
 * ```
 * auto maybe_ns_instance = niceshade::instance::create(ns_opts);
 * ```
 *
 * Don't forget to check for errors:
 *
 * ```
 * if (maybe_ns_instance.is_error()) {
 *   fprintf(stderr, "niceshade error: [%s]", maybe_ns_instance.error_message().c_str());
 *   exit(1);
 * }
 * auto& ns_instance = maybe_ns_instance.get();
 * ```
 *
 * \subsubsection compiling Compiling HLSL
 *
 * If you have a piece of HLSL code with techniques defined inline, use \ref
 * niceshade::instance::parse_techniques_and_compile to compile all the techniques for your desired
 * targets.
 *
 * Begin by loading your HLSL:
 *
 * ```
 * // assume hlsl_string is loaded from a file somehow
 * auto input_blob = niceshade::input_blob { hlsl_string.data(), hlsl_string.size() };
 * ```
 *
 * The next step is listing your target platforms. The example below targets Metal 1.2 for iOS and
 * OpenGL ES 3.2:
 *
 * ```
 * std::vector<niceshade::target_desc> targets {
 *  { niceshade::target_api::METAL, 1, 2, niceshade::target_platform_class::MOBILE},
 *  { niceshade::target_api::GL, 3, 2, niceshade::target_platform_class::MOBILE}
 * };
 * ```
 *
 * You may also supply additional preprocessor definitions that shall be added when compiling your
 * techniques:
 *
 * ```
 * niceshade::define_container additional_definitions {
 *  {"define1", "value1"}
 * };
 * ```
 *
 * Finally, you may call \ref niceshade::instance::parse_techniques_and_compile:
 *
 * ```
 * auto maybe_results = ns_instance.parse_techniques_and_compile(
 *    input_blob,
 *   "your_file_name.hlsl",
 *    niceshade::const_span<target_desc> {targets.data(), targets.size()},
 *    additional_definitions);
 * ```
 *
 * Note that passing in the correct file name is important if your HLSL code contains `#include`
 * directives.
 *
 * The value returned by the method shall contain an error message if compiling the HLSL code fails:
 *
 * ```
 * if (maybe_results.is_error()) {
 *   fprintf(stderr, "Error while compiling HLSL: %s", maybe_results.error_message().c_str());
 *   exit(1);
 * }
 * ```
 *
 * If the compilation is successful, the returned value is a tuple, where the
 * first element is a sequence of technique descriptions, and the second element is a sequence of
 * corresponding \ref niceshade::compiled_technique objects containing the shader code for each
 * requested target, as well as information about resources consumed by the shader (\ref
 * niceshade::pipeline_layout), and other metadata.
 */

#include "libniceshade/instance.h"