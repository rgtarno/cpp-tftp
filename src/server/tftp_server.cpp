#include "server/tftp_server.hpp"

#include <sys/epoll.h>
#include <unistd.h>

#include "common/debug_macros.hpp"
#include "common/utils.hpp"

//========================================================
tftp_server::tftp_server(const std::string &server_root, const std::string &local_interface, const int port_num, const size_t max_clients) :
    _server_root(server_root),
    _epoll_fd(-1),
    _max_clients(max_clients),
    _exit_requested(false),
    _conn_handler((local_interface.empty() ? "0.0.0.0" : local_interface), port_num),
    _client_connections{}
{
  if (chdir(server_root.c_str()) < 0)
  {
    dbg_err("Failed to chdir to server root '{}' : {}", server_root, utils::string_error(errno));
    throw std::runtime_error("Failed to chdir");
  }

  _epoll_fd = epoll_create1(EPOLL_CLOEXEC);
  if (_epoll_fd < 0)
  {
    dbg_err("Failed to create epoll : {}", utils::string_error(errno));
    throw std::runtime_error("Failed to create epoll");
  }
}

//========================================================
void tftp_server::stop()
{
  dbg_dbg("Stopping server...");
  _exit_requested = true;
}

//========================================================
tftp_server::~tftp_server()
{
  if (_epoll_fd > 0)
  {
    close(_epoll_fd);
  }
}

//========================================================
void tftp_server::start()
{
  epoll_ctl_add(_conn_handler.sd(), EPOLLIN, &_conn_handler);

  const int   TIMEOUT_MS = 1000;
  const int   MAX_EVENTS = _max_clients + 1;
  epoll_event events[MAX_EVENTS];

  while (!_exit_requested)
  {
    const int num_events = epoll_wait(_epoll_fd, events, MAX_EVENTS, TIMEOUT_MS);
    if (num_events < 0)
    {
      if (errno == EINTR)
      {
        continue;
      }
      dbg_err("epoll error : {}", utils::string_error(errno));
      throw std::runtime_error("epoll error");
    }

    for (int i = 0; i < num_events; ++i)
    {
      /* Handle new requests */
      if (events[i].data.ptr == &_conn_handler)
      {
        _conn_handler.handle_read();

        while (_conn_handler.requests_pending() && (_client_connections.size() < _max_clients))
        {
          auto new_request = _conn_handler.get_request();
          dbg_dbg("Accepting new connection from client {}", new_request.client);
          _client_connections.emplace_back(new_request.request, new_request.client);
          const uint32_t epoll_events = _client_connections.back().wait_for_read() ? EPOLLIN : EPOLLOUT;
          epoll_ctl_add(_client_connections.back().sd(), epoll_events, &_client_connections.back());
          epoll_ctl_add(_client_connections.back().timer_fd(), EPOLLIN, &_client_connections.back());
        }
      }
      else
      {
        /* Service connected clients */
        tftp_server_connection *conn = reinterpret_cast<tftp_server_connection *>(events[i].data.ptr);
        if (events[i].events & EPOLLIN)
        {
          conn->handle_read();
        }
        else if (events[i].events & EPOLLOUT)
        {
          conn->handle_write();
        }
        else if (events[i].events & (EPOLLERR | EPOLLHUP))
        {
          dbg_warn("Socket error on connection to {}, closing connection", conn->client());
          conn->set_finished(true);
        }
        else
        {
          const int unknown_event = events[i].events;
          dbg_warn("Unknown event : {}", unknown_event);
        }

        events[i].events = conn->wait_for_read() ? EPOLLIN : EPOLLOUT;
        epoll_ctl_mod(conn->sd(), &events[i]);
      }
    }

    // Clean up -- TODO: integrate this in to the above epoll event handling
    for (auto iter = _client_connections.begin(); iter != _client_connections.end(); ++iter)
    {
      if (iter->is_finished())
      {
        dbg_dbg("Closing connection {}", iter->client());
        epoll_ctl_del(iter->sd());
        epoll_ctl_del(iter->timer_fd());
        iter = _client_connections.erase(iter);
      }
    }
  }
  dbg_dbg("Server stopped");
}

//========================================================
void tftp_server::epoll_ctl_add(const int fd, const uint32_t events, void *data)
{
  struct epoll_event e = {0, {0}};
  e.events             = events;
  e.data.ptr           = data;

  if (epoll_ctl(_epoll_fd, EPOLL_CTL_ADD, fd, &e) < 0)
  {
    dbg_err("epoll add failed : {}", utils::string_error(errno));
    throw std::runtime_error("epoll add failed");
  }
}

//========================================================
void tftp_server::epoll_ctl_mod(const int fd, struct epoll_event *ev)
{
  if (epoll_ctl(_epoll_fd, EPOLL_CTL_MOD, fd, ev) < 0)
  {
    dbg_err("epoll mod failed : {}", utils::string_error(errno));
    throw std::runtime_error("epoll mod failed");
  }
}

//========================================================
void tftp_server::epoll_ctl_del(const int fd)
{
  if (epoll_ctl(_epoll_fd, EPOLL_CTL_DEL, fd, NULL) < 0)
  {
    dbg_err("epoll del failed : {}", utils::string_error(errno));
    throw std::runtime_error("epoll del failed");
  }
}