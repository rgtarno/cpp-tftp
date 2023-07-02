#pragma once

#include "spdlog/fmt/ostr.h"
#include "spdlog/sinks/stdout_color_sinks.h"
#include <arpa/inet.h>
#include <filesystem>
#include <fmt/core.h>
#include <optional>
#include <string.h>
#include <string>
#include <vector>

namespace utils
{

  inline std::string string_error(const int errnum)
  {
    return std::string(std::strerror(errnum));
  }

  std::vector<char> native_to_netascii(const std::vector<char> &data);
  std::vector<char> netascii_to_native(const std::vector<char> &data);

  std::optional<sockaddr_in> to_sockaddr_in(const std::string &addr, const uint16_t port);

  bool is_subpath(const std::filesystem::path &path, const std::filesystem::path &base);

  std::vector<std::string> extract_c_strings_from_buffer(const std::vector<char> &buffer, const size_t offset = 0);

  int get_mtu(const int sd);

  size_t get_file_size(const char *fname);

}; // namespace utils

std::ostream &operator<<(std::ostream &os, const struct sockaddr_in &sa);
