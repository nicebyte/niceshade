/**
 * Copyright (c) 2025 nicegraf contributors
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

#include "impl/pipeline-layout-builder.h"

#include "impl/error-macros.h"

namespace niceshade {

namespace {

bool should_process_resource(
    uint32_t                     id,
    const spirv_cross::Compiler& compiler,
    bool                         preserve_bindings) noexcept {
  return compiler.get_decoration(id, spv::DecorationDescriptorSet) != AUTOGEN_CIS_DESCRIPTOR_SET &&
         compiler.get_name(id) != "SPIRV_Cross_DummySampler" &&
         (preserve_bindings || compiler.get_active_interface_variables().count(id) > 0u);
}
}  // namespace

error pipeline_layout_builder::process_resources(
    const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
    descriptor_type                                        resource_type,
    stage_mask_bit                                         smb,
    spirv_cross::Compiler&                                 refl,
    bool                                                   preserve_bindings) noexcept {
  for (const auto& r : resources) {
    if (!should_process_resource(r.id, refl, preserve_bindings)) { continue; }
    uint32_t set_idx     = refl.get_decoration(r.id, spv::DecorationDescriptorSet);
    uint32_t binding_idx = refl.get_decoration(r.id, spv::DecorationBinding);
    max_set_             = max_set_ < set_idx ? set_idx : max_set_;
    descriptor& desc     = sets_[set_idx][binding_idx];
    if (desc.type == descriptor_type::INVALID) {
      // This resource hasn't been encountered before.
      desc.slot = binding_idx;
      desc.type = resource_type;
      desc.name = r.name;
      nres_++;
    }
    if (desc.type != descriptor_type::INVALID && desc.type != resource_type) {
      NICESHADE_RETURN_ERROR(
          "Attempt to assign a descriptor of different type to "
          "slot ",
          binding_idx,
          " in set ",
          set_idx,
          " which is already occupied by ",
          desc.name.c_str());
    }
    if (desc.type != descriptor_type::INVALID && r.name != desc.name) {
      NICESHADE_RETURN_ERROR(
          "Assigning different names "
          "(\"",
          desc.name.c_str(),
          "\" and \"",
          r.name.c_str(),
          "\")  to descriptor at slot ",
          binding_idx,
          " in set ",
          set_idx);
    }
    desc.stage_mask |= smb;
    const spirv_cross::SPIRType& r_type = refl.get_type(r.type_id);
    if (r_type.array.size() > 1u) { NICESHADE_RETURN_ERROR("Array of arrays in descriptors not supported."); }
    desc.is_array = r_type.array.size() > 0u;
    desc.array_size = r_type.array_size_literal[0] ? r_type.array[0] : (~0u);
    desc_usages_.add(set_idx, binding_idx, std::make_pair(&refl, r.id));
  }
  return error {};
}

bool pipeline_layout_builder::remap_resources() noexcept {
  uint32_t num_descriptors_of_type[(int)descriptor_type::INVALID] = {0u};
  for (auto& set_id_and_layout : sets_) {
    for (auto& binding_id_and_descriptor : set_id_and_layout.second) {
      const auto& desc = binding_id_and_descriptor.second;
      if (desc.is_array && desc.array_size == ~0u) {
        return false;
      }
      auto desc_type = desc.type;
      if (desc_type == descriptor_type::LOADSTORE_IMAGE) desc_type = descriptor_type::TEXTURE;
      if (desc_type == descriptor_type::STORAGE_BUFFER) desc_type = descriptor_type::UNIFORM_BUFFER;
      const uint32_t native_binding = (num_descriptors_of_type[(int)desc_type]);
      const uint32_t binding_shift  = !desc.is_array ? 1u : desc.array_size;
      num_descriptors_of_type[(int)desc_type] += binding_shift;
      binding_id_and_descriptor.second.native_binding = native_binding;
      for (auto& compiler_and_id :
           desc_usages_.get_usages(set_id_and_layout.first, binding_id_and_descriptor.first)) {
        compiler_and_id.first->set_decoration(
            compiler_and_id.second,
            spv::DecorationBinding,
            native_binding);
      }
    }
  }
  return true;
}

value_or_error<pipeline_layout> pipeline_layout_builder::build() noexcept {
  if (!remap_resources()) {
    NICESHADE_RETURN_ERROR(
        "Failed to remap resources -- array sizes must be known at compile time.");
  }

  pipeline_layout layout;
  layout.sets_    = std::move(sets_);
  layout.max_set_ = max_set_;
  layout.nres_    = nres_;
  sets_           = decltype(sets_) {};
  max_set_        = 0u;
  nres_           = 0u;
  desc_usages_.clear();

  return std::move(layout);
}

}  // namespace niceshade
