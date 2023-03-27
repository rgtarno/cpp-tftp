#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <vector>

namespace tftp
{

  static const size_t DATA_PKT_MAX_SIZE      = 516;
  static const size_t DATA_PKT_DATA_MAX_SIZE = 512;
  static const size_t ACK_PKT_MAX_SIZE       = 4;

  enum class packet_t : uint8_t
  {
    READ = 1,
    WRITE,
    DATA,
    ACK,
    ERROR,
    OACK
  };

  enum class mode_t
  {
    NETASCII,
    OCTET
  };

  enum class error_t : uint16_t
  {
    NOT_DEFINED,
    FILE_NOT_FOUND,
    ACCESS_ERROR,
    DISK_FULL,
    ILLEGAL_OPERATION,
    UNKNOWN_TID,
    FILE_EXISTS,
    NO_USER
  };

  std::string error_code_to_string(const error_t err);

  struct rw_packet_t
  {
    rw_packet_t() : type(packet_t::READ), filename(""), mode(mode_t::OCTET), options{} {};
    rw_packet_t(const std::string &f, const packet_t t, const mode_t m) : type(t), filename(f), mode(m), options{} {};
    packet_t                                         type;
    std::string                                      filename;
    mode_t                                           mode;
    std::vector<std::pair<std::string, std::string>> options;
  };

  struct oack_packet_t
  {
    oack_packet_t() : options{} {};
    std::vector<std::pair<std::string, std::string>> options;
  };

  struct data_packet_t
  {
    data_packet_t() : data{}, block_number(0){};
    std::vector<char> data;
    uint16_t          block_number;
  };

  struct ack_packet_t
  {
    explicit ack_packet_t(const uint16_t bn) : block_number(bn){};
    uint16_t block_number;
  };

  struct error_packet_t
  {
    error_packet_t() : error_msg(""), error_code(0){};
    error_packet_t(const error_t code, const std::string &msg) :
        error_msg(msg), error_code(static_cast<uint16_t>(code)){};
    std::string error_msg;
    uint16_t    error_code;
  };

  std::vector<char> serialise_rw_packet(const rw_packet_t &packet);
  std::vector<char> serialise_data_packet(const data_packet_t &packet);
  std::vector<char> serialise_ack_packet(const ack_packet_t &packet);
  std::vector<char> serialise_error_packet(const error_packet_t &packet);
  std::vector<char> serialise_oack_packet(const oack_packet_t &packet);

  std::optional<rw_packet_t>    deserialise_rw_packet(const std::vector<char> &data);
  std::optional<data_packet_t>  deserialise_data_packet(const std::vector<char> &data);
  std::optional<ack_packet_t>   deserialise_ack_packet(const std::vector<char> &data);
  std::optional<error_packet_t> deserialise_error_packet(const std::vector<char> &data);
  std::optional<oack_packet_t>  deserialise_oack_packet(const std::vector<char> &data);

  std::optional<mode_t> string_to_mode_t(std::string mode_str);
  std::string           mode_t_to_string(const mode_t mode);

} // namespace tftp
