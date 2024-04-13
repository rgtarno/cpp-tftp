
#include <signal.h>

#include "common/debug_macros.hpp"
#include "server/tftp_server.hpp"

static tftp_server *_pserver = nullptr;

void sig_handler(int signum);
void setup_signal_handlers();
void print_usage(char *argv0);
int  initialise_logger(const bool trace);

//==========================================================
int main(int argc, char **argv)
{

  if (argc < 3)
  {
    print_usage(argv[0]);
    return 1;
  }

  bool log_trace = false;
  if (argc > 3)
  {
    try
    {
      log_trace = std::stoul(argv[3]);
    }
    catch (const std::exception &err)
    {
      fmt::print(stderr, "Failed to parse DEBUG argument : {}\n", err.what());
      return 1;
    }
  }
  const std::string server_root(argv[1]);
  const std::string interface(argv[2]);

  setup_signal_handlers();
  if (initialise_logger(log_trace))
  {
    return 1;
  }

  try
  {
    tftp_server server(server_root, interface, 69, 100);
    _pserver = &server;

    dbg_trace("Starting server");
    server.start();
  }
  catch (const std::exception &e)
  {
    dbg_err("Server failed : {}", e.what());
    return 1;
  }

  dbg_trace("Exit now");
  return 0;
}

//==========================================================
void sig_handler(int signum)
{
  dbg_trace("Received signal {}", signum);
  if (_pserver)
  {
    _pserver->stop();
  }
}

//==========================================================
void setup_signal_handlers()
{
  struct sigaction new_action;
  sigemptyset(&new_action.sa_mask);
  new_action.sa_flags   = 0;
  new_action.sa_handler = sig_handler;

  sigaction(SIGINT, &new_action, NULL);
  sigaction(SIGHUP, &new_action, NULL);
  sigaction(SIGTERM, &new_action, NULL);
  sigaction(SIGABRT, &new_action, NULL);
}

//==========================================================
void print_usage(char *argv0)
{
  fmt::print(stderr, "Usage: {} [SERVER_ROOT] [INTERFACE] [DEBUG]\n", argv0);
  fmt::print(stderr, "\tSERVER_ROOT: (Required) Path to a directory from which to serve / receive files\n");
  fmt::print(stderr, "\tINTERFACE:   (Required) Local ip address to bind to (0.0.0.0 for all)\n");
  fmt::print(stderr, "\tDEBUG:       (Optional) 1 to turn on debug and trace prints\n");
}

//==========================================================
int initialise_logger(const bool trace)
{
  try
  {
    spdlog::set_pattern(LOGGER_PATTERN);
    auto err_logger = spdlog::stderr_color_st("console");
    if (trace)
    {
      spdlog::set_level(spdlog::level::trace);
      dbg_dbg("Debug prints on");
    }
    else
    {
      spdlog::set_level(spdlog::level::info);
    }
    dbg_dbg("Initialised log");
  }
  catch (const std::exception &e)
  {
    fmt::print(stderr, "Failed to setup logger");
    return 1;
  }
  return 0;
}