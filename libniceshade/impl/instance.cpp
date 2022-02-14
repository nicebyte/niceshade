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

#include "libniceshade/impl/dxc-wrapper.h"
#include "libniceshade/impl/compilation.h"
#include "libniceshade/impl/pipeline-layout-builder.h"
#include "libniceshade/impl/separate-to-combined-builder.h"
#include "libniceshade/impl/technique-parser.h"

namespace libniceshade {

instance::instance(const instance::options& opts)
    : dxc_ {new dxc_wrapper{opts.shader_model, opts.dxc_params, opts.dxc_lib_folder}} {}

instance::~instance() { if (dxc_) delete dxc_; }

value_or_error<compiled_techniques>
instance::compile(const_span<compiler_input> inputs, const_span<target_desc> targets) {
  compiled_techniques result;
  for (const auto& input : inputs) {
    const const_span<technique_desc>& techniques = input.technique_descs;

    for (const technique_desc& tech : techniques) {
      std::vector<spirv_blob> spirv_blobs;
      // Produce SPIR-V.
      for (const technique_desc::entry_point& ep : tech.entry_points) {
        auto maybe_spirv_blob = dxc_->compile_hlsl2spv(
            (const char*)input.hlsl.cbegin(),
            input.hlsl.size(),
            input.file_name,
            ep,
            tech.defines);
        NICESHADE_RETURN_IF_ERROR(maybe_spirv_blob);
        if (maybe_spirv_blob.get().size() == 0) { NICESHADE_RETURN_ERROR("no SPIR-V generated"); }
        spirv_blobs.emplace_back(std::move(maybe_spirv_blob.get()));
      }

      // Create compilations and populate the pipeline layout.
      pipeline_layout_builder      res_layout_builder;
      separate_to_combined_builder image_map_builder;
      separate_to_combined_builder sampler_map_builder;
      std::vector<compilation>     compilations;
      for (const target_desc& target_info : targets) {
        for (const technique_desc::entry_point& ep : tech.entry_points) {
          const intptr_t ep_idx = &ep - tech.entry_points.data();
          compilations.emplace_back(ep.stage, spirv_blobs[ep_idx], target_info);
          compilations.back().add_resources_to_pipeline_layout(res_layout_builder);
          compilations.back().add_cis_to_map(image_map_builder, sampler_map_builder);
        }
      }
      auto maybe_res_layout = res_layout_builder.build();
      NICESHADE_RETURN_IF_ERROR(maybe_res_layout);

      // Create a new compiled technique.
      result.emplace_back();
      compiled_technique& compiled_tech = result.back();
      compiled_tech.name                = tech.name;
      compiled_tech.layout              = std::move(maybe_res_layout.get());
      compiled_tech.image_map           = std::move(image_map_builder.build());
      compiled_tech.sampler_map         = std::move(sampler_map_builder.build());

      // Run all compilations.
      for (compilation& c : compilations) {
        if (compiled_tech.targeted_outputs.empty() ||
            compiled_tech.targeted_outputs.back().target != c.target()) {
          compiled_tech.targeted_outputs.emplace_back();
          compiled_tech.targeted_outputs.back().target = c.target();
        }
        targeted_output& target_out               = compiled_tech.targeted_outputs.back();
        auto             maybe_compilation_result = c.run(compiled_tech.layout);
        NICESHADE_RETURN_IF_ERROR(maybe_compilation_result);
        target_out.stages.emplace_back();
        target_out.stages.back().result = std::move(maybe_compilation_result.get());
        target_out.stages.back().stage  = c.stage();
      }
    }
  }
  return result;
}

value_or_error<descs_and_compiled_techniques> instance::parse_techniques_and_compile(
    input_blob              in_blob,
    const char*             file_name,
    const_span<target_desc> targets,
    const define_container& global_defines) {
  auto maybe_parsed_techniques = parse_techniques(in_blob, global_defines);
  NICESHADE_RETURN_IF_ERROR(maybe_parsed_techniques);
  if (maybe_parsed_techniques.get().size() == 0) {
    NICESHADE_RETURN_ERROR("The input file does not appear to define any techniques. "
                           "Define techniques with a special comment (`//T:').\n");
  }
  std::vector<compiler_input> inputs;
  compiler_input              input;
  input.technique_descs = const_span<technique_desc>(
      maybe_parsed_techniques.get().data(),
      maybe_parsed_techniques.get().size());
  input.file_name                = file_name;
  input.hlsl                     = in_blob;
  auto maybe_compiled_techniques = compile(const_span<compiler_input> {&input, 1u}, targets);
  NICESHADE_RETURN_IF_ERROR(maybe_compiled_techniques);
  assert(maybe_compiled_techniques.get().size() == maybe_compiled_techniques.get().size());
  return std::make_tuple(
      std::move(maybe_parsed_techniques.get()),
      std::move(maybe_compiled_techniques.get()));
}

}  // namespace libniceshade