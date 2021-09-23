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


#include "pipeline_layout.h"
#include <stdlib.h>
#include <stdio.h>

namespace {

bool should_process_resource(uint32_t id,
                             const spirv_cross::Compiler& compiler) {
  return compiler.get_decoration(id,
                                 spv::DecorationDescriptorSet) != AUTOGEN_CIS_DESCRIPTOR_SET &&
         compiler.get_name(id) != "SPIRV_Cross_DummySampler" &&
         compiler.get_active_interface_variables().count(id) > 0u;
}

}

void pipeline_layout::process_resources(
    const spirv_cross::SmallVector<spirv_cross::Resource> &resources,
    descriptor_type resource_type,
    stage_mask_bit smb,
    spirv_cross::Compiler &refl) {
  for (const auto &r : resources) {
    if (!should_process_resource(r.id, refl)) { continue; }
    uint32_t set_id =
        refl.get_decoration(r.id, spv::DecorationDescriptorSet);
    uint32_t binding_id = refl.get_decoration(r.id, spv::DecorationBinding);
    max_set_ = max_set_ < set_id ? set_id : max_set_;
    descriptor_set &set = sets_[set_id];
    set.slot = set_id;
    descriptor &desc = set.layout[binding_id];
    if (desc.type == descriptor_type::INVALID) {
      // This resource hasn't been encountered before.
      desc.slot = binding_id;
      desc.type = resource_type;
      desc.name = r.name;
      nres_++;
    }
    if (desc.type != descriptor_type::INVALID &&
        desc.type != resource_type) {
      fprintf(stderr, "Attempt to assign a descriptor of different type to "
                      "slot %d in set %d which is already occupied by "
                      "\"%s\"\n", binding_id, set_id, desc.name.c_str());
      exit(1);
    }
    if (desc.type != descriptor_type::INVALID &&
        r.name != desc.name) {
      fprintf(stderr, "Assigning different names "
                      "(\"%s\" and \"%s\")  to descriptor at slot %d in set "
                      "%d.\n", desc.name.c_str(), r.name.c_str(),
                      binding_id, set_id);
      exit(1);
    }
    desc.stage_mask |= smb;
    desc.usages.emplace_back(&refl, r.id);
  }
}
void pipeline_layout::dump_native_binding_map(FILE* f) const{
  fprintf(f, "/**NGF_NATIVE_BINDING_MAP\n");
  for (const auto &set_id_and_layout : sets_) {
    for (const auto &binding_id_and_descriptor : set_id_and_layout.second.layout) {
      fprintf(f, "(%d %d) : %d\n",
              set_id_and_layout.first,
              binding_id_and_descriptor.first,
              binding_id_and_descriptor.second.native_binding);
    }
  }
  fprintf(f, "(-1 -1) : -1\n");
  fprintf(f, "**/\n");
}

void pipeline_layout::remap_resources() {
  uint32_t num_descriptors_of_type[NGF_PLMD_DESC_NUM_TYPES] = {0u};
  for (auto &set_id_and_layout : sets_) {
    for (auto &binding_id_and_descriptor : set_id_and_layout.second.layout) {
      const uint32_t native_binding = (num_descriptors_of_type[(int)binding_id_and_descriptor.second.type])++;
      binding_id_and_descriptor.second.native_binding = native_binding;
      for (auto& compiler_and_id : binding_id_and_descriptor.second.usages) {
        compiler_and_id.first->set_decoration(compiler_and_id.second, spv::DecorationBinding, native_binding);
      }
    }
  }
}

const descriptor_set_layout& pipeline_layout::set(uint32_t set_id) const {
  static const descriptor_set_layout empty_layout;
  auto it = sets_.find(set_id);
  if (it != sets_.cend()) {
    return it->second.layout;
  } else {
    return empty_layout;
  }
}
