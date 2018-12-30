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
#pragma warning(push)
#pragma warning(disable:4200)

#include <stdint.h>

#if defined(__cplusplus)
extern "C" {
#endif

typedef struct plmd plmd;

#define PLMD_DESC_UNIFORM_BUFFER         (0x00)
#define PLMD_DESC_STORAGE_BUFFER         (0x01)
#define PLMD_DESC_LOADSTORE_IMAGE        (0x02)
#define PLMD_DESC_IMAGE                  (0x03)
#define PLMD_DESC_SAMPLER                (0x04)
#define PLMD_DESC_COMBINED_IMAGE_SAMPLER (0x05)

#define PLMD_STAGE_VISIBILITY_VERTEX_BIT   (0x01)
#define PLMD_STAGE_VISIBILITY_FRAGMENT_BIT (0x02)

/**
 * Pipeline metadata header.
 */
typedef struct plmd_header {
  uint32_t magic_number; /**< must always be 0xdeadbeef */
  uint32_t header_size; /**< size of the metadata header in bytes. */
  uint32_t version_maj; /**< major version of the format in use. */
  uint32_t version_min; /**< minor version of the format in use. */

  /**
   * Offset, in bytes, from the beginning of the file, at which the
   * PIPELINE_LAYOUT record is stored.
   */
  uint32_t pipeline_layout_offset;

  /**
   * Offset, in bytes, from the beginning of the file, at which a 
   * SEPARATE_TO_COMBINED_MAP record is stored, which maps separate image
   * bindings to the corresponding auto-generated combined image/sampler
   * bindings.
   */
  uint32_t image_to_cis_map_offset;

  /**
   * Offset, in bytes, from the beginning of the file, at which a
   * SEPARATE_TO_COMBINED_MAP record is stored, which map separate sampler
   * bindings to the corresponding auto-generated combined image/sampler
   * bindings.
   */
  uint32_t sampler_to_cis_map_offset;

  /**
   * Offset, in bytes, from the beginning of the file, at which the
   * USER_METADATA record is stored.
   */
  uint32_t user_metadata_offset;
} plmd_header;

/**
 * Information about a descriptor.
 */
typedef struct plmd_descriptor {
  uint32_t binding; /**< Binding within the set. */
  uint32_t type; /**< Type of the descriptor (PLMD_DESC_...) */
  uint32_t stage_visibility_mask; /**< Mask indicating which shader stages the
                                       descriptor is used from. */
} plmd_descriptor;

/**
 * Information about a descriptor set.
 */
typedef struct plmd_descriptor_set_layout {
  uint32_t ndescriptors; /**< Number of descriptors in the set. */
  plmd_descriptor descriptors[];
} plmd_descriptor_set_layout;

/**
 * Information about a pipeline layout.
 */
typedef struct plmd_layout {
  uint32_t ndescriptor_sets; /**< Number of descriptor sets.*/
  const plmd_descriptor_set_layout **set_layouts;
} plmd_layout;

/**
 * A separate-to-combined map entry.
 */
typedef struct plmd_cis_map_entry {
  /**
   * Descriptor set of the separate image or sampler object.
   */
  uint32_t separate_set_id;
  /**
   * Binding of the separate image or sampler object.
   */
  uint32_t separate_binding_id;
  /**
   * Number of combined image/samplers that use this separate image or sampler.
   */
  uint32_t ncombined_ids;
  uint32_t combined_ids[]; /**< IDs of the combined image/samplers. */
} plmd_cis_map_entry;

/**
 * Some platforms do not have full separation between textures and samplers.
 * For example, in OpenGL, in order to sample a texture with a particular
 * sampler, both the texture and the sampler need to be bound to the same
 * texture unit by the CPU-side code. This is in contrast to HLSL, which allows
 * specifying the sampler to use directly from the shader code.
 *
 * To address this discrepancy, each unique texture-sampler pair used by the 
 * source HLSL generates a "synthetic" combined texture/sampler in the output.
 * Each separate texture and sampler is then mapped to a set of auto-generated 
 * combined texture/samplers that it is used in.
 */
typedef struct plmd_cis_map {
  uint32_t nentries; /**< Number of entries in the map. */
  const plmd_cis_map_entry **entries;
} plmd_cis_map;

/**
 * A user-provided metadata entry.
 */
typedef struct plmd_user_entry {
  const char *key;
  const char *value;
} plmd_user_entry;

/**
 * User-provided metadata.
 */
typedef struct plmd_user {
  uint32_t nentries; /**< Number of entries. */
  plmd_user_entry *entries;
} plmd_user;

typedef enum plmd_error {
  PLMD_ERROR_OK,
  PLMD_ERROR_OUTOFMEM,
  PLMD_ERROR_MAGIC_NUMBER_MISMATCH,
  PLMD_ERROR_BUFFER_TOO_SMALL,
  PLMD_ERROR_WEIRD_BUFFER_SIZE
} plmd_error;

typedef struct plmd_alloc_callbacks {
  void* (*alloc)(size_t);
  void  (*free)(void*);
} plmd_alloc_callbacks;

plmd_error plmd_load(const void *buf, size_t buf_size,
                             const plmd_alloc_callbacks *alloc_cb,
                             plmd **result);
void plmd_destroy(plmd *m, const plmd_alloc_callbacks *alloc_cb);
const plmd_layout* plmd_get_layout(const plmd *m);
const plmd_cis_map* plmd_get_image_to_cis_map(const plmd *m);
const plmd_cis_map* plmd_get_sampler_to_cis_map(const plmd *m);
const plmd_user* plmd_get_user(const plmd *m);
const plmd_header* plmd_get_header(const plmd *m);

#if defined(__cplusplus)
}
#endif

#pragma warning(pop)
