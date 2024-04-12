#include "tftp_connection_handler.hpp"
#include "debug_macros.hpp"
#include "utils.hpp"

//========================================================
tftp_connection_handler::tftp_connection_handler(const std::string &addr, const uint16_t port) :
    _udp(), _request_queue{}
{
  _udp.bind(addr, port);
  _udp.set_non_blocking(true);
}

//========================================================
int tftp_connection_handler::sd() const
{
  return _udp.sd();
}

//========================================================
void tftp_connection_handler::handle_read()
{
  std::string ip_address = "";
  uint16_t    port_num   = 0;
  auto        data       = _udp.recv_from(ip_address, port_num, 2048);

  while (!data.empty())
  {
    auto request = tftp::deserialise_rw_packet(data);
    const auto client  = utils::to_sockaddr_in(ip_address, port_num);
    if (!client || !request)
    {
      if (client)
      {
        dbg_err("Failed to parse request from {}", client.value());
      }
      else
      {
        dbg_err("Failed to parse request");
      }
    }
    else
    {
      dbg_trace("Enqueued request from {}:{}", ip_address, port_num);
      _request_queue.emplace(request.value(), client.value());
    }
    data = _udp.recv_from(ip_address, port_num, 2048);
  }
}

//========================================================
bool tftp_connection_handler::requests_pending() const
{
  return !_request_queue.empty();
}

//========================================================
tftp_connection_handler::request_t tftp_connection_handler::get_request()
{
  if (_request_queue.empty())
  {
    throw std::out_of_range("Request queue is empty");
  }
  auto ret = _request_queue.front();
  _request_queue.pop();
  return ret;
}