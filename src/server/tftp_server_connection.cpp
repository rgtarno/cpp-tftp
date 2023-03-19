#include "tftp_server_connection.hpp"

#include <filesystem>
#include <netinet/in.h>

#include "debug_macros.hpp"
#include "utils.hpp"

//========================================================
tftp_server_connection::tftp_server_connection(const tftp::rw_packet_t &request, struct sockaddr_in &client_address) :
    _udp(),
    _client(client_address),
    _data_pkt(),
    _error_pkt(),
    _finished(false),
    _pkt_ready(false),
    _fd(NULL),
    _state(state_t::ERROR),
    _block_number(0)

{
  _udp.bind("", 0);
  _udp.connect(client_address);
  _udp.set_non_blocking(true);

  const auto error = is_operation_allowed(request.filename, request.type);
  if (error)
  {
    _error_pkt = error.value();
    _state     = state_t::ERROR;
  }
  else if (request.mode != tftp::mode_t::OCTET)
  {
    dbg_warn("Received NETASCII request from {}, which is not supported", _client);
    _error_pkt = tftp::error_packet_t(tftp::error_t::ILLEGAL_OPERATION, "Netascii and mail modes are not supported");
    _state     = state_t::ERROR;
  }
  else
  {
    _data_pkt.data.resize(512);

    switch (request.type)
    {
    case tftp::packet_t::READ: {
      dbg_trace("Connection created for request to READ '{}' by client [{}]", request.filename, _client);
      _state        = state_t::SEND_DATA;
      _block_number = 1;
      _fd           = fopen(request.filename.c_str(), "r");
      if (_fd == NULL)
      {
        dbg_err("Failed to open file '{}' for reading [{}]", request.filename, _client);
        _error_pkt = tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "Failed to open file for reading");
        _state     = state_t::ERROR;
      }
      break;
    }
    case tftp::packet_t::WRITE: {
      dbg_trace("Connection created for request to WRITE '{}' by client [{}]", request.filename, _client);
      _state        = state_t::SEND_ACK;
      _block_number = 0;
      _fd           = fopen(request.filename.c_str(), "w");
      if (_fd == NULL)
      {
        dbg_err("Failed to open file '{}' for writing [{}]", request.filename, _client);
        _error_pkt = tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "Failed to open file for writing");
        _state     = state_t::ERROR;
      }
      break;
    }
    case tftp::packet_t::DATA:
    case tftp::packet_t::ACK:
    case tftp::packet_t::ERROR:
    default: {
      dbg_err("Received unexpected packet type from {}", _client);
      _error_pkt = tftp::error_packet_t(tftp::error_t::ILLEGAL_OPERATION, "ILLEGAL_OPERATION");
      _state     = state_t::ERROR;
    }
    }
  }
}

//========================================================
tftp_server_connection::~tftp_server_connection()
{
  if (_fd != NULL)
  {
    fclose(_fd);
  }
}

//========================================================
int tftp_server_connection::sd() const
{
  return _udp.sd();
}

//========================================================
bool tftp_server_connection::is_finished() const
{
  return _finished;
}

//========================================================
bool tftp_server_connection::wait_for_read() const
{
  return (_state == state_t::WAIT_FOR_ACK || _state == state_t::WAIT_FOR_DATA);
}
//========================================================
bool tftp_server_connection::wait_for_write() const
{
  return (_state == state_t::SEND_ACK || _state == state_t::SEND_DATA || _state == state_t::ERROR);
}

//========================================================
const struct sockaddr_in &tftp_server_connection::client()
{
  return _client;
}

//========================================================
void tftp_server_connection::handle_read()
{
  switch (_state)
  {
  case state_t::WAIT_FOR_ACK: {
    const auto ack_packet = tftp::deserialise_ack_packet(_udp.recv(tftp::ACK_PKT_MAX_SIZE));
    if (ack_packet)
    {
      if (ack_packet->block_number != _block_number)
      {
        dbg_err("Received incorrect block number in ack {} vs expected {} [{}]", ack_packet->block_number,
                _block_number, _client);
        _state = state_t::ERROR;
      }
      else
      {
        dbg_trace("Received ack to block {}", _block_number);
        _state = state_t::SEND_DATA;
        ++_block_number;
      }
    }
    break;
  }
  case state_t::WAIT_FOR_DATA: {
    const auto data_packet = tftp::deserialise_data_packet(_udp.recv(tftp::DATA_PKT_MAX_SIZE));
    if (data_packet)
    {
      if (data_packet->block_number != _block_number)
      {
        dbg_err("Received incorrect block number in data packet {} from {}", _block_number, _client);
        _state = state_t::ERROR;
      }
      else
      {
        dbg_trace("Received data block {} from {}", _block_number, _client);
        if (fwrite(data_packet->data.data(), 1, data_packet->data.size(), _fd) < data_packet->data.size())
        {
          dbg_err("Short write occued on data block {} from {}", _block_number, _client);
          _error_pkt = tftp::error_packet_t(tftp::error_t::NOT_DEFINED, "Internal server error");
          _state     = state_t::ERROR;
          break;
        }

        if (data_packet->data.size() < tftp::DATA_PKT_DATA_MAX_SIZE)
        {
          dbg_trace("Received final data block from client {}", _client);
          fclose(_fd);
          _fd = NULL;
        }

        _state = state_t::SEND_ACK;
      }
    }
    break;
  }
  case state_t::SEND_ACK:
  case state_t::SEND_DATA:
  case state_t::ERROR:
  default: {
    break;
  }
  }
}

//========================================================
void tftp_server_connection::handle_write()
{
  switch (_state)
  {
  case state_t::SEND_DATA: {

    if (!_pkt_ready)
    {
      const size_t block_size = 512;
      size_t       read       = 0;
      while (read < block_size && !feof(_fd))
      {
        read += fread(_data_pkt.data.data(), 1, block_size - read, _fd);
      }
      if (read < 512 || feof(_fd))
      {
        dbg_trace("Read last data block {} ({} bytes)", _block_number, read);
        _data_pkt.data.resize(read);
        if ((read == 512) != feof(_fd))
        {
          _finished = true;
        }
        else
        {
          dbg_trace("Ended cleanly on 512");
        }
      }
      else
      {
        dbg_info("Created data packet block {}", _block_number);
      }

      _data_pkt.block_number = _block_number;
      _pkt_ready             = true;
    }

    const ssize_t ret = _udp.send(tftp::serialise_data_packet(_data_pkt));
    if ((ret <= 0) && ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
    {
      dbg_err("Send data packet failed for client {} : {}", _client, utils::string_error(errno));
      _error_pkt = tftp::error_packet_t(tftp::error_t::NOT_DEFINED, "Internal server error");
      _state     = state_t::ERROR;
    }
    else if (ret > 0)
    {
      dbg_info("Sent data packet block {}", _block_number);
      _pkt_ready = false;
      _state     = state_t::WAIT_FOR_ACK;
    }
    else
    {
      dbg_info("Send for data packet block {} not ready for client {}", _block_number, _client);
    }
    break;
  }
  case state_t::SEND_ACK: {
    const auto    data = tftp::serialise_ack_packet(tftp::ack_packet_t(_block_number));
    const ssize_t ret  = _udp.send(data);
    if ((ret <= 0) && ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
    {
      dbg_err("Send failed : {}", utils::string_error(errno));
      _error_pkt = tftp::error_packet_t(tftp::error_t::NOT_DEFINED, "Internal server error");
      _state     = state_t::ERROR;
    }
    else if (ret > 0)
    {
      dbg_info("Sent ack packet block {}", _block_number);
      ++_block_number;
      _state = state_t::WAIT_FOR_DATA;
      if (_fd == NULL)
      {
        _finished = true;
      }
    }
    else
    {
      dbg_info("Send for data packet block {} not ready for client {}", _block_number, _client);
    }
    break;
  }
  case state_t::WAIT_FOR_ACK:
  case state_t::WAIT_FOR_DATA:
  case state_t::ERROR: {
    const auto    data = tftp::serialise_error_packet(_error_pkt);
    const ssize_t ret  = _udp.send(data);
    if ((ret <= 0) && ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
    {
      dbg_err("Send error msg failed : {}", utils::string_error(errno));
      _finished = true;
    }
    else if (ret > 0)
    {
      dbg_trace("Sent error packet");
      _finished = true;
    }
    break;
  }
  default: {
    dbg_err("No implemented yet");
    throw std::runtime_error("Not implemented");
  }
  }
}

//========================================================
std::optional<tftp::error_packet_t> tftp_server_connection::is_operation_allowed(const std::string   &file_request,
                                                                                 const tftp::packet_t type) const
{
  const auto filepath              = std::filesystem::current_path() /= std::filesystem::path(file_request);
  const auto canonical_filepath    = std::filesystem::weakly_canonical(filepath);
  const auto canonical_server_root = std::filesystem::absolute(std::filesystem::current_path());

  dbg_trace("Checking if requested file : {} is withing server root {}", canonical_filepath, canonical_server_root);

  if (!utils::is_subpath(canonical_filepath, canonical_server_root))
  {
    dbg_warn("File {} is not in server root [{}]", canonical_filepath, _client);
    return tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "Access denied");
  }

  if ((type == tftp::packet_t::WRITE) && (std::filesystem::exists(canonical_filepath)))
  {
    dbg_warn("File {} already exists [{}]", canonical_filepath, _client);
    return tftp::error_packet_t(tftp::error_t::FILE_EXISTS, "File already exists");
  }

  if ((type == tftp::packet_t::READ) && (!std::filesystem::exists(canonical_filepath)))
  {
    dbg_warn("File {} does not exists [{}]", canonical_filepath, _client);
    return tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "File not found");
  }

  return std::nullopt;
}
