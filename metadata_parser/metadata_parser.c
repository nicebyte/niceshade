#include "metadata_parser.h"
#include <assert.h>
#include <stdlib.h>
#if defined(_WIN32) || defined(_WIN64)
  #pragma comment(lib, "ws2_32.lib")
  #include <winsock2.h>
#else
  #include <arpa/inet.h>
#endif

static const ngf_meta_alloc_callbacks stdlib_alloc = {
  .alloc = malloc,
  .free = free
};

struct ngf_meta {
  const uint8_t *raw_data;
  const ngf_meta_header *header;
  ngf_meta_layout layout;
  const ngf_meta_cis_map *images_to_cis_map;
  const ngf_meta_cis_map *samplers_to_cis_map;
  const ngf_meta_user *user;
};


ngf_meta_error ngf_meta_load(const void *buf, size_t buf_size,
                             const ngf_meta_alloc_callbacks *alloc_cb,
                             ngf_meta **result) {
  assert(buf);
  assert(result);
  if (alloc_cb == NULL) {
    alloc_cb = &stdlib_alloc;
  }
  ngf_meta_error err = NGF_META_ERROR_OK;
  ngf_meta *meta = alloc_cb->alloc(sizeof(ngf_meta));
  *result =  meta;
  if (result == NULL) {
    err = NGF_META_ERROR_OUTOFMEM;
    goto ngf_meta_load_cleanup;
  }
  uint8_t *raw_data = alloc_cb->alloc(buf_size);
  if (raw_data == NULL) {
    err = NGF_META_ERROR_OUTOFMEM;
    goto ngf_meta_load_cleanup;
  }
  memcpy(raw_data, buf, buf_size);
  uint32_t *fields = (uint32_t*)raw_data;
  for (uint32_t f = 0u; f < buf_size >> 2u; ++f) {
    fields[f] = ntohl(fields[f]);
  }
  meta->raw_data = raw_data;
  meta->header = (const ngf_meta_header*)meta->raw_data;
  if (meta->header->magic_number != 0xdeadbeef) {
    err = NGF_META_ERROR_MAGIC_NUMBER_MISMATCH;
    goto ngf_meta_load_cleanup;
  }
  if (meta->header->pipeline_layout_offset >= buf_size) {
    err = NGF_META_ERROR_BUFFER_TOO_SMALL;
    goto ngf_meta_load_cleanup;
  }
  const uint32_t nsets =
      *(const uint32_t*)&meta->raw_data[meta->header->pipeline_layout_offset];
  meta->layout.ndescriptor_sets = nsets;
  meta->layout.set_layouts = alloc_cb->alloc(sizeof(void*) * nsets);
  if (meta->layout.set_layouts == NULL) {
    err = NGF_META_ERROR_OUTOFMEM;
    goto ngf_meta_load_cleanup;
  }
  const uint8_t *set_ptr =
      &meta->raw_data[meta->header->pipeline_layout_offset + 4u];
  for (uint32_t s = 0u; s < nsets; ++s) {
    meta->layout.set_layouts[s] = (const ngf_meta_desc_set_layout*)set_ptr;
    const uint32_t set_data_size =
        meta->layout.set_layouts[s]->ndescriptors * sizeof(ngf_meta_desc) + 4u;
    set_ptr += set_data_size;
  }
  meta->images_to_cis_map = (const ngf_meta_cis_map*)&meta->raw_data[meta->header->image_to_cis_map_offset];
  meta->samplers_to_cis_map = (const ngf_meta_cis_map*)&meta->raw_data[meta->header->sampler_to_cis_map_offset];
  meta->user = (const ngf_meta_user*)&meta->raw_data[meta->header->user_metadata_offset];

ngf_meta_load_cleanup:
  if (err != NGF_META_ERROR_OK) {
    ngf_meta_destroy(meta, alloc_cb);
  }
  return err;
}

void ngf_meta_destroy(ngf_meta *m, const ngf_meta_alloc_callbacks *alloc_cb) {
  assert(alloc_cb);
  if (m != NULL) {
    if (m->raw_data != NULL) {
      alloc_cb->free(m->raw_data);
    }
    if (m->layout.set_layouts != NULL) {
      alloc_cb->free(m->layout.set_layouts);
    }
    alloc_cb->free(m);
  }
}

const ngf_meta_layout* ngf_meta_get_layout(const ngf_meta *m) {
  return &m->layout;
}


const ngf_meta_cis_map* ngf_meta_get_image_to_cis_map(const ngf_meta *m) {
  return m->images_to_cis_map;
}

const ngf_meta_cis_map* ngf_meta_get_sampler_to_cis_map(const ngf_meta *m) {
  return m->samplers_to_cis_map;
}

const ngf_meta_user* ngf_meta_get_user(const ngf_meta *m) {
  return m->user;
}

const ngf_meta_header* ngf_meta_get_header(const ngf_meta *m) {
  return m->header;
}
