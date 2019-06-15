/**
 * Copyright (c) 2019 nicegraf contributors
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

#include <stdio.h>
#include <string>
#include "file_utils.h"
#include "pipeline_layout.h"

class header_file_writer {
public:
  header_file_writer(const std::string &f,
                     const std::string &p)
                     : path_(f + PATH_SEPARATOR + p),
                       file_(p.empty()
                               ? NULL
                               : fopen(path_.c_str(), "wb")) {
    if (file_) fprintf(file_, "/*auto-generated, do not edit*/\n"
                              "#pragma once\n");
  
  }
  ~header_file_writer() { if(file_) fclose(file_); }

  void begin_technique(const std::string &name) {
    std::string ident = name;
    std::replace_if(ident.begin(), ident.end(),
                    [](char c) { return c == '-'; }, '_');
    if(file_) fprintf(file_, "namespace %s {\n", ident.c_str());
  }

  void end_technique() {
    if(file_) fprintf(file_, "}\n");
  }

  void write_descriptor(const descriptor &d, uint32_t set_id) {
    if(file_) {
      fprintf(file_,
              "  static constexpr int %s_Binding = %d;\n"
              "  static constexpr int %s_Set = %d;\n",
              d.name.c_str(), d.slot, d.name.c_str(), set_id);
    }
  }
  
  bool is_open() const { return file_ != NULL; }

  const char* path() const { return path_.c_str(); }

private:
  std::string path_;
  FILE *file_ = NULL;
};