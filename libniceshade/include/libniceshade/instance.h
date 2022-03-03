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

#include "libniceshade/error.h"
#include "libniceshade/input.h"
#include "libniceshade/output.h"
#include "libniceshade/span.h"
#include "libniceshade/target.h"

#include <stdint.h>
#include <string>
#include <vector>

/**
 * @file
 * @brief
 */

namespace niceshade {

class dxc_wrapper;

/**
 * An instance of niceshade compiler.
 */
class instance {
public:
  /**
   * niceshade compiler configuration.
   */
  struct options {
    /**
     * The HLSL shader model to use. Should be specified as [major version]_[minor_version], e.g.
     * "6_2".
     */
    std::string shader_model = "6_2";

    /**
     * A sequence of additional parameters to be passed directly to Microsoft DirectXShaderCompiler.
     */
    span<std::string> dxc_params;

    /**
     * The path to the folder in which niceshade shall look for the Microsoft DirectXShaderCompiler
     * shared library file.
     */
    std::string dxc_lib_folder = "./";
  };

  /**
   * Creates a new compiler instance.
   *
   * @param opts niceshade configuration.
   * @return The newly created instance object, or an error on failure.
   */
  static value_or_error<instance> create(const options& opts) noexcept;

  instance() = default;
  ~instance() noexcept;
  instance(const instance&) = delete;
  instance& operator=(const instance&) = delete;
  instance(instance&& other) noexcept { *this = std::move(other); }
  instance& operator=(instance&& other) noexcept {
    dxc_       = other.dxc_;
    other.dxc_ = nullptr;
    return *this;
  }

  /**
   * Parses techniques defined directly in the source HLSL and compiles them.
   *
   * @param in_blob The source HLSL.
   * @param file_name The name of the file from which the source HLSL originates. It is important
   * for this to be correct for HLSL `#include` directives to work properly.
   * @param targets A list of descriptions of targets to generate shaders for.
   * @param global_defines A list of additional preprocessor definitions to add during compilation.
   * @return \ref descs_and_compiled_techniques
   */
  value_or_error<descs_and_compiled_techniques> parse_techniques_and_compile(
      input_blob              in_blob,
      const char*             file_name,
      const_span<target_desc> targets,
      const define_container& global_defines) noexcept;

  /**
   * Compiles several \ref compiler_input units at a time. Note that any techniques defined inline
   * in the HLSL code are ignored.
   * @param compiler_inputs A sequence of compiler inputs to process.
   * @param targets A sequence of descriptions of targets to generate output for.
   * @return \ref compiled_techniques
   */
  value_or_error<compiled_techniques>
  compile(const_span<compiler_input> compiler_inputs, const_span<target_desc> targets) noexcept;

private:
  dxc_wrapper* dxc_;
};

}  // namespace niceshade