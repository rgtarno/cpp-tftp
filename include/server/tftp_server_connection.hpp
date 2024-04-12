#pragma once

#include <arpa/inet.h>
#include <memory>
#include <optional>
#include <string>

#include <spdlog/logger.h>

#include "common/tftp.hpp"
#include "common/tftp_read_file.hpp"
#include "common/tftp_write_file.hpp"
#include "common/timer.hpp"
#include "common/udp_connection.hpp"

class tftp_server_connection
{
public:
  tftp_server_connection(const tftp::rw_packet_t &request, struct sockaddr_in &client_address);
  ~tftp_server_connection();
  tftp_server_connection()                               = delete;
  tftp_server_connection(const tftp_server_connection &) = delete;
  tftp_server_connection(tftp_server_connection &&)      = delete;
  tftp_server_connection &operator=(const tftp_server_connection &) = delete;
  tftp_server_connection &operator=(tftp_server_connection &&) = delete;

  int  sd() const;
  int  timer_fd() const;
  void handle_read();
  void handle_write();

  void set_finished(const bool finished);
  bool is_finished() const;
  bool wait_for_read() const;
  bool wait_for_write() const;

  const struct sockaddr_in &client() const;

  enum class state_t
  {
    SEND_ACK,
    SEND_OACK,
    SEND_DATA,
    WAIT_FOR_ACK,
    WAIT_FOR_DATA,
    ERROR
  };

  static std::string state_to_string(const state_t state);

private:
  std::shared_ptr<spdlog::logger> _logger;
  udp_connection                  _udp;
  const tftp::packet_t            _type;
  tftp_read_file                  _file_reader;
  tftp_write_file                 _file_writer;
  struct sockaddr_in              _client;
  std::string                     _client_str;
  tftp::data_packet_t             _data_pkt;
  tftp::error_packet_t            _error_pkt;
  bool                            _finished;
  bool                            _final_ack;
  bool                            _pkt_ready;
  state_t                         _state;
  uint8_t                         _timeout_s;
  uint8_t                         _timeout_count;
  uint16_t                        _block_number;
  size_t                          _block_size;
  tftp::oack_packet_t             _oack_packet;
  timer                           _timer;

  void                                process_options(const tftp::rw_packet_t &request);
  void                                retransmit();
  std::optional<tftp::error_packet_t> is_operation_allowed(const std::string   &file_request,
                                                           const tftp::packet_t type) const;
};