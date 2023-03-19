
#include <signal.h>

#include "debug_macros.hpp"
#include "tftp_server.hpp"

static tftp_server *_pserver = nullptr;

void sig_handler(int signum);
void setup_signal_handlers();
void print_usage(char *argv0);
void initialise_logger();

//==========================================================
int main(int argc, char **argv)
{
  setup_signal_handlers();
  initialise_logger();

  if (argc < 3)
  {
    print_usage(argv[0]);
    return 1;
  }

  const std::string server_root(argv[1]);
  const std::string interface(argv[2]);

  try
  {
    tftp_server server(server_root, interface, 5);
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
  fmt::print("Usage: {} [SERVER_ROOT] [INTERFACE]\n", argv0);
}

//==========================================================
void initialise_logger()
{
  spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v");
  auto err_logger = spdlog::stderr_color_mt("console");
  spdlog::set_level(spdlog::level::trace);
  dbg_dbg("Initialised log");
}