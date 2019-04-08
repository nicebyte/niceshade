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
#define _CRT_SECURE_NO_WARNINGS
#include "metadata_parser/metadata_parser.h"
#include "file_utils.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

static const char *DESCRIPTOR_TYPE_NAMES[] = {
  "UNIFORM_BUFFER",
  "STORAGE_BUFFER",
  "LOADSTORE_IMAGE",
  "IMAGE",
  "SAMPLER",
  "COMBINED_IMAGE_SAMPLER"
};

void print_cis_map(const ngf_plmd_cis_map *m);

int main(int argc, const char *argv[]) {
  if (argc <= 1) {
    printf("Usage: display_metadata <file name>\n");
    exit(0);
  }
  const char *file_name = argv[1];
  std::string buf = read_file(file_name);
  ngf_plmd *m;
  ngf_plmd_error err = ngf_plmd_load(buf.data(), buf.size(), NULL, &m);
  if (err != NGF_PLMD_ERROR_OK) {
    fprintf(stderr, "Error loading pipeline metadata: %d\n", err);
    exit(1);
  }
  printf("{\n");
  printf("\"header\": {\n");
  const ngf_plmd_header *header = ngf_plmd_get_header(m);
  printf("  \"magic_number\": %u,\n", header->magic_number);
  printf("  \"header_size\": %d,\n", header->header_size);
  printf("  \"version_maj\": %d,\n", header->version_maj);
  printf("  \"version_min\": %d,\n", header->version_min);
  printf("  \"pipeline_layout_offset\": %d,\n", header->pipeline_layout_offset);
  printf("  \"image_to_cis_map_offset\": %d,\n",
         header->image_to_cis_map_offset);
  printf("  \"sampler_to_cis_map_offset\": %d,\n",
         header->sampler_to_cis_map_offset);
  printf("  \"user_metadata_offset\": %d\n},\n", header->user_metadata_offset);

  printf("\"pipeline_layout\": {\n");
  const ngf_plmd_layout *layout = ngf_plmd_get_layout(m);
  printf("  \"descriptor_sets\": [\n");
  for (uint32_t s = 0u; s < layout->ndescriptor_sets; ++s) {
    const ngf_plmd_descriptor_set_layout *dsl = layout->set_layouts[s];
    printf("    {\n");
    printf("      \"set\": %d,\n", s);
    printf("      \"descriptors\": [\n");
    for (uint32_t di = 0u; di < dsl->ndescriptors; ++di) {
      const ngf_plmd_descriptor *d = &(dsl->descriptors[di]);
      printf("        {\n");
      printf("          \"binding\": %d,\n", d->binding);
      printf("          \"type\": \"%s\",\n", DESCRIPTOR_TYPE_NAMES[d->type]);
      printf("          \"stage_vis\": %d\n", d->stage_visibility_mask);
      printf("        }");
      if (di != dsl->ndescriptors - 1u) printf(",\n");
      else printf("\n");
    }
    printf("      ]\n");
    if (s != layout->ndescriptor_sets - 1u) printf("    },\n");
    else printf("    }\n");
  }
  printf("  ]\n");
  printf("},\n");

  printf("\"image_to_cis_map\": {\n");
  print_cis_map(ngf_plmd_get_image_to_cis_map(m));
  printf("},\n");

  printf("\"sampler_to_cis_map\": {\n");
  print_cis_map(ngf_plmd_get_sampler_to_cis_map(m));
  printf("},\n");

  printf("\"user_metadata\": {\n");
  const ngf_plmd_user *user = ngf_plmd_get_user(m);
  for (uint32_t e = 0u; e < user->nentries; ++e) {
    printf("  \"%s\": \"%s\"", user->entries[e].key, user->entries[e].value);
    if (e != user->nentries - 1) printf(",");
    printf("\n");
  }
  printf("}\n}\n");
  ngf_plmd_destroy(m, NULL);
  return 0;
}

void print_cis_map(const ngf_plmd_cis_map *m) {
  printf("  \"entries\": [\n");
  for (uint32_t e = 0u; e < m->nentries; ++e) {
    const ngf_plmd_cis_map_entry *entry = m->entries[e];
    printf("    {\n");
    printf("      \"entry\": %d,\n", e);
    printf("      \"separate_set_id\": %d,\n", entry->separate_set_id);
    printf("      \"separate_binding_id\": %d,\n", entry->separate_binding_id);
    printf("      \"combined_ids\": [");
    for (uint32_t c = 0u; c < entry->ncombined_ids; ++c) {
      printf("%d%s", entry->combined_ids[c],
              c != entry->ncombined_ids - 1? ", " : "");
    }
    printf("]\n");
    printf("    }%s", e != m->nentries - 1? ",\n" : "\n");
  }
  printf("  ]\n");
}
