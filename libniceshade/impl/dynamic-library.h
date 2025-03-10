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

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif

#include "impl/platform.h"

#include <vector>
#include <string>

namespace niceshade {

class dynamic_lib {
  public:
  dynamic_lib() = default;

  dynamic_lib(const std::vector<std::string>& paths) noexcept {
    for (size_t i = 0; h_ == nullptr && i < paths.size(); ++i) {
      h_ = LoadLibraryA(paths[i].c_str());
    }
  }

  ~dynamic_lib() noexcept {
#if !HAS_ASAN()
    if (h_) FreeModule(h_);
#endif
  }

  dynamic_lib(const dynamic_lib&) = delete;

  dynamic_lib& operator=(const dynamic_lib&) = delete;

  dynamic_lib(dynamic_lib&& other) noexcept { *this = std::move(other); }
  dynamic_lib& operator=(dynamic_lib&& other) noexcept {
    h_ = other.h_;
    other.h_ = nullptr;
    return *this;
  }

  bool is_valid() const noexcept { return h_ == nullptr; }

  LPVOID get_proc_address(LPCSTR name) const noexcept {
    return GetProcAddress(h_, name);
  }

  private:
  ModuleHandle h_ = nullptr;
};

}  // namespace niceshade
