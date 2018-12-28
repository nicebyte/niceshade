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

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct ngf_meta ngf_meta;

#define NGF_META_DESC_UNIFORM_BUFFER         (0x00)
#define NGF_META_DESC_STORAGE_BUFFER         (0x01)
#define NGF_META_DESC_LOADSTORE_IMAGE        (0x02)
#define NGF_META_DESC_IMAGE                  (0x03)
#define NGF_META_DESC_SAMPLER                (0x04)
#define NGF_META_DESC_COMBINED_IMAGE_SAMPLER (0x05)

#define NGF_META_STAGE_VISIBILITY_VERTEX_BIT   (0x01)
#define NGF_META_STAGE_VISIBILITY_FRAGMENT_BIT (0x02)

typedef struct ngf_meta_header {
  uint32_t magic_number;
  uint32_t header_size;
  uint32_t version_maj;
  uint32_t version_min;
  uint32_t pipeline_layout_offset;
  uint32_t image_to_cis_map_offset;
  uint32_t sampler_to_cis_map_offset;
  uint32_t user_metadata_offset;
} ngf_meta_header;

typedef struct ngf_meta_desc {
  uint32_t binding;
  uint32_t type;
  uint32_t stage_visibility_mask; 
} ngf_meta_desc;

typedef struct ngf_meta_desc_set_layout {
  uint32_t ndescriptors;
  ngf_meta_desc descriptors[];
} ngf_meta_desc_set_layout;

typedef struct ngf_meta_layout {
  uint32_t ndescriptor_sets;
  const ngf_meta_desc_set_layout **set_layouts;
} ngf_meta_layout;

typedef struct ngf_meta_cis_map_entry {
  uint32_t separate_set_id;
  uint32_t separate_binding_id;
  uint32_t ncombined_ids;
  uint32_t combined_ids[];
} ngf_meta_cis_map_entry;

typedef struct ngf_meta_cis_map {
  uint32_t nentries;
  const ngf_meta_cis_map_entry **entries;
} ngf_meta_cis_map;

typedef struct ngf_meta_user_entry {
  const char *key;
  const char *value;
} ngf_meta_user_entry;

typedef struct ngf_meta_user {
  uint32_t nentries;
  ngf_meta_user_entry *entries;
} ngf_meta_user;

typedef enum ngf_meta_error {
  NGF_META_ERROR_OK,
  NGF_META_ERROR_OUTOFMEM,
  NGF_META_ERROR_MAGIC_NUMBER_MISMATCH,
  NGF_META_ERROR_BUFFER_TOO_SMALL,
  NGF_META_ERROR_WEIRD_BUFFER_SIZE
} ngf_meta_error;

typedef struct ngf_meta_alloc_callbacks {
  void* (*alloc)(size_t);
  void  (*free)(void*);
} ngf_meta_alloc_callbacks;

ngf_meta_error ngf_meta_load(const void *buf, size_t buf_size,
                             const ngf_meta_alloc_callbacks *alloc_cb,
                             ngf_meta **result);
void ngf_meta_destroy(ngf_meta *m, const ngf_meta_alloc_callbacks *alloc_cb);
const ngf_meta_layout* ngf_meta_get_layout(const ngf_meta *m);
const ngf_meta_cis_map* ngf_meta_get_image_to_cis_map(const ngf_meta *m);
const ngf_meta_cis_map* ngf_meta_get_sampler_to_cis_map(const ngf_meta *m);
const ngf_meta_user* ngf_meta_get_user(const ngf_meta *m);
const ngf_meta_header* ngf_meta_get_header(const ngf_meta *m);

#if defined(__cplusplus)
}
#endif
