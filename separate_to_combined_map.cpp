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


#include "separate_to_combined_map.h"

void separate_to_combined_map::add_resource(uint32_t separate_id,
                                            uint32_t combined_id,
                                            const spirv_cross::Compiler 
                                                &compiler) {
  uint32_t set_id = compiler.get_decoration(separate_id,
                                            spv::DecorationDescriptorSet);
  uint32_t binding_id = compiler.get_decoration(separate_id,
                                                spv::DecorationBinding);
  uint32_t combined_binding_id =
      compiler.get_decoration(combined_id, spv::DecorationBinding);
  map_[set_and_binding{set_id, binding_id}][combined_binding_id] = true;
}

void separate_to_combined_map::serialize(
    pipeline_metadata_file &metadata_file) const {
  metadata_file.write_field((uint32_t)map_.size());
  for (const auto &entry : map_) {
    const set_and_binding &sb = entry.first;
    const linear_dict<uint32_t, bool> &combined_image_samplers =
        entry.second;
    metadata_file.write_field(sb.set);
    metadata_file.write_field(sb.binding);
    metadata_file.write_field((uint32_t)combined_image_samplers.size());
    for (const auto &c : combined_image_samplers) {
      metadata_file.write_field(c.first);
    }
  }
}