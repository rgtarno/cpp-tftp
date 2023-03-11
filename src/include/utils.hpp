#pragma once

#include <fmt/core.h>
#include <string.h>

#include <string>

namespace utils
{

  inline std::string string_error(const int errnum)
  {
    char buf[256];
    if (strerror_r(errnum, buf, 256) == 0)
    {
      return std::string(buf);
    }
    return fmt::format("Unknwon error : {}", errnum);
  }

}; // namespace utils