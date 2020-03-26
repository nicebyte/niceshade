/**
 * Copyright (c) 2019 nicegraf contributors
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

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>
#include "dxcapi.h"
#include "shader_defines.h"
#include "technique_parser.h"
#include <vector>
#include <string>
#include <stdint.h>
#include <variant>
#include <type_traits>

class DxcWrapper {
  template <class T>
  class ComPtr {
  public:
    ComPtr() = default;
    explicit ComPtr(T* ptr) : ptr_(ptr) {
      static_assert(std::is_base_of<IUnknown, T>);
    }
    ComPtr(const ComPtr&) = delete;
    ComPtr(ComPtr &&other) { *this = std::move(other); }
    ~ComPtr() { release(); }
    ComPtr& operator=(const ComPtr &) = delete;
    ComPtr& operator=(const ComPtr&& other) {
      release();
      ptr_ = other.ptr_;
      other.ptr_ = nullptr;
      return *this;
    }
    T* operator->() { return ptr_; }
    const T* operator->() const { return ptr_; }
    T* get() { return ptr_; }
    const T* get() const { return ptr_; }
    T** PtrToContent() { return &ptr_; }

    
  private:
    void release() {
      if (ptr_) {
        ptr_->Release();
      }
    }
    T *ptr_ = nullptr;
  };

  class DynamicLib {
  public:
    DynamicLib() = default;
    DynamicLib(const char **candidate_names, size_t nnames) {
      for (size_t i = 0; h_ == NULL && i < nnames; ++i)
        h_ = LoadLibraryA(candidate_names[i]);
    }
    ~DynamicLib() { if(h_) FreeModule(h_); }
    DynamicLib(const DynamicLib&) = delete;
    DynamicLib& operator=(const DynamicLib&) = delete;
    bool IsValid() const { return h_ == NULL; }
    LPVOID GetProcAddress(LPCSTR name) const { return ::GetProcAddress(h_, name); }
  private:
    HMODULE h_ = NULL;
  };

public:
  struct Result {
    std::vector<uint32_t> spirv_result;
    std::string error_message;
  };

  DxcWrapper();

  Result CompileHlslToSpirv(const char *source,
                            size_t source_size,
                            const char *input_file_name,
                            const technique::entry_point &entry_point,
                            const define_container &global_macro_definitions);

private:
  DynamicLib dxcompiler_dll_;
  ComPtr<IDxcLibrary> library_instance_;
  ComPtr<IDxcCompiler> compiler_instance_;
};