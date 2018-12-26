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

#include "file_utils.h"

#include <stdlib.h>
#include <stdio.h>
#if defined(_WIN32) || defined(_WIN64)
  #include <io.h>
  #if defined(_WIN64)
  #define filelen(f) _filelengthi64(_fileno(f))
  #elif defined(_WIN32)
  #define filelen(f) _filelength(_fileno(f))
  #endif
#else
  #include <sys/stat.h>
  size_t filelen(FILE *f) {
    struct stat statbuf;
    fstat(fileno(f), &statbuf);
    return statbuf.st_size;
  }
#endif

// Reads the contents of a file into an std::string.
std::string read_file(const char *path) {
  FILE *input_file = fopen(path, "rb");
  if (input_file == nullptr) {
    fprintf(stderr, "Failed to open file %s\n", path);
    exit(1);
  }
  size_t len = filelen(input_file);
  std::string contents;
  contents.reserve(len + 1u);
  contents.resize(len);
  size_t read_bytes = fread(&contents[0], 1u, len, input_file);
  if (read_bytes != len) {
    fprintf(stderr, "Failed to read file %s\n", path);
    exit(1);
  }
  fclose(input_file);
  return contents;
}
