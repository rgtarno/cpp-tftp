#pragma once

#include <spdlog/fmt/ostr.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <arpa/inet.h>
#include <filesystem>
#include <fmt/core.h>
#include <optional>
#include <string.h>
#include <string>
#include <sstream>
#include <vector>

/* formatter for struct sockaddr_in, required to be in the global namespace */
template<> class fmt::formatter<struct sockaddr_in>
{
  public:
    constexpr auto parse (format_parse_context& ctx) { return ctx.begin(); }
    template <typename Context>
    constexpr auto format (const struct sockaddr_in& sa, Context& ctx) const
    {
      char addr_buf[INET_ADDRSTRLEN] = {0};
      if (inet_ntop(AF_INET, &(sa.sin_addr), addr_buf, INET_ADDRSTRLEN) == NULL)
      {
        return format_to(ctx.out(), "unknown:{}", htons(sa.sin_port));
      }
      else
      {
        return format_to(ctx.out(), "{}:{}", addr_buf, htons(sa.sin_port));
      }
    }
};

std::ostream &operator<<(std::ostream &os, const struct sockaddr_in &sa);

namespace utils
{

  inline std::string string_error(const int errnum)
  {
    return std::string(std::strerror(errnum));
  }

  std::vector<char> native_to_netascii(const std::vector<char> &data);
  std::vector<char> netascii_to_native(const std::vector<char> &data);

  std::optional<struct sockaddr_in> to_sockaddr_in(const std::string &addr, const uint16_t port);

  bool is_subpath(const std::filesystem::path &path, const std::filesystem::path &base);

  std::vector<std::string> extract_c_strings_from_buffer(const std::vector<char> &buffer, const size_t offset = 0);

  int get_mtu(const int sd);

  size_t get_file_size(const char *fname);

  inline std::string sockaddr_to_str(const struct sockaddr_in &sa)
  {
    std::ostringstream oss;
    oss << sa;
    return oss.str();
  }

}; // namespace utils
