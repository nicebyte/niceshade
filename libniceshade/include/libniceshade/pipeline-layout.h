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

#pragma once

#include <stdint.h>
#include <string>
#include <vector>
#include <map>

/**
 * @file
 * @brief
 */

namespace niceshade {

/** Enumerates the types of resources that can be accessed by a programmable shader stage. */
enum class descriptor_type {
  UNIFORM_BUFFER      = 0, // NGF_PLMD_DESC_UNIFORM_BUFFER,
  STORAGE_BUFFER      = 1, // NGF_PLMD_DESC_STORAGE_BUFFER,
  LOADSTORE_IMAGE     = 2, // NGF_PLMD_DESC_LOADSTORE_IMAGE,
  TEXTURE             = 3, // NGF_PLMD_DESC_IMAGE,
  SAMPLER             = 4, // NGF_PLMD_DESC_SAMPLER,
  TEXTURE_AND_SAMPLER = 5, // NGF_PLMD_DESC_COMBINED_IMAGE_SAMPLER,
  INVALID             = 6
};

/**
 * Enumerates bits corresponding ti programmable shader stages.
 * 
 * The values of individual enumerants can be combined together using the logical OR operation to
 * form a bitmask indicating which stages of the pipeline a particular resource is visible/used
 * from.
 */
enum stage_mask_bit {
  STAGE_MASK_VERTEX   = 1, // NGF_PLMD_STAGE_VISIBILITY_VERTEX_BIT
  STAGE_MASK_FRAGMENT = 2, // NGF_PLMD_STAGE_VISIBILITY_FRAGMENT_BIT
  STAGE_MASK_COMPUTE  = 4, // NGF_PLMD_STAGE_VISIBILITY_COMPUTE_BIT
};

/**
 * Encapsulates data associated with a resource descriptor.
 */
struct descriptor {
  uint32_t        slot;                                   /**< A descriptor's binding within its set.*/
  descriptor_type type       = descriptor_type::INVALID;  /**< Type of resorce accessed. */
  uint32_t        stage_mask = 0u;  /**< A bitmask indicating the pipeline stages that the descriptor is used from. */
  std::string     name;             /**< The name used to refer to the descriptor in the shader's source code. */
  uint32_t        native_binding;   /**< The actual binding used by the target API (if a remapping from descriptor/set model is needed). */
  bool            is_array;         /**< Set to true if the descriptor pertains to an array (e.g. array of textures). */
  uint32_t        array_size;       /**< If the descriptor pertains to an array, this holds the array size. If the array size is not known at
                                         compile time (e.g. it is specified by a specialization constant), this will be set to 0xffffffff. */
};

/**
 * Specifies the layout of a descriptor set. The key is the descriptor index, the value is
 * descriptor-related data.
 */
using descriptor_set_layout = std::map<uint32_t, descriptor>;

/**
 * Stores information about all shader resources accessed by a technique.
 */
class pipeline_layout {
  friend class pipeline_layout_builder;
public:
  /**
   * Iterator for the contents for the pipeline layout. Points to a pair of descriptor set index
   * and descriptor set layout.
   */
  using iterator = std::map<uint32_t, descriptor_set_layout>::const_iterator;

  /** @return The total number of descriptor sets in the layout. */
  uint32_t set_count() const noexcept { return max_set_ + 1; }

  /** @return The total number of descriptors across all descriptor sets in the layout. */
  uint32_t res_count() const noexcept { return nres_; }

  /**
   * @param set_id The id of the descriptor set to retrieve the layour for.
   * @return The layout of the specified descriptor set.
   */
  const descriptor_set_layout& set(uint32_t set_id) const noexcept {
    static const descriptor_set_layout empty_layout {};
    auto it = sets_.find(set_id);
    return (it != sets_.cend()) ? it->second : empty_layout;
  }

  /** @return an interator pointing to the beginning of the pipeline layout. */
  iterator begin() const noexcept { return sets_.begin(); }

  /** @return end iterator of the pipeline layout. */
  iterator end() const noexcept { return sets_.end(); }

private:
  std::map<uint32_t, descriptor_set_layout> sets_;   // shouldn't be unordered_map to guarantee consistent order.
  uint32_t                           max_set_ = 0u;  // Max set number encountered.
  uint32_t                           nres_    = 0u;  // Total number of resources.
};

}  // namespace niceshade