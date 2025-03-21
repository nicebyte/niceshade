/**
 * Copyright (c) 2024 nicegraf contributors
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

#define _CRT_SECURE_NO_WARNING

#include "impl/com-ptr.h"
#include "impl/dynamic-library.h"
#include "impl/platform.h"
#include "impl/technique-parser.h"
#include "libniceshade/common-types.h"
#include "libniceshade/error.h"
#include "libniceshade/span.h"

#include <stdint.h>
#include <string>
#include <vector>

namespace niceshade {

class dxc_wrapper {
public:
  struct result {
    std::vector<uint32_t> spirv_code;
    std::string           diag_message;

    bool has_data() const noexcept { return spirv_code.size() > 0; }

    bool has_diag_msg() const noexcept { return diag_message.size() > 0; }
  };

  static value_or_error<dxc_wrapper> create(
      const std::string&       sm,
      span<std::string>        dxc_params,
      const std::string&       exe_dir,
      hlsl_diagnostic_callback diag_callback) noexcept;

  dxc_wrapper() noexcept = default;
  dxc_wrapper(dxc_wrapper&&) noexcept = default;
  ~dxc_wrapper() noexcept;

  dxc_wrapper& operator=(dxc_wrapper&&) = default;

  value_or_error<spirv_blob> compile_hlsl2spv(
      const char*                        source,
      size_t                             source_size,
      const char*                        input_file_name,
      const technique_desc::entry_point& entry_point,
      const define_container&            defines) noexcept;

private:
  std::wstring                shader_model_;
  dynamic_lib                 dxcompiler_dll_;
  com_ptr<IDxcLibrary>        library_instance_;
  com_ptr<IDxcCompiler>       compiler_instance_;
  com_ptr<IDxcIncludeHandler> include_handler_;
  std::vector<LPCWSTR>        dxc_params_;
  hlsl_diagnostic_callback    diag_callback_ = nullptr;
};

}  // namespace niceshade
