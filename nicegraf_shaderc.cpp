/**
Copyright © 2018 nicegraf contributors
Permission is hereby granted, free of charge, to any person obtaining a copy of
this software and associated documentation files (the “Software”), to deal in
the Software without restriction, including without limitation the rights to
use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
the Software, and to permit persons to whom the Software is furnished to do so,
subject to the following conditions:
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
#include "shaderc/shaderc.hpp"
#include "spirv_glsl.hpp"
#include "spirv_msl.hpp"
#include "spirv_reflect.hpp"
#include <algorithm>
#include <assert.h>
#include <regex>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unordered_map>
#include <unordered_set>

// Platform-specific stuff.
#if defined(_WIN32) || defined(_WIN64)

#pragma comment(lib, "ws2_32.lib")
#include <winsock2.h>
#include <io.h>
#define PATH_SEPARATOR  "\\"
#if defined(_WIN32)
#define filelen(f) _filelength(_fileno(f))
#elif defined(_WIN64)
#define filelen(f) _filelength64(_fileno(f))
#endif  // _WIN32

#else

#include <arpa/inet.h>
#define PATH_SEPARATOR "/"
#include <sys/stat.h>
size_t filelen(FILE *f) {
  struct stat statbuf;
  fstat(fileno(f), &statbuf);
  return statbuf.st_size;
}

#endif

const char *USAGE = R"(
Usage: ngf_shaderc [options]

Compiles GLSL shaders for multiple different targets.

Options:

  -f <filename> - Specifies an input file name to be processed. Shader stage is
    determined from the file name extension.
      .vert.glsl - corrseponds to vertex shader; 
      .frag.glsl - fragment shader;
      .geom.glsl - geometry shader,
      .tese.glsl - tess evaluation shader;
      .tesc.glsl - tess control shader

  -o <filename> - Name (excluding parent folder and extension) for the output
    file. By default the name of the input file is used.

  -D <name>=<value> - If coming after an `-f` option, specifies an additional
    definition to add when processing the corresponding file.  If coming before
    any of the `-f` options, the definition will be added for all files.

  -t <target> - Generate shader for the given target.  Accepted values are:
      gl430
      gles310
      gles300
      msl10
      msl11
      msl12
      msl20
      msl10ios
      msl11ios
      msl12ios
      msl20ios
      spv 
    If specified multiple times, shaders for all of the mentioned targets will
    be generated.

  -O <path> - Folder to store output files in. Default is the current working
    directory.
)";

enum class descriptor_type_code {
  UNIFORM_BUFFER = 0x00,
  STORAGE_BUFFER = 0x01,
  LOADSTORE_IMAGE = 0x02,
  TEXTURE = 0x03,
  SAMPLER = 0x04,
  TEXTURE_AND_SAMPLER = 0x05,
};

using descriptor_set_layout = std::vector<std::pair<size_t, descriptor_type_code>>;

class resource_layout {
public:
  void add_resource(const spirv_cross::Resource &r,
                    const spirv_cross::CompilerReflection &refl,
                    descriptor_type_code type);

  uint32_t set_count() const { return max_set_ + 1; }
  uint32_t res_count() const { return nres_; }
  const descriptor_set_layout& set(uint32_t set) const;

private:
  std::unordered_map<uint32_t, descriptor_set_layout> set_map_;
  uint32_t max_set_ = 0u;
  uint32_t nres_ = 0u;
};

struct input_item {
  std::string input_file_name;
  std::string output_file_name;
  std::string input_file_basename;
  std::string input_file_extension;
  std::unordered_map<std::string, std::string> defines;
  shaderc_shader_kind kind;
  shaderc::SpvCompilationResult spirv;
  resource_layout layout;
  uint32_t spirv_len = 0u;
};

enum target_type {
  TARGET_GL_430 = 0,
  TARGET_GLES_300,
  TARGET_GLES_310,
  TARGET_MSL_10,
  TARGET_MSL_11,
  TARGET_MSL_12,
  TARGET_MSL_20,
  TARGET_MSL_10_IOS,
  TARGET_MSL_11_IOS,
  TARGET_MSL_12_IOS,
  TARGET_MSL_20_IOS,
  TARGET_SPIRV,
  TARGET_COUNT
};

enum class target_platform_class {
  DONTCARE,
  DESKTOP,
  MOBILE
};

struct target_info {
  const char *name_string;
  uint32_t version_maj;
  uint32_t version_min;
  target_platform_class platform;
};

const target_info TARGET_INFOS[TARGET_COUNT] = {
  {
    "gl430",
    4u, 3u,
    target_platform_class::DESKTOP
  },
  {
    "gles300",
    3u, 0u,
    target_platform_class::MOBILE
  },
  {
    "gles310",
    3u, 1u,
    target_platform_class::MOBILE,
  },
  {
    "msl10",
    1u, 0u,
    target_platform_class::DESKTOP
  },
  {
    "msl11",
    1u, 1u,
    target_platform_class::DESKTOP
  },
  {
    "msl12",
    1u, 2u,
    target_platform_class::DESKTOP
  },
  {
    "msl20",
    2u, 0u,
    target_platform_class::DESKTOP
  },
  {
    "msl10ios",
    1u, 0u,
    target_platform_class::MOBILE
  },
  {
    "msl11ios",
    1u, 1u,
    target_platform_class::MOBILE
  },
  {
    "msl12ios",
    1u, 2u,
    target_platform_class::MOBILE
  },
  {
    "msl20ios",
    2u, 0u,
    target_platform_class::MOBILE
  },
  {
    "spv",
    0u, 0u,
    target_platform_class::DONTCARE
  }
};

void add_defines_from_map(const std::unordered_map<std::string, std::string> &m,
                          shaderc::CompileOptions &opts);

std::string generate_output_name(const input_item &input,
                                 const std::string &out_folder,
                                 const std::string &suffix);

void write_network_word(uint32_t word, FILE *f);

class file_cache {
public:
  const std::string& get_contents(const std::string &file_name);
private:
  std::unordered_map<std::string, std::string> cache_;
};

class includer: public shaderc::CompileOptions::IncluderInterface {
public:
  explicit includer(file_cache *file_cache) :
    file_cache_(file_cache) {}

  shaderc_include_result* GetInclude(const char *file_name,
                                     shaderc_include_type,
                                     const char *, size_t) override;

  void ReleaseInclude(shaderc_include_result *data) override;

private:
  file_cache *file_cache_;
};

int main (int argc, char *argv[]) {
  if (argc <= 1) {
    printf("%s\n", USAGE);
    exit(0);
  }
  
  std::vector<input_item> inputs;
  std::unordered_map<std::string, std::string> global_defines;
  std::unordered_set<target_type> targets;
  std::string out_folder = ".";

  // Parse options.
  for (uint32_t o = 1u; o < (uint32_t)argc; o += 2u) {
    std::string option_name = argv[o];
    if (o + 1u >= (uint32_t)argc) {
      fprintf(stderr, "Expected option value after %s\n", argv[o]);
      exit(1);
    }
    std::string option_value = argv[o + 1u];
    if ("-f" == option_name) { // Input file name.
      static const std::regex file_name_regex(
        "^(.*\\" PATH_SEPARATOR ")?([^\\" PATH_SEPARATOR 
        "]+).(vert|frag|tesc|tese|geom).glsl$");
      std::smatch match_result;
      bool matched = std::regex_match(option_value,
                                      match_result,
                                      file_name_regex);
      if (!matched) {
        fprintf(stderr, "Incorrectly formatted file name \"%s\"\n",
                option_value.c_str());
        exit(1);
      }
      std::string extension = match_result[3].str();
      std::string basename = match_result[2].str();
      shaderc_shader_kind kind;
      if (extension == "vert") {
        kind = shaderc_vertex_shader;
      } else if (extension == "frag") {
        kind = shaderc_fragment_shader;
      } else if (extension == "geom") {
        kind = shaderc_geometry_shader;
      } else if (extension == "tese") {
        kind = shaderc_tess_evaluation_shader;
      } else if (extension == "tesc") {
        kind = shaderc_tess_control_shader;
      } else {
        fprintf(stderr,
                "Could not parse shader type from extension \".%s.glsl\"\n",
                extension.c_str());
        exit(1);
      }
      inputs.emplace_back();
      inputs.back().input_file_name = option_value;
      inputs.back().kind = kind;
      inputs.back().input_file_extension = extension;
      inputs.back().input_file_basename = basename;
    } else if ("-o" == option_name) { // Output file name.
      if (inputs.empty()) {
        fprintf(stderr, "Undexpected -o option\n");
        exit(1);
      }
      inputs.back().output_file_name = option_value;
    } else if ("-D" == option_name) { // #define
      size_t eq_idx = option_value.find_first_of('=');
      std::string name = option_value.substr(0, eq_idx);
      std::string value =
          eq_idx != std::string::npos && eq_idx < option_value.size() - 1u
              ? option_value.substr(eq_idx + 1u) : "";
      if (inputs.empty()) global_defines[name] = value;
      else inputs.back().defines[name] = value;
    } else if ("-t" == option_name) { // Target to generate code for.
      if (option_value == "gl430") targets.insert(TARGET_GL_430);
      else if (option_value == "gles300") targets.insert(TARGET_GLES_300);
      else if (option_value == "gles310") targets.insert(TARGET_GLES_310);
      else if (option_value == "spirv") targets.insert(TARGET_SPIRV);
      else if (option_value == "msl10") targets.insert(TARGET_MSL_10);
      else if (option_value == "msl11") targets.insert(TARGET_MSL_11);
      else if (option_value == "msl12") targets.insert(TARGET_MSL_12);
      else if (option_value == "msl12") targets.insert(TARGET_MSL_20);
      else if (option_value == "msl10ios") targets.insert(TARGET_MSL_10_IOS);
      else if (option_value == "msl11ios") targets.insert(TARGET_MSL_11_IOS);
      else if (option_value == "msl12ios") targets.insert(TARGET_MSL_12_IOS);
      else if (option_value == "msl12ios") targets.insert(TARGET_MSL_20_IOS);
      else {
        fprintf(stderr, "Unknown target \"%s\"\n", option_value.c_str());
        exit(1);
      }
    } else if ("-O" == option_name) { // Output folder.
      out_folder = option_value;
    } else {
      fprintf(stderr, "Unknown option: \"%s\"\n", option_name.c_str());
      exit(1);
    }
  }

  // Compile all inputs and collect metadata about resources.
  shaderc::Compiler spirv_compiler;
  file_cache file_cache;
  bool compilation_successful = true;
  for (auto &i : inputs) {
    shaderc::CompileOptions shaderc_opts;
    add_defines_from_map(global_defines, shaderc_opts);
    add_defines_from_map(i.defines, shaderc_opts);
    shaderc_opts.SetAutoBindUniforms(true);
    shaderc_opts.SetAutoMapLocations(true);
    shaderc_opts.SetIncluder(std::make_unique<includer>(&file_cache));
    const std::string &glsl_source = file_cache.get_contents(i.input_file_name);
    const shaderc_shader_kind kind = i.kind;
    i.spirv = spirv_compiler.CompileGlslToSpv(glsl_source.c_str(),
                                              glsl_source.length(),
                                              kind,
                                              i.input_file_name.c_str(),
                                              shaderc_opts);
    shaderc_compilation_status compile_status = i.spirv.GetCompilationStatus();
    if (compile_status != shaderc_compilation_status_success) {
      fprintf(stderr, "Error when compiling %s:\n %s\n",
              i.input_file_name.c_str(),
              i.spirv.GetErrorMessage().c_str());
      compilation_successful = false;
      continue;
    }
    i.spirv_len = i.spirv.cend() - i.spirv.cbegin();
    spirv_cross::CompilerReflection refl(i.spirv.cbegin(), i.spirv_len);
    spirv_cross::ShaderResources resources = refl.get_shader_resources();
    for (const spirv_cross::Resource &ub : resources.uniform_buffers)
      i.layout.add_resource(ub, refl, descriptor_type_code::UNIFORM_BUFFER);
    for (const spirv_cross::Resource &sb: resources.storage_buffers)
      i.layout.add_resource(sb, refl, descriptor_type_code::STORAGE_BUFFER);
    for (const spirv_cross::Resource &si: resources.sampled_images)
      i.layout.add_resource(si, refl, descriptor_type_code::TEXTURE_AND_SAMPLER);
    for (const spirv_cross::Resource &s: resources.separate_samplers)
      i.layout.add_resource(s, refl, descriptor_type_code::SAMPLER);
    for (const spirv_cross::Resource &im: resources.separate_images)
      i.layout.add_resource(im, refl, descriptor_type_code::TEXTURE);
  }
  if (!compilation_successful) {
    fprintf(stderr,
            "Some shaders failed to compile. "
            "Output files have not been modified.\n");
    exit(1);
  }

  // Write output files.
  for (const auto &i : inputs) {
    // Write layout file.
    std::string layout_file_name = generate_output_name(i, out_folder, "rlo");
    FILE *layout_file = fopen(layout_file_name.c_str(), "wb");
    if (layout_file == nullptr) {
      fprintf(stderr, "Failed to open output file %s\n",
              layout_file_name.c_str());
      exit(1);
    }
    uint32_t nsets = i.layout.set_count();
    write_network_word(nsets, layout_file);
    uint32_t nres = i.layout.res_count();
    write_network_word(nres, layout_file);
    for (uint32_t s = 0u; s < nsets; ++s) {
      const descriptor_set_layout &set_layout = i.layout.set(s);
      if (set_layout.size() > 0u) {
        for (const auto &d : set_layout) {
          write_network_word(s, layout_file);
          write_network_word(static_cast<uint32_t>(d.second), layout_file);
          write_network_word(d.first, layout_file);
        }
      }
    }
    fclose(layout_file);

    // Write each target flavor.
    for (const target_type f : targets) {
      const target_info &ti = TARGET_INFOS[f];
      std::string out;
      std::string out_filename = generate_output_name(i, out_folder,
                                                      ti.name_string);
      switch(f) {
      case TARGET_GLES_300:
      case TARGET_GLES_310:
      case TARGET_GL_430: {
        spirv_cross::CompilerGLSL spv_cross(i.spirv.cbegin(), i.spirv_len);
        spirv_cross::CompilerGLSL::Options opts;
        opts.version = ti.version_maj * 100u + ti.version_min * 10u;
        opts.separate_shader_objects = true;
        opts.es = (ti.platform == target_platform_class::MOBILE);
        spv_cross.set_common_options(opts);
        out = spv_cross.compile();
        break;
      }
      case TARGET_SPIRV: break;
      case TARGET_MSL_10:
      case TARGET_MSL_11:
      case TARGET_MSL_12:
      case TARGET_MSL_20:
      case TARGET_MSL_10_IOS:
      case TARGET_MSL_11_IOS:
      case TARGET_MSL_12_IOS:
      case TARGET_MSL_20_IOS: {
        spirv_cross::CompilerMSL spv_cross(i.spirv.cbegin(), i.spirv_len);
        spirv_cross::CompilerMSL::Options opts;
        opts.set_msl_version(ti.version_maj, ti.version_min);
        const bool ios = ti.platform == target_platform_class::MOBILE;
        opts.platform = ios ? spirv_cross::CompilerMSL::Options::iOS
                            : spirv_cross::CompilerMSL::Options::macOS;
        spv_cross.set_msl_options(opts);
        out = spv_cross.compile();
        break;
      }
      default:
        fprintf(stderr, "Not implemented yet\n");
      }
      FILE *out_file = fopen(out_filename.c_str(), "w");
      if (out_file == nullptr) {
        fprintf(stderr, "Failed to open output file %s\n",
                out_filename.c_str());
        exit(1);
      }
      if (f != TARGET_SPIRV) {
        fwrite(&out[0], sizeof(uint8_t), out.length(), out_file);
      } else {
        fwrite(i.spirv.cbegin(), sizeof(uint32_t), i.spirv_len, out_file);
      }
      fclose(out_file);
    }
  }

  return 0;
}

void add_defines_from_map(const std::unordered_map<std::string, std::string> &m,
                          shaderc::CompileOptions &opts) {
  for (const auto &d : m) {
    if (!d.second.empty()) {
      opts.AddMacroDefinition(d.first.c_str(), d.first.length(),
                              d.second.c_str(), d.second.length());
    } else {
      opts.AddMacroDefinition(d.first);
    }
  }
}

const std::string& file_cache::get_contents(const std::string &file_name) {
  auto it = cache_.find(file_name);
  if (it == cache_.end()) {
    FILE *f = fopen(file_name.c_str(), "r");
    if (f == nullptr) {
      fprintf(stderr, "Failed to open file %s\n", file_name.c_str());
      exit(1);
    }
    size_t len = filelen(f);
    std::string contents(len, '\0');
    fread(&contents[0], 1u, len, f);
    it = cache_.insert(std::make_pair(file_name, std::move(contents))).first;
  } 
  return it->second;
}

shaderc_include_result* includer::GetInclude(const char *file_name,
                                             shaderc_include_type, const char *,
                                             size_t) {
  const std::string &content = file_cache_->get_contents(file_name);
  auto result = new shaderc_include_result;
  result->source_name = nullptr;
  result->source_name_length = 0u;
  result->content = content.c_str();
  result->content_length = content.length();
  result->user_data = nullptr;
  return result;
}

void includer::ReleaseInclude(shaderc_include_result *data) { delete data; }

std::string generate_output_name(const input_item &input,
                                const std::string &out_folder, 
                                const std::string &suffix) {
  const std::string &basename =
      input.output_file_name.empty()
          ? input.input_file_basename
          : input.output_file_name;
  return out_folder + PATH_SEPARATOR + basename + "." + suffix + "." +
         input.input_file_extension;
}

void write_network_word(uint32_t word, FILE *f) {
  uint32_t nbo = htonl(word);
  fwrite(&nbo, sizeof(uint32_t), 1u, f);
}

void resource_layout::add_resource(const spirv_cross::Resource &r,
                                   const spirv_cross::CompilerReflection &refl,
                                   descriptor_type_code type) {
  uint32_t set = refl.get_decoration(r.id, spv::DecorationDescriptorSet);
	uint32_t binding = refl.get_decoration(r.id, spv::DecorationBinding);
  max_set_ = max_set_ < set ? set : max_set_;
  set_map_[set].emplace_back(binding, type);
  nres_++;
}

const descriptor_set_layout& resource_layout::set(uint32_t set) const {
  static const descriptor_set_layout empty_layout;
  auto it = set_map_.find(set);
  if (it != set_map_.end()) return it->second;
  else return empty_layout;
}

