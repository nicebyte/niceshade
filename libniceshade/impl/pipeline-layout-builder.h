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

#include "libniceshade/error.h"
#include "libniceshade/pipeline-layout.h"
#include "spirv_reflect.hpp"

#include <map>
#include <vector>

namespace niceshade {

class pipeline_layout_builder {
public:
  error process_resources(
      const spirv_cross::SmallVector<spirv_cross::Resource>& resources,
      descriptor_type                                        resource_type,
      stage_mask_bit                                         smb,
      spirv_cross::Compiler&                                 refl,
      bool                                                   preserve_bindings) noexcept;

  value_or_error<pipeline_layout> build() noexcept;

private:
  void remap_resources() noexcept;

  using descriptor_usage = std::pair<spirv_cross::Compiler*, spirv_cross::ID>;
  class descriptor_usage_map {
  private:
    static constexpr uint64_t make_key(uint64_t a, uint64_t b) noexcept {
      return (a << (uint64_t)32) | b;
    }

  public:
    void add(uint32_t set, uint32_t binding, descriptor_usage usage) noexcept {
      descriptor_usages_[make_key(set, binding)].emplace_back(usage);
    }
    const std::vector<descriptor_usage>& get_usages(uint32_t set, uint32_t binding) noexcept {
      return descriptor_usages_[make_key(set, binding)];
    }

    void clear() noexcept { descriptor_usages_.clear(); }

  private:
    std::map<uint64_t, std::vector<descriptor_usage>> descriptor_usages_;
  };

  std::map<uint32_t, descriptor_set_layout>
                       sets_;  // shouldn't be undordered_map to guarantee consistent order.
  descriptor_usage_map desc_usages_;
  uint32_t             max_set_ = 0u;  // Max set number encountered.
  uint32_t             nres_    = 0u;  // Total number of resources.
};

constexpr uint32_t AUTOGEN_CIS_DESCRIPTOR_SET = 9999u;

}  // namespace niceshade
