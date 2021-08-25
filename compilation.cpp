#define _CRT_SECURE_NO_WARNINGS

#include "compilation.h"

#include "spirv_glsl.hpp"
#include "spirv_msl.hpp"

compilation::compilation(shader_kind kind,
                         const std::vector<uint32_t>& spirv_code,
                         const target_info& target_info) : target_info_(target_info),
                                                           kind_(kind),
                                                           original_spirv_(spirv_code) {
  switch (target_info_.api) {
  case target_api::GL: {
    auto gl_compiler = std::make_unique<spirv_cross::CompilerGLSL>(
      spirv_code.data(), spirv_code.size());
    spirv_cross::CompilerGLSL::Options opts;
    opts.version = target_info.version_maj * 100u + target_info.version_min * 10u;
    opts.separate_shader_objects = true;
    opts.es = (target_info.platform == target_platform_class::MOBILE);
    gl_compiler->set_common_options(opts);
    gl_compiler->build_dummy_sampler_for_combined_images();
    gl_compiler->build_combined_image_samplers();
    spv_cross_compiler_ = std::move(gl_compiler);
    break;
  }
  case target_api::VULKAN: {
    spv_cross_compiler_ =
      std::make_unique<spirv_cross::CompilerReflection>(spirv_code.data(),
        spirv_code.size());
    break;
  }
  case target_api::METAL: {
    auto msl_compiler = std::make_unique<spirv_cross::CompilerMSL>(spirv_code.data(),
      spirv_code.size());
    spirv_cross::CompilerMSL::Options opts;
    opts.set_msl_version(target_info.version_maj, target_info.version_min);
    if (target_info.version_min >= 1 && target_info.version_maj >= 2) {
      // Enable native texel buffers on Metal 2.1+.
      opts.texture_buffer_native = true;
    }
    const bool ios = target_info.platform == target_platform_class::MOBILE;
    opts.platform = ios ? spirv_cross::CompilerMSL::Options::iOS
      : spirv_cross::CompilerMSL::Options::macOS;
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
    spv_cross_compiler_->set_decoration(cis.combined_id,
                                        spv::DecorationBinding,
                                        cis_idx);
    spv_cross_compiler_->set_decoration(cis.combined_id,
                                        spv::DecorationDescriptorSet,
                                        AUTOGEN_CIS_DESCRIPTOR_SET);
  }

}

void compilation::add_cis_to_map(separate_to_combined_map &image_map,
                                    separate_to_combined_map &sampler_map) const {
  for (const spirv_cross::CombinedImageSampler& cis :
       spv_cross_compiler_->get_combined_image_samplers()) {
    image_map.add_resource(cis.image_id, cis.combined_id, *spv_cross_compiler_);
    sampler_map.add_resource(cis.sampler_id, cis.combined_id, *spv_cross_compiler_);
  }
}

void compilation::add_resources_to_pipeline_layout(pipeline_layout& layout) const {
  const stage_mask_bit smb =
    kind_ == shader_kind::vertex
    ? STAGE_MASK_VERTEX
    : STAGE_MASK_FRAGMENT;
  auto process_resources =
    [this, smb, &layout](
      const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
      descriptor_type dtype) {
        layout.process_resources(resources, dtype, smb,
                                 *spv_cross_compiler_);
  };
 
  spirv_cross::ShaderResources resources =
    spv_cross_compiler_->get_shader_resources();

  process_resources(resources.uniform_buffers,
    descriptor_type::UNIFORM_BUFFER);
  process_resources(resources.storage_buffers,
    descriptor_type::STORAGE_BUFFER);
  process_resources(resources.separate_samplers,
    descriptor_type::SAMPLER);
  process_resources(resources.separate_images,
    descriptor_type::TEXTURE);
}

void compilation::run(const std::string &out_file_path, const pipeline_layout& layout) {
  const std::string full_out_file_path =
    out_file_path + 
    (kind_ == shader_kind::vertex ? ".vs." : ".ps.") +
    target_info_.file_ext;

  FILE* out_file =
      fopen(full_out_file_path.c_str(), "wb");

  if (out_file == nullptr) {
    fprintf(stderr,
            "Failed to open output file %s\n",
            out_file_path.c_str());
    exit(1);
  }

  std::string result;
  if (target_info_.api != target_api::VULKAN) {
    result = spv_cross_compiler_->compile();
    fwrite(&result[0], sizeof(uint8_t), result.length(), out_file);
    layout.dump_native_binding_map(out_file);
  } else {
    fwrite(original_spirv_.data(), sizeof(uint32_t),
           original_spirv_.size(), out_file);
  }
  fclose(out_file);
}

