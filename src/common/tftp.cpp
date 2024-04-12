
#include "common/tftp.hpp"

#include <algorithm>
#include <cstring>

#include "common/utils.hpp"

namespace
{
  const char OCTET_MODE_STR[]    = "OCTET";
  const char NETASCII_MODE_STR[] = "NETASCII";
}; // namespace

//========================================================
std::vector<char> tftp::serialise_rw_packet(const rw_packet_t &packet)
{
  const auto   mode_str = tftp::mode_t_to_string(packet.mode);
  const size_t packet_size =
      2 + packet.filename.size() + 1 + mode_str.size() + 1; // opcode + filename + null byte + mode + null_byte
  std::vector<char> ret;
  ret.reserve(packet_size);
  ret.push_back(0);
  ret.push_back(static_cast<char>(packet.type));
  ret.insert(ret.end(), packet.filename.begin(), packet.filename.end());
  ret.push_back(0);
  ret.insert(ret.end(), mode_str.begin(), mode_str.end());
  ret.push_back(0);
  return ret;
}

//========================================================
std::optional<tftp::rw_packet_t> tftp::deserialise_rw_packet(const std::vector<char> &data)
{
  rw_packet_t packet;
  if ((data.size() < 4) ||
      ((static_cast<packet_t>(data.at(1)) != packet_t::READ) && (static_cast<packet_t>(data.at(1)) != packet_t::WRITE)))
  {
    return {};
  }
  packet.type = static_cast<packet_t>(data.at(1));

  std::vector<std::string> params = utils::extract_c_strings_from_buffer(data, 2);
  if (params.size() < 2)
  {
    return {};
  }
  packet.filename = std::move(params[0]);
  const auto mode = tftp::string_to_mode_t(params[1]);
  if (!mode)
  {
    return {};
  }
  packet.mode = mode.value();

  if ((params.size() % 2) != 0)
  {
    // Ignore anything else, options should be in pairs of name & value
    return packet;
  }

  for (size_t i = 2; i < params.size(); i += 2)
  {
    packet.options.push_back(std::make_pair(std::move(params.at(i)), std::move(params.at(i + 1))));
    std::transform(packet.options.back().first.begin(), packet.options.back().first.end(),
                   packet.options.back().first.begin(), ::toupper);
  }
  return packet;
}

//========================================================
std::vector<char> tftp::serialise_data_packet(const data_packet_t &packet)
{
  const size_t      packet_size = 2 + 2 + packet.data.size(); // opcode + block_number + data
  std::vector<char> ret;
  ret.reserve(packet_size);
  ret.push_back(0);
  ret.push_back(static_cast<char>(packet_t::DATA));
  ret.push_back(packet.block_number >> 8);
  ret.push_back(packet.block_number & 0xFF);
  ret.insert(ret.end(), packet.data.begin(), packet.data.end());
  return ret;
}
//========================================================
std::optional<tftp::data_packet_t> tftp::deserialise_data_packet(const std::vector<char> &data)
{
  data_packet_t packet;
  if ((data.size() < 4) || (static_cast<packet_t>(data.at(1)) != packet_t::DATA))
  {
    return {};
  }

  packet.block_number =
      (static_cast<uint16_t>((unsigned char)data.at(2)) << 8) | static_cast<uint16_t>((unsigned char)data.at(3));
  packet.data.assign(data.begin() + 4, data.end());
  return packet;
}

//========================================================
std::vector<char> tftp::serialise_ack_packet(const ack_packet_t &packet)
{
  const size_t      packet_size = 2 + 2; // opcode + block_number
  std::vector<char> ret;
  ret.reserve(packet_size);
  ret.push_back(0);
  ret.push_back(static_cast<char>(packet_t::ACK));
  ret.push_back(packet.block_number >> 8);
  ret.push_back(packet.block_number & 0xFF);
  return ret;
}

//========================================================
std::optional<tftp::ack_packet_t> tftp::deserialise_ack_packet(const std::vector<char> &data)
{
  if ((data.size() < 4) || (static_cast<packet_t>(data.at(1)) != packet_t::ACK))
  {
    return {};
  }
  const uint16_t block_num =
      (static_cast<uint16_t>((unsigned char)data.at(2)) << 8) | static_cast<uint16_t>((unsigned char)data.at(3));
  return ack_packet_t(block_num);
}

//========================================================
std::vector<char> tftp::serialise_error_packet(const error_packet_t &packet)
{
  const size_t      packet_size = 2 + 2 + packet.error_msg.size() + 1; // opcode + erron_num + error_msg + null byte
  std::vector<char> ret;
  ret.reserve(packet_size);
  ret.push_back(0);
  ret.push_back(static_cast<char>(packet_t::ERROR));
  ret.push_back(packet.error_code >> 8);
  ret.push_back(packet.error_code & 0xFF);
  ret.insert(ret.end(), packet.error_msg.begin(), packet.error_msg.end());
  ret.push_back(0);
  return ret;
}

//========================================================
std::optional<tftp::error_packet_t> tftp::deserialise_error_packet(const std::vector<char> &data)
{
  error_packet_t packet;
  if ((data.size() < 5) || (static_cast<packet_t>(data.at(1)) != packet_t::ERROR))
  {
    return {};
  }

  packet.error_code = (static_cast<uint16_t>(data.at(2)) << 8) | static_cast<uint16_t>(data.at(3));

  const auto null = std::find(data.begin() + 4, data.end(), 0);
  if (null == data.end())
  {
    return {};
  }
  packet.error_msg = std::string(data.begin() + 4, null);
  return packet;
}

//========================================================
std::vector<char> tftp::serialise_oack_packet(const tftp::oack_packet_t &packet)
{
  const size_t packet_size = [&]() {
    size_t size = 2; // opcode
    for (const auto &param : packet.options)
    {
      size += param.first.size() + 1 + param.second.size() + 1;
    }
    return size;
  }();

  std::vector<char> ret;
  ret.reserve(packet_size);
  ret.push_back(0);
  ret.push_back(static_cast<char>(packet_t::OACK));

  for (const auto &param : packet.options)
  {
    ret.insert(ret.end(), param.first.begin(), param.first.end());
    ret.push_back(0);
    ret.insert(ret.end(), param.second.begin(), param.second.end());
    ret.push_back(0);
  }
  return ret;
}

//========================================================
std::optional<tftp::oack_packet_t> tftp::deserialise_oack_packet(const std::vector<char> &data)
{
  if ((data.size() < 2) || (static_cast<packet_t>(data.at(1)) != packet_t::OACK))
  {
    return {};
  }

  oack_packet_t            packet;
  std::vector<std::string> params = utils::extract_c_strings_from_buffer(data, 2);

  if ((params.size() % 2) != 0)
  {
    // Ignore anything else, options should be in pairs of name & value
    return packet;
  }

  for (size_t i = 0; i < params.size(); i += 2)
  {
    packet.options.push_back(std::make_pair(std::move(params.at(i)), std::move(params.at(i + 1))));
  }
  return packet;
}

//========================================================
std::optional<tftp::mode_t> tftp::string_to_mode_t(std::string mode_str)
{
  std::transform(mode_str.begin(), mode_str.end(), mode_str.begin(), ::toupper);

  if (std::strcmp(mode_str.c_str(), OCTET_MODE_STR) == 0)
  {
    return mode_t::OCTET;
  }
  else if (std::strcmp(mode_str.c_str(), NETASCII_MODE_STR) == 0)
  {
    return mode_t::NETASCII;
  }

  return {};
}

//========================================================
std::string tftp::mode_t_to_string(const tftp::mode_t mode)
{
  switch (mode)
  {
  case mode_t::OCTET: {
    return std::string(OCTET_MODE_STR);
  }
  case mode_t::NETASCII: {
    return std::string(NETASCII_MODE_STR);
  }
  default: {
    return std::string("UNKNOWN");
  }
  }
}

//========================================================
std::string tftp::error_code_to_string(const tftp::error_t err)
{
  switch (err)
  {
  default:
  case error_t::NOT_DEFINED: {
    return std::string("Not defined");
  }
  case error_t::FILE_NOT_FOUND: {
    return std::string("File not found");
  }
  case error_t::ACCESS_ERROR: {
    return std::string("Access violation");
  }
  case error_t::DISK_FULL: {
    return std::string("Disk full or allocation exceeded");
  }
  case error_t::ILLEGAL_OPERATION: {
    return std::string("Illegal TFTP operation");
  }
  case error_t::UNKNOWN_TID: {
    return std::string("Unknown transfer ID");
  }
  case error_t::FILE_EXISTS: {
    return std::string("File already exists");
  }
  case error_t::NO_USER: {
    return std::string("No such user");
  }
  }
  return std::string("");
}
