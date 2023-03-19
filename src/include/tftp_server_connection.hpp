#pragma once

#include <arpa/inet.h>
#include <optional>
#include <stdio.h>
#include <string>

#include "tftp.hpp"
#include "udp_connection.hpp"

class tftp_server_connection
{
public:
  tftp_server_connection(const tftp::rw_packet_t &request, struct sockaddr_in &client_address);
  ~tftp_server_connection();
  tftp_server_connection()                                          = delete;
  tftp_server_connection(const tftp_server_connection &)            = delete;
  tftp_server_connection(tftp_server_connection &&)                 = delete;
  tftp_server_connection &operator=(const tftp_server_connection &) = delete;
  tftp_server_connection &operator=(tftp_server_connection &&)      = delete;

  int  sd() const;
  void handle_read();
  void handle_write();

  bool is_finished() const;
  bool wait_for_read() const;
  bool wait_for_write() const;

  const struct sockaddr_in &client();

  enum class state_t
  {
    SEND_ACK,
    SEND_DATA,
    WAIT_FOR_ACK,
    WAIT_FOR_DATA,
    ERROR
  };

private:
  udp_connection       _udp;
  struct sockaddr_in   _client;
  tftp::data_packet_t  _data_pkt;
  tftp::error_packet_t _error_pkt;
  bool                 _finished;
  bool                 _pkt_ready;
  FILE                *_fd;
  state_t              _state;
  uint16_t             _block_number;

  std::optional<tftp::error_packet_t> is_operation_allowed(const std::string   &file_request,
                                                           const tftp::packet_t type) const;
};