/**
Copyright © 2018 nicegraf contributors
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
of the Software, and to permit persons to whom the Software is furnished to do
so, subject to the following conditions:
The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.
THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include "pipeline_layout.h"
#include <stdlib.h>
#include <stdio.h>

void pipeline_layout::add_resources(
    const std::vector<spirv_cross::Resource> &resources,
    const spirv_cross::Compiler &refl, descriptor_type resource_type,
    stage_mask_bit smb) {
  for (const auto &r : resources) {
    uint32_t set_id =
        refl.get_decoration(r.id, spv::DecorationDescriptorSet);
    uint32_t binding_id = refl.get_decoration(r.id, spv::DecorationBinding);
    max_set_ = max_set_ < set_id ? set_id : max_set_;
    descriptor_set &set = sets_[set_id];
    set.slot = set_id;
    descriptor &desc = set.layout[binding_id];
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
    desc.slot = binding_id;
    desc.type = resource_type;
    desc.stage_mask |= smb;
    desc.name = r.name;
    nres_++;
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
