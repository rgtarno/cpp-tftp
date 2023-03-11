#pragma once

#include <string>
#include <vector>

class udp_connection
{
public:
  udp_connection();
  udp_connection(const udp_connection &) = delete;
  udp_connection(udp_connection &&)      = delete;
  udp_connection &operator=(const udp_connection &) = delete;
  udp_connection &operator=(udp_connection &&) = delete;
  ~udp_connection();

  void              bind(const std::string &ip_address, const uint16_t port_num);
  void              connect(const std::string &ip_address, const uint16_t port_num);
  void              send(const std::vector<char> &data);
  std::vector<char> recv(const size_t size);
  void              send_to(const std::string &ip_address, const uint16_t port_num, const std::vector<char> &data);
  std::vector<char> recv_from(std::string &ip_address, uint16_t &port_num, const size_t size);

  int sd() const
  {
    return _sd;
  }

private:
  int _sd;
};