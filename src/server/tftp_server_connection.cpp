#include "server/tftp_server_connection.hpp"

#include <filesystem>
#include <netinet/in.h>

#include "common/debug_macros.hpp"
#include "common/utils.hpp"

namespace
{
  const char    BLKSIZE_OPT[] = "BLKSIZE";
  const char    TSIZE_OPT[]   = "TSIZE";
  const char    TIMEOUT_OPT[] = "TIMEOUT";
  const uint8_t MAX_TIMEOUTS  = 3;
}; // namespace

//========================================================
tftp_server_connection::tftp_server_connection(const tftp::rw_packet_t &request, struct sockaddr_in &client_address) :
    _logger(spdlog::get("console")),
    _udp(),
    _type(request.type),
    _file_reader(),
    _file_writer(),
    _client(client_address),
    _client_str(utils::sockaddr_to_str(_client)),
    _data_pkt(),
    _error_pkt(),
    _finished(false),
    _final_ack(false),
    _pkt_ready(false),
    _state(state_t::ERROR),
    _timeout_s(2),
    _timeout_count(0),
    _block_number(0),
    _block_size(512),
    _oack_packet{},
    _timer()

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
  else
  {
    _data_pkt.data.resize(_block_size);

    process_options(request);

    switch (request.type)
    {
    case tftp::packet_t::READ: {
      log_debug(_logger, "Connection created for request to READ '{}' by client [{}]", request.filename, _client_str);
      try
      {
        _file_reader.open(request.filename, request.mode);
      }
      catch (const std::exception &err)
      {
        log_error(_logger, "Failed to open file '{}' for reading [{}] : ", request.filename, _client_str, err.what());
        _error_pkt = tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "Failed to open file for reading");
        _state     = state_t::ERROR;
      }

      if (_oack_packet.options.empty())
      {
        _state        = state_t::SEND_DATA;
        _block_number = 1;
      }
      else
      {
        _block_number = 0;
        _state        = state_t::SEND_OACK;
      }

      break;
    }
    case tftp::packet_t::WRITE: {
      log_debug(_logger, "Connection created for request to WRITE '{}' by client [{}]", request.filename, _client_str);
      if (_oack_packet.options.empty())
      {
        _state = state_t::SEND_ACK;
      }
      else
      {
        _state = state_t::SEND_OACK;
      }
      _block_number = 0;
      try
      {
        _file_writer.open(request.filename, request.mode);
      }
      catch (const std::exception &err)
      {
        log_error(_logger, "Failed to open file '{}' for writing [{}] : ", request.filename, _client_str, err.what());
        _error_pkt = tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "Failed to open file for writing");
        _state     = state_t::ERROR;
      }
      break;
    }
    case tftp::packet_t::DATA:
    case tftp::packet_t::ACK:
    case tftp::packet_t::ERROR:
    case tftp::packet_t::OACK:
    default: {
      log_error(_logger, "Received unexpected packet type from {}", _client_str);
      _error_pkt = tftp::error_packet_t(tftp::error_t::ILLEGAL_OPERATION, "ILLEGAL_OPERATION");
      _state     = state_t::ERROR;
    }
    }
  }
}

//========================================================
tftp_server_connection::~tftp_server_connection() = default;

//========================================================
/**
 * @brief Parse options contained in a read/write request packet
 *
 * Currently supports block size, transfer size and timeout duration
 */
void tftp_server_connection::process_options(const tftp::rw_packet_t &request)
{
  for (const auto &opt : request.options)
  {
    log_trace(_logger, "Processing option '{}' val = '{}'", opt.first, opt.second);
    if (std::strcmp(opt.first.c_str(), BLKSIZE_OPT) == 0)
    {
      try
      {
        uint32_t val  = std::stoul(opt.second);
        _block_size   = val;
        const int MTU = utils::get_mtu(_udp.sd());
        if (MTU < 0)
        {
          log_warn(_logger, "Failed to query MTU [{}]", _client_str);
          _oack_packet.options.push_back(std::make_pair(opt.first, opt.second));
          log_trace(_logger, "Requested blksize is {} [{}]", _block_size, _client_str);
        }
        else if (MTU < static_cast<int>(_block_size))
        {
          _block_size = MTU;
          _oack_packet.options.push_back(std::make_pair(opt.first, std::to_string(MTU)));
          log_info(_logger, "Requested blksize is greater than MTU : {} vs {}. Replying with MTU [{}]", _block_size,
                   MTU, _client_str);
        }
        else
        {
          _oack_packet.options.push_back(std::make_pair(opt.first, opt.second));
          log_trace(_logger, "Requested blksize is {} [{}]", _block_size, _client_str);
        }
      }
      catch (const std::exception &err)
      {
        log_error(_logger, "Failed to convert blksize value to int '{}' [{}]", opt.second, _client_str);
      }
    }
    else if (std::strcmp(opt.first.c_str(), TSIZE_OPT) == 0)
    {
      switch (request.type)
      {
      case tftp::packet_t::READ: {
        const size_t FILE_SIZE = utils::get_file_size(request.filename.c_str());
        _oack_packet.options.push_back(std::make_pair(opt.first, std::to_string(FILE_SIZE)));
        break;
      }
      case tftp::packet_t::WRITE: {
        log_trace(_logger, "Incoming file '{}' is {} bytes [{}]", request.filename, opt.second, _client_str);
        break;
      }
      case tftp::packet_t::DATA:
      case tftp::packet_t::ACK:
      case tftp::packet_t::OACK:
      case tftp::packet_t::ERROR:
      default: {
        break;
      }
      }
    }
    else if (std::strcmp(opt.first.c_str(), TIMEOUT_OPT) == 0)
    {
      try
      {
        const uint64_t req_timeout_s = std::stoull(opt.second);
        if ((req_timeout_s < 1) || (req_timeout_s > 255))
        {
          log_warn(_logger, "Received invalid timeout value '{}' [{}]", opt.second, _client_str);
        }
        else
        {
          _timeout_s = static_cast<uint8_t>(req_timeout_s);
          _oack_packet.options.push_back(std::make_pair(opt.first, std::to_string(_timeout_s)));
          log_trace(_logger, "Set timeout to {}s [{}]", _timeout_s, _client_str);
        }
      }
      catch (const std::exception &err)
      {
        log_error(_logger, "Failed to convert timeout value to int '{}' [{}]", opt.second, _client_str);
      }
    }
  }
}

//========================================================
/**
 * @brief Returns file descriptor of the UDP socket
 */
int tftp_server_connection::sd() const
{
  return _udp.sd();
}
//========================================================
/**
 * @brief Returns file descriptor of the timer fd
 */
int tftp_server_connection::timer_fd() const
{
  return _timer.fd();
}

//========================================================
bool tftp_server_connection::is_finished() const
{
  return _finished;
}

//========================================================
/**
 * @brief Set the finished flag
 */
void tftp_server_connection::set_finished(const bool finished)
{
  _finished = finished;
}

//========================================================
/**
 * @brief Returns true if this client session is waiting to receive a packet
 */
bool tftp_server_connection::wait_for_read() const
{
  return (_state == state_t::WAIT_FOR_ACK || _state == state_t::WAIT_FOR_DATA);
}
//========================================================
/**
 * @brief Returns true if this client session is waiting to send a packet
 */
bool tftp_server_connection::wait_for_write() const
{
  return (_state == state_t::SEND_ACK || _state == state_t::SEND_DATA || _state == state_t::SEND_OACK ||
          _state == state_t::ERROR);
}

//========================================================
const struct sockaddr_in &tftp_server_connection::client() const
{
  return _client;
}

//========================================================
/**
 * @brief Changes the state machines state to resend the previous packet
 *
 */
void tftp_server_connection::retransmit()
{
  if (_state == state_t::WAIT_FOR_ACK)
  {
    _state = state_t::SEND_DATA;
  }
  else if (_state == state_t::WAIT_FOR_DATA)
  {
    _state = state_t::SEND_ACK;
    _block_number -= 1;
  }
  else
  {
    assert(false);
    log_error(_logger, "Unable to retransmit: Invalid state [{}]", _client_str);
  }
}

//========================================================
/**
 * @brief Handles reading of the next packet and advances the state machine
 *
 */
void tftp_server_connection::handle_read()
{
  if (_timer.has_expired())
  {
    if (_timeout_count >= MAX_TIMEOUTS)
    {
      log_error(_logger, "Reached maximum retransmits, ending connection [{}]", _client_str);
      _finished = true;
    }
    else
    {
      _timeout_count += 1;
      log_warn(_logger, "Timed out in '{}': retransmitting last packet. [{}]", state_to_string(_state), _client_str);
      retransmit();
    }
    return;
  }

  switch (_state)
  {
  case state_t::WAIT_FOR_ACK: {
    const auto recv_data  = _udp.recv(tftp::ACK_PKT_MAX_SIZE);
    const auto ack_packet = tftp::deserialise_ack_packet(recv_data);
    if (ack_packet)
    {
      _timeout_count = 0;
      if (ack_packet->block_number == (_block_number - 1))
      {
        log_trace(
            _logger,
            "Received ack to previous packet data packet ({}), assuming packet was lost, retransmitting last [{}]",
            _block_number - 1, _client_str);
        _state = state_t::SEND_DATA;
      }
      else if (ack_packet->block_number == _block_number)
      {
        if (_final_ack)
        {
          log_trace(_logger, "Received final ack ({}) [{}]", _block_number, _client_str);
          _finished = true;
          break;
        }
        log_trace(_logger, "Received ack to block {} [{}]", _block_number, _client_str);
        _pkt_ready = false;
        _state     = state_t::SEND_DATA;
        ++_block_number;
      }
      else
      {
        log_error(_logger, "Received incorrect block number in ack {} vs expected {} [{}]", ack_packet->block_number,
                  _block_number, _client_str);
        _state = state_t::ERROR;
      }
    }
    else
    {
      const auto error_packet = tftp::deserialise_error_packet(recv_data);
      if (error_packet)
      {
        log_warn(_logger, "Received error when waiting for ack to block {} from client [{}] : {} - {}", _block_number,
                 _client_str, error_packet->error_code, error_packet->error_msg);
        _finished = true;
      }
    }
    break;
  }
  case state_t::WAIT_FOR_DATA: {
    const auto recv_data   = _udp.recv(_block_size);
    const auto data_packet = tftp::deserialise_data_packet(recv_data);
    if (data_packet)
    {
      _timeout_count = 0;
      if (data_packet->block_number == (_block_number - 1))
      {
        log_trace(
            _logger,
            "Received data packet to previous ack packet ({}), assuming packet was lost, retransmitting last ack [{}]",
            _block_number - 1, _client_str);
        _block_number -= 1;
        _state = state_t::SEND_ACK;
      }
      else if (data_packet->block_number == _block_number)
      {
        log_trace(_logger, "Received data block {} from {}", _block_number, _client_str);
        _file_writer.write(data_packet->data);
        if (_file_writer.error())
        {
          log_error(_logger, "Error occued when writing data block {} from {}", _block_number, _client_str);
          _error_pkt = tftp::error_packet_t(tftp::error_t::NOT_DEFINED, "Internal server error");
          _state     = state_t::ERROR;
          break;
        }

        if (data_packet->data.size() < _block_size)
        {
          log_trace(_logger, "Received final data block from client {}", _client_str);
          _final_ack = true;
        }

        _state = state_t::SEND_ACK;
      }
      else
      {
        log_error(_logger, "Received incorrect block number in data packet {} from {}", _block_number, _client_str);
        _state = state_t::ERROR;
      }
    }
    else
    {
      const auto error_packet = tftp::deserialise_error_packet(recv_data);
      if (error_packet)
      {
        log_warn(_logger, "Received error when waiting for data packet block {} from client [{}] : {} - {}",
                 _block_number, _client_str, error_packet->error_code, error_packet->error_msg);
        _finished = true;
      }
    }
    break;
  }
  case state_t::SEND_ACK:
  case state_t::SEND_OACK:
  case state_t::SEND_DATA:
  case state_t::ERROR:
  default: {
    break;
  }
  }
}

//========================================================
/**
 * @brief Handles sending of the current packet and advances the state machine
 *
 */
void tftp_server_connection::handle_write()
{
  switch (_state)
  {
  case state_t::SEND_DATA: {

    if (!_pkt_ready)
    {
      _file_reader.read_in_to(_data_pkt.data, _block_size);
      if (_file_reader.error())
      {
        log_error(_logger, "Error occued when reading data block {} from {}", _block_number, _client_str);
        _error_pkt = tftp::error_packet_t(tftp::error_t::NOT_DEFINED, "Internal server error");
        _state     = state_t::ERROR;
        break;
      }
      else if ((_data_pkt.data.size() < _block_size) || _file_reader.eof())
      {
        log_trace(_logger, "Read last data block {} ({} bytes) [{}]", _block_number, _data_pkt.data.size(),
                  _client_str);
        if (_data_pkt.data.size() < _block_size)
        {
          _final_ack = true;
        }
      }

      _data_pkt.block_number = _block_number;
      _pkt_ready             = true;
    }

    const ssize_t ret = _udp.send(tftp::serialise_data_packet(_data_pkt));
    if ((ret <= 0) && ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
    {
      log_error(_logger, "Send data packet failed for client {} : {}", _client_str, utils::string_error(errno));
      _error_pkt = tftp::error_packet_t(tftp::error_t::NOT_DEFINED, "Internal server error");
      _state     = state_t::ERROR;
    }
    else if (ret > 0)
    {
      log_trace(_logger, "Sent data packet block {} [{}]", _block_number, _client_str);
      _state = state_t::WAIT_FOR_ACK;
      _timer.arm_timer(_timeout_s);
    }
    else
    {
      log_trace(_logger, "Send for data packet block {} not ready for client {}", _block_number, _client_str);
    }
    break;
  }
  case state_t::SEND_ACK: {
    const auto    data = tftp::serialise_ack_packet(tftp::ack_packet_t(_block_number));
    const ssize_t ret  = _udp.send(data);
    if ((ret <= 0) && ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
    {
      log_error(_logger, "Send failed : {}", utils::string_error(errno));
      _error_pkt = tftp::error_packet_t(tftp::error_t::NOT_DEFINED, "Internal server error");
      _state     = state_t::ERROR;
    }
    else if (ret > 0)
    {
      if (_final_ack)
      {
        log_trace(_logger, "Sent final ack packet block {} [{}]", _block_number, _client_str);
        _finished = true;
        break;
      }
      log_trace(_logger, "Sent ack packet block {} [{}]", _block_number, _client_str);
      ++_block_number;
      _state = state_t::WAIT_FOR_DATA;
      _timer.arm_timer(_timeout_s);
    }
    else
    {
      log_info(_logger, "Send for data packet block {} not ready for client {}", _block_number, _client_str);
    }
    break;
  }
  case state_t::SEND_OACK: {
    const auto    data = tftp::serialise_oack_packet(_oack_packet);
    const ssize_t ret  = _udp.send(data);
    if ((ret <= 0) && ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
    {
      log_error(_logger, "Send OACK failed : {}", utils::string_error(errno));
      _finished = true;
    }
    else if (ret > 0)
    {
      log_trace(_logger, "Sent OACK packet");
      if (_type == tftp::packet_t::READ)
      {
        _state = state_t::WAIT_FOR_ACK;
      }
      else
      {
        _state = state_t::WAIT_FOR_DATA;
      }
    }
    break;
  }
  case state_t::ERROR: {
    const auto    data = tftp::serialise_error_packet(_error_pkt);
    const ssize_t ret  = _udp.send(data);
    if ((ret <= 0) && ((errno != EAGAIN) && (errno != EWOULDBLOCK)))
    {
      log_error(_logger, "Send error msg failed : {}", utils::string_error(errno));
      _finished = true;
    }
    else if (ret > 0)
    {
      log_trace(_logger, "Sent error packet");
      _finished = true;
    }
    break;
  }
  case state_t::WAIT_FOR_ACK:
  case state_t::WAIT_FOR_DATA:
  default: {
    log_error(_logger, "Error state");
    throw std::runtime_error("Error state");
  }
  }
}

//========================================================
/**
 * @brief Checks if a read / write operation is allowed
 *
 * Checks if the request filepath is contained within the server root.
 * Checks if a file already exists for write requests.
 * Checks if a file doesn't exist for read requests.
 *
 * @param file_request Request filepath
 * @param type Request type: read or write.
 * @return std::optional<tftp::error_packet_t> Returns nullopt if operation is ok, otherwise, returns the error packet
 * with the error code & msg.
 */
std::optional<tftp::error_packet_t> tftp_server_connection::is_operation_allowed(const std::string   &file_request,
                                                                                 const tftp::packet_t type) const
{
  const auto filepath              = std::filesystem::current_path() /= std::filesystem::path(file_request);
  const auto canonical_filepath    = std::filesystem::weakly_canonical(filepath);
  const auto canonical_server_root = std::filesystem::absolute(std::filesystem::current_path());

  log_trace(_logger, "Checking if requested file : {} is within server root {}", canonical_filepath.c_str(),
            canonical_server_root.c_str());

  if (!utils::is_subpath(canonical_filepath, canonical_server_root))
  {
    log_warn(_logger, "File {} is not in server root [{}]", canonical_filepath.c_str(), _client_str);
    return tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "Access denied");
  }

  if ((type == tftp::packet_t::WRITE) && (std::filesystem::exists(canonical_filepath)))
  {
    log_warn(_logger, "File {} already exists [{}]", canonical_filepath.c_str(), _client_str);
    return tftp::error_packet_t(tftp::error_t::FILE_EXISTS, "File already exists");
  }

  if ((type == tftp::packet_t::READ) && (!std::filesystem::exists(canonical_filepath)))
  {
    log_warn(_logger, "File {} does not exists [{}]", canonical_filepath.c_str(), _client_str);
    return tftp::error_packet_t(tftp::error_t::ACCESS_ERROR, "File not found");
  }

  log_trace(_logger, "File is within server root");
  return std::nullopt;
}

//========================================================
std::string tftp_server_connection::state_to_string(const state_t state)
{
  switch (state)
  {
  case state_t::WAIT_FOR_ACK:
    return std::string("Wait for ACK");
  case state_t::WAIT_FOR_DATA:
    return std::string("Wait for DATA");
  case state_t::SEND_ACK:
    return std::string("Send ACK");
  case state_t::SEND_DATA:
    return std::string("Send DATA");
  case state_t::SEND_OACK:
    return std::string("Send OACK");
  case state_t::ERROR:
    return std::string("Send ERROR");
  default:
    return std::string("UNKNOWN");
  }
}