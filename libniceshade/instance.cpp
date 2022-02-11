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

#include "libniceshade/instance.h"

#include "libniceshade/compilation.h"
#include "libniceshade/pipeline-layout-builder.h"
#include "libniceshade/separate-to-combined-map.h"

namespace libniceshade {

instance::instance(const instance::options& opts)
    : dxc_ {opts.shader_model, opts.dxc_params, opts.dxc_lib_folder} {}

value_or_error<compiled_techniques>
instance::compile(const_span<compiler_input> inputs, const_span<target_desc> targets) {
  compiled_techniques result;
  std::vector<spirv_blob> spirv_blobs;
  for (const auto& input : inputs) {
    const const_span<technique_desc>& techniques = input.technique_descs;

    for (const technique_desc& tech : techniques) {
      result.emplace_back();
      compiled_technique& compiled_tech = result.back();
      compiled_tech.name = tech.name;
      for (const technique_desc::entry_point& ep : tech.entry_points) {
        // Produce SPIR-V.
        auto maybe_spirv_blob = dxc_.compile_hlsl2spv(
            (const char*)input.hlsl.cbegin(),
            input.hlsl.size(),
            input.file_name,
            ep,
            tech.defines);
        NICESHADE_RETURN_IF_ERROR(maybe_spirv_blob);
        if (maybe_spirv_blob.get().size() == 0) { NICESHADE_RETURN_ERROR("no SPIR-V generated"); }
        spirv_blobs.emplace_back(std::move(maybe_spirv_blob.get()));
        pipeline_layout_builder res_layout_builder;
        std::vector<compilation> compilations;
        for (const target_desc& target_info : targets) {
          compilations.emplace_back(ep.stage, spirv_blobs.back(), target_info);
          compilations.back().add_resources_to_pipeline_layout(res_layout_builder);
          // TODO: separate-to-combined map
        }
        auto maybe_res_layout = res_layout_builder.build();
        NICESHADE_RETURN_IF_ERROR(maybe_res_layout);
        compiled_tech.layout = std::move(maybe_res_layout.get());
        for (compilation& c : compilations) {
         auto maybe_compilation_result = c.run(compiled_tech.layout);
         NICESHADE_RETURN_IF_ERROR(maybe_compilation_result);
         compiled_tech.stages.emplace_back();
         compiled_tech.stages.back().result = std::move(maybe_compilation_result.get());
         compiled_tech.stages.back().stage = c.stage();
        }
      }
    }
  }
  return result;
}

}  // namespace libniceshade