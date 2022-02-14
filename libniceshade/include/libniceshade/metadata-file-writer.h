/**
 * Copyright (c) 2022 nicegraf contributors
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

#include "metadata-parser/metadata-parser.h"

#include <stdint.h>
#include <stdio.h>

namespace libniceshade {

// Convenience class for generating pipeline metadata in binary format.
class metadata_file_writer {
public:
  // Open a new pipeline metadata file for writing.
  explicit metadata_file_writer(const char* file_path);

  // Begin a new record.
  void start_new_record();

  // Write a field into the current record.
  void write_field(uint32_t value);

  // Write the contents of the given byte buffer into the file.
  void write_raw_bytes(const void* bytes, size_t nbytes);

  // Finalize writing and close the file.
  void finalize();

private:
  FILE*           f_;
  ngf_plmd_header header_;
  uint32_t*       current_section_offset_ptr_;
  uint32_t        current_offset_ = sizeof(ngf_plmd_header);
};

}  // namespace libniceshade