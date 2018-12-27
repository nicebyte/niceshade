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
#include <stdint.h>
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
  // Adds descriptors of a given type to the pipeline layout.
  void add_resources(const std::vector<spirv_cross::Resource> &resources,
                     const spirv_cross::Compiler &refl,
                     descriptor_type resource_type,
                     stage_mask_bit smb);

  // Returns the total number of descriptor sets in the layout.
  uint32_t set_count() const { return max_set_ + 1; }

  // Returns the total number of descriptors across all descriptor sets in the
  // layout.
  uint32_t res_count() const { return nres_; }

  // Returns the layout of the n-th descriptor set.
  const descriptor_set_layout& set(uint32_t set_id) const;

private:
  struct descriptor_set {
    uint32_t slot = 0u;
    descriptor_set_layout layout;
  };
  linear_dict<uint32_t, descriptor_set> sets_;
  uint32_t max_set_ = 0u;
  uint32_t nres_ = 0u;
};