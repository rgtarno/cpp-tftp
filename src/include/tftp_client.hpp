#pragma once

#include <string>

#include "tftp.hpp"
#include "udp_connection.hpp"

class tftp_client
{
public:
  explicit tftp_client(const std::string &tftp_server, const tftp::mode_t mode = tftp::mode_t::OCTET,
                       const std::string &local_interface = "");

  void send_file(const std::string &filename);
  void get_file(const std::string &filename);

private:
  std::string    _server_address;
  tftp::mode_t   _transfer_mode;
  udp_connection _udp;
};