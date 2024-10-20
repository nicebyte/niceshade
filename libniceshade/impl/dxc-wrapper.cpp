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

#ifndef _CRT_SECURE_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#endif
#include "impl/dxc-wrapper.h"

#include "impl/error-macros.h"

#include <stdlib.h>
#include <string>

namespace {

#if defined(_WIN64) || defined(_WIN32)
static const std::string dxc_lib_filename = "dxcompiler.dll";
#elif defined(__APPLE__)
  #if defined(__aarch64__)
  static const std::string dxc_lib_filename = "libdxcompiler-armv8.dylib";
  #else
  static const std::string dxc_lib_filename = "libdxcompiler.dylib";
  #endif
#else
static const std::string dxc_lib_filename = "libdxcompiler.so";
#endif

static std::vector<std::string> get_dxc_lib_path_candidates(const std::string& exe_dir) noexcept {
  return {
      exe_dir + "/" + dxc_lib_filename,
      exe_dir + "/deps/dxc/" + dxc_lib_filename,
      "/../deps/dxc/" + dxc_lib_filename};
}

std::wstring towstring(const char* src, size_t len) noexcept {
  std::wstring ws(len + 1, '\0');
  std::mbstowcs(ws.data(), src, len);
  return ws;
}
}  // namespace

namespace niceshade {

value_or_error<dxc_wrapper> dxc_wrapper::create(
    const std::string& sm,
    span<std::string>  dxc_params,
    const std::string& exe_dir,
    hlsl_diagnostic_callback diag_callback) noexcept {
  dxc_wrapper result;
  result.shader_model_   = towstring(sm.c_str(), sm.length());
  result.dxcompiler_dll_ = get_dxc_lib_path_candidates(exe_dir);
  result.dxc_params_.emplace_back(L"-spirv");  // always enable spir-v codegen.
  // Convert dxc parameters to wide string.
  for (const std::string& dxc_param : dxc_params) {
    wchar_t* wide_dxc_param = new wchar_t[dxc_param.size() + 1u];
    std::mbstowcs(wide_dxc_param, dxc_param.c_str(), dxc_param.length() + 1u);
    result.dxc_params_.emplace_back(wide_dxc_param);
  }

  // Verify that the dymamic library could be loaded.
  if (result.dxcompiler_dll_.is_valid()) {
    fprintf(stderr, "dxcompiler library not loaded (exe dir was \"%s\").\n", exe_dir.c_str());
    exit(1);
  }

  // Look up the function for creating an instance of the library.
  auto create_proc =
      (DxcCreateInstanceProc)result.dxcompiler_dll_.get_proc_address("DxcCreateInstance");
  if (nullptr == create_proc) { NICESHADE_RETURN_ERROR("failed to load DxcCreateInstance"); }

  // Instantiate library, compiler and include handler.
  result.library_instance_ = com_ptr<IDxcLibrary>(
      [&](auto ptr) { return create_proc(CLSID_DxcLibrary, __uuidof(IDxcLibrary), (LPVOID*)ptr); });

  result.compiler_instance_ = com_ptr<IDxcCompiler>([&](auto ptr) {
    return create_proc(CLSID_DxcCompiler, __uuidof(IDxcCompiler), (LPVOID*)ptr);
  });

  result.include_handler_ = com_ptr<IDxcIncludeHandler>(
      [&](auto ptr) { return result.library_instance_->CreateIncludeHandler(ptr); });

  result.diag_callback_ = diag_callback;

  return result;
}

dxc_wrapper::~dxc_wrapper() noexcept {
  for (size_t i = 1u; i < dxc_params_.size(); ++i) delete[] dxc_params_[i];
}

value_or_error<spirv_blob> dxc_wrapper::compile_hlsl2spv(
    const char*                        source,
    size_t                             source_size,
    const char*                        input_file_name,
    const technique_desc::entry_point& entry_point,
    const define_container&            defines) noexcept {
  auto input_blob = com_ptr<IDxcBlobEncoding>([&](auto ptr) {
    return library_instance_
        ->CreateBlobWithEncodingFromPinned(source, (uint32_t)source_size, 0, ptr);
  });

  const std::wstring winput_file_name = towstring(input_file_name, strlen(input_file_name));
  const std::wstring wentry_point_name =
      towstring(entry_point.name.c_str(), entry_point.name.size());

  // This vector contains the wide-string versions of the defines.
  std::vector<std::pair<std::wstring, std::wstring>> wdefines;

  // This vector contains DxcDefine objects that refer back to strings from
  // the preceding `wdefines` vector. Those actually get fed to the dxc compiler.
  std::vector<DxcDefine> dxc_defines;

  wdefines.reserve(defines.size());
  dxc_defines.reserve(defines.size());
  for (const std::pair<std::string, std::string>& define : defines) {
    wdefines.emplace_back(
        towstring(define.first.c_str(), define.first.size()),
        towstring(define.second.c_str(), define.second.size()));
    const auto& wdefine = wdefines.back();
    dxc_defines.emplace_back(DxcDefine {
        wdefine.first.c_str(),
        wdefine.second.empty() ? nullptr : wdefine.second.c_str()});
  }

  const std::wstring target_profile = [&entry_point]() {
    switch (entry_point.stage) {
    case pipeline_stage::vertex: return L"vs_";
    case pipeline_stage::fragment: return L"ps_";
    case pipeline_stage::compute: return L"cs_";
    default: exit(1);
    }
  }() + shader_model_;
  auto dxc_result = com_ptr<IDxcOperationResult>([&, this](auto ptr) {
    return compiler_instance_->Compile(
        input_blob.get(),
        winput_file_name.c_str(),
        wentry_point_name.c_str(),
        target_profile.c_str(),
        dxc_params_.data(),
        (uint32_t)dxc_params_.size(),
        dxc_defines.data(),
        (uint32_t)dxc_defines.size(),
        include_handler_.get(),
        ptr);
  });

  auto dxc_spirv_blob = com_ptr<IDxcBlob>([&](auto ptr) { return dxc_result->GetResult(ptr); });
  auto errmsg_blob =
      com_ptr<IDxcBlobEncoding>([&](auto ptr) { return dxc_result->GetErrorBuffer(ptr); });
  const size_t errmsg_blob_size = errmsg_blob->GetBufferSize();
  if (errmsg_blob_size && diag_callback_) {
    diag_callback_((const char*)errmsg_blob->GetBufferPointer(), errmsg_blob->GetBufferSize());
  }

  if (dxc_spirv_blob.get() != nullptr && dxc_spirv_blob->GetBufferSize() > 0) {
    const uint32_t* dxc_blob_start =
        reinterpret_cast<uint32_t*>(dxc_spirv_blob->GetBufferPointer());
    const uint32_t* dxc_blob_end =
        dxc_blob_start + dxc_spirv_blob->GetBufferSize() / sizeof(uint32_t);
    return spirv_blob(dxc_blob_start, dxc_blob_end);
  } else {
    NICESHADE_RETURN_ERROR("failed to compile HLSL to SPIR-V");
  }
}

}  // namespace niceshade
