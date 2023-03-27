#pragma once

#include <arpa/inet.h>
#include <optional>
#include <string>

#include "tftp.hpp"
#include "tftp_read_file.hpp"
#include "tftp_write_file.hpp"
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
    SEND_OACK,
    SEND_DATA,
    WAIT_FOR_ACK,
    WAIT_FOR_DATA,
    ERROR
  };

private:
  udp_connection       _udp;
  const tftp::packet_t _type;
  tftp_read_file       _file_reader;
  tftp_write_file      _file_writer;
  struct sockaddr_in   _client;
  tftp::data_packet_t  _data_pkt;
  tftp::error_packet_t _error_pkt;
  bool                 _finished;
  bool                 _final_ack;
  bool                 _pkt_ready;
  state_t              _state;
  uint16_t             _block_number;
  size_t               _block_size;
  tftp::oack_packet_t  _oack_packet;

  void process_options(const std::vector<std::pair<std::string, std::string>> &options);

  std::optional<tftp::error_packet_t> is_operation_allowed(const std::string   &file_request,
                                                           const tftp::packet_t type) const;
};