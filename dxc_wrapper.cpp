#include "dxc_wrapper.h"
#include <string>
#include <locale>
#include <codecvt>
#include <stdio.h>

namespace {
  static const char* dxc_lib_candidates[] = {
    "third_party/dxcompiler.dll",
    "dxcompiler.dll",
  };
  static constexpr size_t ndxc_lib_candidates_ =
      sizeof(dxc_lib_candidates) / sizeof(dxc_lib_candidates[0]);
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

  const HRESULT create_library_result =
      create_proc(CLSID_DxcLibrary, __uuidof(IDxcLibrary),
                  (LPVOID*)library_instance_.PtrToContent());
  if (create_library_result != S_OK) {
    exit(1);
  }

  const HRESULT create_compiler_result =
      create_proc(CLSID_DxcCompiler, __uuidof(IDxcCompiler),
                  (LPVOID*)compiler_instance_.PtrToContent());
  if (create_compiler_result != S_OK) {
    exit(1);
  }
}

DxcWrapper::Result DxcWrapper::CompileHlslToSpirv(
    const char* source,
    size_t source_size,
    const char* input_file_name,
    const technique::entry_point& entry_point,
    const define_container& global_macro_definitions) {
  ComPtr<IDxcBlobEncoding> input_blob;

  const HRESULT create_blob_result =
    library_instance_->CreateBlobWithEncodingFromPinned(
        source,
        source_size,
        0,
        input_blob.PtrToContent());
  if (create_blob_result != S_OK) {
    exit(1);
  }

  LPCWSTR args[] = {
    L"-spirv",
    L"-Zpc",
  };
  const size_t args_count = sizeof(args)/sizeof(args[0]);

  std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> wstr_conv;
  const std::wstring winput_file_name = wstr_conv.from_bytes(input_file_name);
  const std::wstring wentry_point_name = wstr_conv.from_bytes(entry_point.name.c_str());
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
  ComPtr<IDxcOperationResult> dxc_result;
  const HRESULT compile_result =
    compiler_instance_->Compile(
        input_blob.get(),
        winput_file_name.c_str(),
        wentry_point_name.c_str(),
        target_profile,
        args,
        args_count,
        NULL,
        0,
        NULL,
        dxc_result.PtrToContent());
  if (compile_result != S_OK) {
    exit(1);
  }

  Result result;

  ComPtr<IDxcBlob> spirv_blob;
  dxc_result->GetResult(spirv_blob.PtrToContent());
  if (spirv_blob->GetBufferSize() > 0) {
    result.spirv_result =
        std::vector<uint32_t>(spirv_blob->GetBufferSize() / sizeof(uint32_t),
                              0u);
    memcpy(result.spirv_result.data(),
           spirv_blob->GetBufferPointer(),
           spirv_blob->GetBufferSize());
  }

  ComPtr<IDxcBlobEncoding> errmsg_blob;
  dxc_result->GetErrorBuffer(errmsg_blob.PtrToContent());
  if (errmsg_blob->GetBufferSize() > 0) {
    result.error_message = std::string(errmsg_blob->GetBufferSize() + 1u,
                                       '\0');
    memcpy((void*)result.error_message.data(),
            errmsg_blob->GetBufferPointer(),
            errmsg_blob->GetBufferSize());
  }

  return result;
 }
