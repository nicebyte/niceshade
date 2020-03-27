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

#include "dxc_wrapper.h"
#include <string>
#include <stdlib.h>
#include <stdio.h>

namespace {
  static const char* dxc_lib_candidates[] = {
    "third_party/dxcompiler.dll",
    "dxcompiler.dll",
  };

  static constexpr size_t ndxc_lib_candidates_ =
      sizeof(dxc_lib_candidates) / sizeof(dxc_lib_candidates[0]);

  std::wstring towstring(const char* src, size_t len) {
    std::wstring ws(len + 1, '\0');
    std::mbstowcs(ws.data(), src, len);
    return ws;
  }
}

DxcWrapper::DxcWrapper() : 
    dxcompiler_dll_(dxc_lib_candidates, ndxc_lib_candidates_) {
  if (dxcompiler_dll_.IsValid()) {
    exit(1);
  }
  auto create_proc =
      (DxcCreateInstanceProc)dxcompiler_dll_.GetProcAddress("DxcCreateInstance");
  if (NULL == create_proc) {
    exit(1);
  }
  
  library_instance_ =
    ComPtr<IDxcLibrary>([create_proc](auto ptr) {
      return create_proc(CLSID_DxcLibrary,
                         __uuidof(IDxcLibrary),
                         (LPVOID*)ptr);
    });

  compiler_instance_ =
    ComPtr<IDxcCompiler>([create_proc](auto ptr) {
      return create_proc(CLSID_DxcCompiler,
                         __uuidof(IDxcCompiler),
                         (LPVOID*)ptr);
    });
}

DxcWrapper::Result DxcWrapper::CompileHlslToSpirv(
    const char* source,
    size_t source_size,
    const char* input_file_name,
    const technique::entry_point& entry_point,
    const define_container& global_macro_definitions) {
  auto input_blob = ComPtr<IDxcBlobEncoding>([&](auto ptr) {
    return library_instance_->CreateBlobWithEncodingFromPinned(
        source,
        source_size,
        0,
        ptr);
  });

  LPCWSTR args[] = {
    L"-spirv",
    L"-Zpc",
  };
  const size_t args_count = sizeof(args)/sizeof(args[0]);

  const std::wstring winput_file_name =
      towstring(input_file_name, strlen(input_file_name));
  const std::wstring wentry_point_name =
      towstring(entry_point.name.c_str(), entry_point.name.size());

  const LPCWSTR target_profile = [&entry_point]() {
    switch (entry_point.kind) {
    case shaderc_vertex_shader:
      return L"vs_6_0";
    case shaderc_fragment_shader:
      return L"ps_6_0";
    default:
      exit(1);
    }
  }();
  auto dxc_result =
      ComPtr<IDxcOperationResult>([&](auto ptr) {
        return compiler_instance_->Compile(
            input_blob.get(),
            winput_file_name.c_str(),
            wentry_point_name.c_str(),
            target_profile,
            args,
            args_count,
            NULL,
            0,
            NULL,
            ptr);
      });

  Result result;

  auto spirv_blob =
      ComPtr<IDxcBlob>([&](auto ptr) {
        return dxc_result->GetResult(ptr);
      });

  if (spirv_blob->GetBufferSize() > 0) {
    result.spirv_result =
        std::vector<uint32_t>(spirv_blob->GetBufferSize() / sizeof(uint32_t),
                              0u);
    memcpy(result.spirv_result.data(),
           spirv_blob->GetBufferPointer(),
           spirv_blob->GetBufferSize());
  }

  auto errmsg_blob =
      ComPtr<IDxcBlobEncoding>([&](auto ptr) {
        return dxc_result->GetErrorBuffer(ptr);
      });

  if (errmsg_blob->GetBufferSize() > 0) {
    result.error_message = std::string(errmsg_blob->GetBufferSize() + 1u,
                                       '\0');
    memcpy((void*)result.error_message.data(),
            errmsg_blob->GetBufferPointer(),
            errmsg_blob->GetBufferSize());
  }

  return result;
 }
