#pragma once

#include <atomic>
#include <list>
#include <string>

#include "server/tftp_connection_handler.hpp"
#include "server/tftp_server_connection.hpp"

class tftp_server
{
public:
  tftp_server(const std::string &server_root, const std::string &local_interface, const size_t max_clients);
  ~tftp_server();

  void start();
  void stop();

private:
  std::string                       _server_root;
  int                               _epoll_fd;
  size_t                            _max_clients;
  std::atomic_bool                  _exit_requested;
  tftp_connection_handler           _conn_handler;
  std::list<tftp_server_connection> _client_connections;

  void epoll_ctl_add(const int fd, const uint32_t events, void *data);
  void epoll_ctl_mod(const int fd, struct epoll_event *ev);
  void epoll_ctl_del(const int fd);
};