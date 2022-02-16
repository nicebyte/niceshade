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

#include  "libniceshade/target.h"

using namespace libniceshade;

struct named_target_info {
  const char *name;
  target_desc target;
};

// Map of string identifiers to a target-specific information.
static const struct named_target_info TARGET_MAP[] = {
  {
    "gl430",
    {
      target_api::GL,
      4u, 3u,
      target_platform_class::DESKTOP
    }
  },
  {
    "gles300",
    {
      target_api::GL,
      3u, 0u,
      target_platform_class::MOBILE
    }
  },
  {
    "gles310",
    {
      target_api::GL,
      3u, 1u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl10",
    {
      target_api::METAL,
      1u, 0u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl11",
    {
      target_api::METAL,
      1u, 1u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl12",
    {
      target_api::METAL,
      1u, 2u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl20",
    {
      target_api::METAL,
      2u, 0u,
      target_platform_class::DESKTOP
    }
  },
{
    "msl21",
    {
      target_api::METAL,
      2u, 1u,
      target_platform_class::DESKTOP
    }
  },
  {
    "msl10ios",
    {
      target_api::METAL,
      1u, 0u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl11ios",
    {
      target_api::METAL,
      1u, 1u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl12ios",
    {
      target_api::METAL,
      1u, 2u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl20ios",
    {
      target_api::METAL,
      2u, 0u,
      target_platform_class::MOBILE
    }
  },
  {
    "msl21ios",
    {
      target_api::METAL,
      2u, 1u,
      target_platform_class::MOBILE
    }
  },
  {
    "spv",
    {
      target_api::VULKAN,
      0u, 0u,
      target_platform_class::DONTCARE
    }
  }
};
constexpr uint32_t TARGET_COUNT = sizeof(TARGET_MAP)/sizeof(TARGET_MAP[0]);

