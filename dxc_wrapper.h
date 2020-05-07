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

#ifdef _MSC_VER
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <unknwn.h>
#define ModuleHandle HMODULE
#undef max
#undef min
#else
#include "WinAdapter.h"
#include <dlfcn.h>
#define LoadLibraryA(name) dlopen(name, RTLD_NOW)
#define GetProcAddress(h, n) dlsym(h, n)
#define FreeModule(h) dlclose(h)
#define ModuleHandle void*
#endif
#include "dxcapi.h"
#include "shader_defines.h"
#include "technique_parser.h"
#include <vector>
#include <string>
#include <stdint.h>
#include <variant>
#include <type_traits>

class dxc_wrapper {
  template <class T>
  class com_ptr {
  public:
    com_ptr() = default;
    explicit com_ptr(T* ptr) : ptr_(ptr) {
      static_assert(std::is_base_of<IUnknown, T>::value);
    }
    template <class F>
    explicit com_ptr(F create_fn) {
      static_assert(std::is_invocable_r<HRESULT, F, T**>::value);
      const HRESULT create_result = create_fn(&ptr_);
      if (create_result != S_OK)
        exit(1);
    }
    com_ptr(const com_ptr&) = delete;
    com_ptr(com_ptr &&other) { *this = std::move(other); }
    ~com_ptr() { release(); }
    com_ptr& operator=(const com_ptr &) = delete;
    com_ptr& operator=(com_ptr&& other) noexcept {
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

  class dynamic_lib {
  public:
    dynamic_lib() = default;
    dynamic_lib(const std::vector<std::string>& paths) {
      for (size_t i = 0; h_ == NULL && i < paths.size(); ++i)
        h_ = LoadLibraryA(paths[i].c_str());
    }
    ~dynamic_lib() { if(h_) FreeModule(h_); }
    dynamic_lib(const dynamic_lib&) = delete;
    dynamic_lib& operator=(const dynamic_lib&) = delete;
    bool IsValid() const { return h_ == NULL; }
    LPVOID get_proc_address(LPCSTR name) const { return GetProcAddress(h_, name); }
  private:
    ModuleHandle h_ = NULL;
  };

public:
  struct result {
    std::vector<uint32_t> spirv_result;
    std::string diag_message;
    bool HasData() const { return spirv_result.size() > 0; }
    bool HasDiagMessage() const { return diag_message.size() > 0; }
  };

  dxc_wrapper(const std::string &sm, bool enable_spv_opt, bool enable_16bit_types, const std::string& exe_dir);

  result compile_hlsl2spv(const char *source,
                          size_t source_size,
                          const char *input_file_name,
                          const technique::entry_point &entry_point,
                          const define_container &defines);

private:
  std::wstring shader_model_;
  bool enable_spv_opt_;
  bool enable_16bit_types_;
  dynamic_lib dxcompiler_dll_;
  com_ptr<IDxcLibrary> library_instance_;
  com_ptr<IDxcCompiler> compiler_instance_;
  com_ptr<IDxcIncludeHandler> include_handler_;
};
