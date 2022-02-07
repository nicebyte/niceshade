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

#include "libniceshade/metadata-file-writer.h"
#include "libniceshade/platform.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

namespace libniceshade {

metadata_file_writer::metadata_file_writer(const char* file_path) {
  f_ = fopen(file_path, "wb");
  if (f_ == nullptr) {
    fprintf(stderr, "Error opening output file %s\n", file_path);
    exit(1);
  }
  fwrite(&header_, sizeof(header_), 1u, f_);  // write placeholder header.
  current_section_offset_ptr_ = &header_.entrypoints_offset;
}

void metadata_file_writer::start_new_record() {
  assert(f_);
  *current_section_offset_ptr_ = htonl(current_offset_);
  current_section_offset_ptr_ += 1u;
}

void metadata_file_writer::write_field(uint32_t value) {
  assert(f_);
  uint32_t nbo = htonl(value);
  fwrite(&nbo, sizeof(uint32_t), 1u, f_);
  current_offset_ += 4u;
}

void metadata_file_writer::write_raw_bytes(const void* bytes, size_t nbytes) {
  size_t nwords_div = nbytes >> 2u;
  size_t nwords_mod = nbytes & 0b11;
  size_t nwords     = nwords_div + (nwords_mod > 0u ? 1u : 0u);
  write_field(0xffffffff);
  write_field((uint32_t)nwords);
  fwrite(bytes, 1u, nbytes, f_);
  if (nwords_mod > 0u) {
    uint32_t dummy = 0u;
    fwrite(&dummy, 1u, sizeof(uint32_t) - nwords_mod, f_);
  }
  current_offset_ += (uint32_t)(nwords * sizeof(uint32_t));
}

void metadata_file_writer::finalize() {
  header_.magic_number = htonl(0xdeadbeef);
  header_.header_size  = htonl(sizeof(header_));
  header_.version_maj  = htonl(0u);
  header_.version_min  = htonl(1u);
  fseek(f_, 0u, 0u);
  fwrite(&header_, sizeof(header_), 1u, f_);
  fclose(f_);
  f_ = nullptr;
}

}  // namespace libniceshade