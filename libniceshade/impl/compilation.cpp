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

#define _CRT_SECURE_NO_WARNINGS

#include "libniceshade/impl/compilation.h"

#include "spirv_glsl.hpp"
#include "spirv_msl.hpp"

namespace libniceshade {

compilation::compilation(
    pipeline_stage     stage,
    const spirv_blob&       spirv_code,
    const target_desc& target_info)
    : target_info_(target_info),
      stage_(stage),
      original_spirv_(std::move(spirv_code)) {
  switch (target_info_.api) {
  case target_api::GL: {
    auto gl_compiler =
        std::make_unique<spirv_cross::CompilerGLSL>(spirv_code.data(), spirv_code.size());
    spirv_cross::CompilerGLSL::Options opts;
    opts.version                 = target_info.version_maj * 100u + target_info.version_min * 10u;
    opts.separate_shader_objects = true;
    opts.es                      = (target_info.platform == target_platform_class::MOBILE);
    gl_compiler->set_common_options(opts);
    gl_compiler->build_dummy_sampler_for_combined_images();
    gl_compiler->build_combined_image_samplers();
    spv_cross_compiler_ = std::move(gl_compiler);
    break;
  }
  case target_api::VULKAN: {
    spv_cross_compiler_ =
        std::make_unique<spirv_cross::CompilerReflection>(spirv_code.data(), spirv_code.size());
    break;
  }
  case target_api::METAL: {
    auto msl_compiler =
        std::make_unique<spirv_cross::CompilerMSL>(spirv_code.data(), spirv_code.size());
    spirv_cross::CompilerMSL::Options opts;
    opts.set_msl_version(target_info.version_maj, target_info.version_min);
    if (target_info.version_min >= 1 && target_info.version_maj >= 2) {
      // Enable native texel buffers on Metal 2.1+.
      opts.texture_buffer_native = true;
    }
    const bool ios = target_info.platform == target_platform_class::MOBILE;
    opts.platform =
        ios ? spirv_cross::CompilerMSL::Options::iOS : spirv_cross::CompilerMSL::Options::macOS;
    opts.enable_decoration_binding = true;
    msl_compiler->set_msl_options(opts);
    spv_cross_compiler_ = std::move(msl_compiler);
    break;
  }
  default: assert(false);
  }

  // Assign human-readable names to combined image samplers, and set appropriate
  // binding and set decorations for them.
  const spirv_cross::SmallVector<spirv_cross::CombinedImageSampler>& cis_array =
      spv_cross_compiler_->get_combined_image_samplers();
  for (uint32_t cis_idx = 0u; cis_idx < cis_array.size(); ++cis_idx) {
    const spirv_cross::CombinedImageSampler& cis = cis_array[cis_idx];
    spv_cross_compiler_->set_name(
        cis.combined_id,
        spv_cross_compiler_->get_name(cis.image_id) + "_" +
            spv_cross_compiler_->get_name(cis.sampler_id));
    spv_cross_compiler_->set_decoration(cis.combined_id, spv::DecorationBinding, cis_idx);
    spv_cross_compiler_->set_decoration(
        cis.combined_id,
        spv::DecorationDescriptorSet,
        AUTOGEN_CIS_DESCRIPTOR_SET);
  }
}

void compilation::add_cis_to_map(
    separate_to_combined_builder& image_map,
    separate_to_combined_builder& sampler_map) const {
  for (const spirv_cross::CombinedImageSampler& cis :
       spv_cross_compiler_->get_combined_image_samplers()) {
    image_map.add_resource(cis.image_id, cis.combined_id, *spv_cross_compiler_);
    sampler_map.add_resource(cis.sampler_id, cis.combined_id, *spv_cross_compiler_);
  }
}

void compilation::add_resources_to_pipeline_layout(pipeline_layout_builder& builder) const {
  const stage_mask_bit smb =
      stage_ == pipeline_stage::vertex ? STAGE_MASK_VERTEX : STAGE_MASK_FRAGMENT;
  auto process_resources = [this, smb, &builder](
                               const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
                               descriptor_type                                        dtype) {
    builder.process_resources(resources, dtype, smb, *spv_cross_compiler_);
  };

  spirv_cross::ShaderResources resources = spv_cross_compiler_->get_shader_resources();

  process_resources(resources.uniform_buffers, descriptor_type::UNIFORM_BUFFER);
  process_resources(resources.storage_buffers, descriptor_type::STORAGE_BUFFER);
  process_resources(resources.separate_samplers, descriptor_type::SAMPLER);
  process_resources(resources.separate_images, descriptor_type::TEXTURE);
}

value_or_error<compilation_result> compilation::run(const pipeline_layout& layout) {
  try {
    return (target_info_.api != target_api::VULKAN)
               ? compilation_result {spv_cross_compiler_->compile() + layout.native_binding_map_string()}
               : compilation_result {original_spirv_};
  } catch (spirv_cross::CompilerError& ce) { NICESHADE_RETURN_ERROR(ce.what()); }
}

}  // namespace libniceshade
