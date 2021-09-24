/**
Copyright © 2020 nicegraf contributors
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

#include "metadata_parser/metadata_parser.h"
#include "spirv_reflect.hpp"
#include <stdint.h>
#include <string>
#include <vector>
#include <map>

// Indicates the type of resource accessed by a programmable shader stage.
enum class descriptor_type {
  UNIFORM_BUFFER = NGF_PLMD_DESC_UNIFORM_BUFFER,
  STORAGE_BUFFER = NGF_PLMD_DESC_STORAGE_BUFFER,
  LOADSTORE_IMAGE = NGF_PLMD_DESC_LOADSTORE_IMAGE,
  TEXTURE = NGF_PLMD_DESC_IMAGE,
  SAMPLER = NGF_PLMD_DESC_SAMPLER,
  TEXTURE_AND_SAMPLER = NGF_PLMD_DESC_COMBINED_IMAGE_SAMPLER,
  INVALID = 0xff
};

// Indicates which programmable shader stage a descriptor is visible from.
enum stage_mask_bit {
  STAGE_MASK_VERTEX = NGF_PLMD_STAGE_VISIBILITY_VERTEX_BIT,
  STAGE_MASK_FRAGMENT = NGF_PLMD_STAGE_VISIBILITY_FRAGMENT_BIT
};

// Descriptor data.
struct descriptor {
  uint32_t slot; // A descriptor's binding within its set.
  descriptor_type type = descriptor_type::INVALID; // Type of resorce accessed.
  uint32_t stage_mask = 0u; // Which stages the descriptor is used from.
  std::string name; // The name used to refer to it in the source code.
  uint32_t native_binding;
  std::vector<std::pair<spirv_cross::Compiler*, spirv_cross::ID>> usages;
};

using descriptor_set_layout = std::map<uint32_t, descriptor>;

// Stores information about shader resources accessed by a technique.
class pipeline_layout {
public:
  // Adds descriptors of a given type to the pipeline layout.
  // `resources' is a vector of spv-cross resources which are to be added.
  // `resource_type' indicates the type, and `smb' indicates which pipeline
  // stage the resources are used at.
  void process_resources(const spirv_cross::SmallVector<spirv_cross::Resource> &resources,
                         descriptor_type resource_type,
                         stage_mask_bit smb,
                         spirv_cross::Compiler &refl);

  // Returns the total number of descriptor sets in the layout.
  uint32_t set_count() const { return max_set_ + 1; }

  // Returns the total number of descriptors across all descriptor sets in the
  // layout.
  uint32_t res_count() const { return nres_; }

  // Returns the layout of the n-th descriptor set.
  const descriptor_set_layout& set(uint32_t set_id) const;


  // Map (set, binding) to a single binding, for targets that have no concept of
  // descriptor sets and use separate biniding spaces for each resource type
  // (i.e. OpenGL and Metal).
  void remap_resources();

  // Dumps out the (set, binding) => (native binding) map as a comment to
  // the output file.
  void dump_native_binding_map(FILE* f) const;

private:
  struct descriptor_set {
    uint32_t slot = 0u;
    descriptor_set_layout layout;
  };
  std::map<uint32_t, descriptor_set> sets_;
  uint32_t max_set_ = 0u; // Max set number encountered.
  uint32_t nres_ = 0u; // Total number of resources.
};

constexpr uint32_t AUTOGEN_CIS_DESCRIPTOR_SET = 9999u;