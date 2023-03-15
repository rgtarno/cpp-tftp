#pragma once

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <arpa/inet.h>
#include <fmt/core.h>
#include <optional>
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

  std::vector<char> native_to_netascii(const std::vector<char> &data);
  std::vector<char> netascii_to_native(const std::vector<char> &data);

  std::optional<sockaddr_in> to_sockaddr_in(const std::string &addr, const uint16_t port);
}; // namespace utils

std::ostream &operator<<(std::ostream &os, const struct sockaddr_in &sa);
