#pragma once

#include <arpa/inet.h>
#include <queue>
#include <string>
#include <vector>

#include "tftp.hpp"
#include "udp_connection.hpp"

class tftp_connection_handler
{
public:
  explicit tftp_connection_handler(const std::string &addr = "", const uint16_t port = 0);

  struct request_t
  {
    request_t(const tftp::rw_packet_t &req, const sockaddr_in cl) :
        request(req), client(cl){};
    tftp::rw_packet_t request;
    sockaddr_in       client;
  };

  int       sd() const;
  void      handle_read();
  bool      requests_pending() const;
  request_t get_request();

private:
  udp_connection        _udp;
  std::queue<request_t> _request_queue;
};