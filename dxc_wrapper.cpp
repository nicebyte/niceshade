/**
 * Copyright (c) 2020 nicegraf contributors
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


#define _CRT_SECURE_NO_WARNINGS
#include "dxc_wrapper.h"
#include <string>
#include <stdlib.h>
#include <string_view>

DEFINE_CROSS_PLATFORM_UUIDOF(IDxcLibrary)
DEFINE_CROSS_PLATFORM_UUIDOF(IDxcCompiler)

namespace {
  static constexpr std::string_view dxc_lib_filename_candidates[] = {
    "dxcompiler.dll",
    "libdxcompiler.so",
    "libdxcompiler.dylib"
  };

  static constexpr std::string_view dxc_lib_dir_candidates[] = {
    "third_party/dxc",
    "../third_party/dxc"
  };

  static std::vector<std::string> get_dxc_lib_path_candidates(const std::string& exe_dir) {
    std::vector<std::string> paths;
    for (const auto& filename : dxc_lib_filename_candidates)
      paths.emplace_back(exe_dir + "/" + std::string(filename));

    for (const auto& dir_name : dxc_lib_dir_candidates)
      for (const auto& filename : dxc_lib_filename_candidates)
        paths.emplace_back(std::string(dir_name) + "/" + std::string(filename));
    return paths;
  }

  std::wstring towstring(const char* src, size_t len) {
    std::wstring ws(len + 1, '\0');
    std::mbstowcs(ws.data(), src, len);
    return ws;
  }
}


dxc_wrapper::dxc_wrapper(const std::string &sm, bool enable_spv_opt, const std::string& exe_dir) :
    shader_model_(towstring(sm.c_str(), sm.length())),
    enable_spv_opt_(enable_spv_opt),
    dxcompiler_dll_(get_dxc_lib_path_candidates(exe_dir)) {
  if (dxcompiler_dll_.IsValid()) {
    fprintf(stderr, "dxcompiler library not loaded.\n");
    exit(1);
  }
  auto create_proc =
      (DxcCreateInstanceProc)dxcompiler_dll_.get_proc_address("DxcCreateInstance");
  if (NULL == create_proc) {
    exit(1);
  }
  
  library_instance_ =
    com_ptr<IDxcLibrary>([&](auto ptr) {
      return create_proc(CLSID_DxcLibrary,
                         __uuidof(IDxcLibrary),
                         (LPVOID*)ptr);
    });

  compiler_instance_ =
    com_ptr<IDxcCompiler>([&](auto ptr) {
      return create_proc(CLSID_DxcCompiler,
                         __uuidof(IDxcCompiler),
                         (LPVOID*)ptr);
    });

    include_handler_ =
        com_ptr<IDxcIncludeHandler>([&](auto ptr) {
          return library_instance_->CreateIncludeHandler(ptr);
        });
}

dxc_wrapper::result dxc_wrapper::compile_hlsl2spv(
    const char* source,
    size_t source_size,
    const char* input_file_name,
    const technique::entry_point& entry_point,
    const define_container& defines) {
  auto input_blob = com_ptr<IDxcBlobEncoding>([&](auto ptr) {
    return library_instance_->CreateBlobWithEncodingFromPinned(
        source,
        (uint32_t)source_size,
        0,
        ptr);
  });

  LPCWSTR args[] = {
    L"-spirv", // enable spir-v codegen.
    L"-Zpc",
    enable_spv_opt_ ? L"-O3" : L"-O0"
  };
  const size_t args_count = sizeof(args)/sizeof(args[0]);

  const std::wstring winput_file_name =
      towstring(input_file_name, strlen(input_file_name));
  const std::wstring wentry_point_name =
      towstring(entry_point.name.c_str(), entry_point.name.size());

  std::vector<std::pair<std::wstring, std::wstring>> wdefines;
  std::vector<DxcDefine> dxc_defines;
  wdefines.reserve(defines.size());
  dxc_defines.reserve(defines.size());
  for (const std::pair<std::string, std::string>& define : defines) {
    wdefines.emplace_back(towstring(define.first.c_str(), define.first.size()),
                          towstring(define.second.c_str(), define.second.size()));
    const auto &wdefine = wdefines.back();
    dxc_defines.emplace_back(DxcDefine { 
                                 wdefine.first.c_str(),
                                 wdefine.second.empty()
                                     ? NULL
                                     : wdefine.second.c_str() });
  }

  const std::wstring target_profile = [&entry_point]() {
    switch (entry_point.kind) {
    case shader_kind::vertex:
      return L"vs_";
    case shader_kind::fragment:
      return L"ps_";
    default:
      exit(1);
    }
  }() + shader_model_;
  auto dxc_result =
      com_ptr<IDxcOperationResult>([&](auto ptr) {
        return compiler_instance_->Compile(
            input_blob.get(),
            winput_file_name.c_str(),
            wentry_point_name.c_str(),
            target_profile.c_str(),
            args,
            (uint32_t)args_count,
            dxc_defines.data(),
            (uint32_t)dxc_defines.size(),
            include_handler_.get(),
            ptr);
      });

  result result;

  auto spirv_blob =
      com_ptr<IDxcBlob>([&](auto ptr) { return dxc_result->GetResult(ptr); });

  if (spirv_blob->GetBufferSize() > 0) {
    result.spirv_result =
        std::vector<uint32_t>(spirv_blob->GetBufferSize() / sizeof(uint32_t),
                              0u);
    memcpy(result.spirv_result.data(),
           spirv_blob->GetBufferPointer(),
           spirv_blob->GetBufferSize());
  }

  auto errmsg_blob =
      com_ptr<IDxcBlobEncoding>([&](auto ptr) {
        return dxc_result->GetErrorBuffer(ptr);
      });

  if (errmsg_blob->GetBufferSize() > 0) {
    result.diag_message = std::string(errmsg_blob->GetBufferSize() + 1u,
                                       '\0');
    memcpy((void*)result.diag_message.data(),
            errmsg_blob->GetBufferPointer(),
            errmsg_blob->GetBufferSize());
  }

  return result;
 }
