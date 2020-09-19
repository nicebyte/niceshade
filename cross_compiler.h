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

#pragma once

#include "spirv_cross.hpp"
#include "target.h"
#include "pipeline_layout.h"
#include "technique_parser.h"
#include "separate_to_combined_map.h"

#include <memory>
#include <vector>
#include <stdint.h>
#include <string>

class cross_compiler {
public:
  cross_compiler(shader_kind kind,
                 const std::vector<uint32_t> &spirv_code,
                 const target_info &target_info);

  void add_resources_to_pipeline_layout(pipeline_layout &layout) const;
  void add_cis_to_map(separate_to_combined_map &image_map,
                      separate_to_combined_map &sampler_map) const;
  void compile(const std::string &out_file_path);
  shader_kind kind() const { return kind_; }

private:
  target_info target_info_;
  shader_kind kind_;
  std::unique_ptr<spirv_cross::Compiler> spv_cross_compiler_;
  const std::vector<uint32_t> &original_spirv_;
};