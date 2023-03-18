#pragma once

#include <string>

#include "tftp.hpp"
#include "udp_connection.hpp"

namespace tftp_client
{
  void send_file(const std::string &filename, const std::string &tftp_server,
                 const tftp::mode_t mode = tftp::mode_t::OCTET, const std::string &local_interface = "");
  void get_file(const std::string &filename, const std::string &tftp_server,
                const tftp::mode_t mode = tftp::mode_t::OCTET, const std::string &local_interface = "");

}; // namespace tftp_client
