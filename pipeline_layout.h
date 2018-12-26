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

#pragma once

#include "linear_dict.h"
#include "spirv_reflect.hpp"
#include <stdlib.h>
#include <stdint.h>
#include <stdio.h>
#include <string>
#include <vector>

// Indicates the type of resource accessed by a programmable shader stage.
enum class descriptor_type {
  UNIFORM_BUFFER = 0x00,
  STORAGE_BUFFER = 0x01,
  LOADSTORE_IMAGE = 0x02,
  TEXTURE = 0x03,
  SAMPLER = 0x04,
  TEXTURE_AND_SAMPLER = 0x05,
  INVALID = 0x06
};

// Indicates which programmable shader stage a descriptor is visible from.
enum stage_mask_bit {
  STAGE_MASK_VERTEX = 0x01,
  STAGE_MASK_FRAGMENT = 0x10
};

// Descriptor data.
struct descriptor {
  uint32_t slot; // A descriptor's binding within its set.
  descriptor_type type = descriptor_type::INVALID; // Type of resorce accessed.
  uint32_t stage_mask = 0u; // Which stages the descriptor is used from.
  std::string name; // The name used to refer to it in the source code.
};

using descriptor_set_layout = linear_dict<uint32_t, descriptor>;

// Stores information about shader resources accessed by a technique.
class pipeline_layout {
public:
  void add_resources(const std::vector<spirv_cross::Resource> &resources,
                     const spirv_cross::Compiler &refl,
                     descriptor_type resource_type,
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

  uint32_t set_count() const { return max_set_ + 1; }
  uint32_t res_count() const { return nres_; }

  const descriptor_set_layout& pipeline_layout::set(uint32_t set_id) const {
    static const descriptor_set_layout empty_layout;
    auto it = sets_.find(set_id);
    if (it != sets_.cend()) {
      return it->second.layout;
    } else {
      return empty_layout;
    }
  }

private:
  struct descriptor_set {
    uint32_t slot = 0u;
    descriptor_set_layout layout;
  };
  linear_dict<uint32_t, descriptor_set> sets_;
  uint32_t max_set_ = 0u;
  uint32_t nres_ = 0u;
};