
#include <signal.h>

#include "debug_macros.hpp"
#include "tftp_server.hpp"

static tftp_server *_pserver = nullptr;

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
int main(int argc, char **argv)
{
  setup_signal_handlers();

  auto err_logger = spdlog::stderr_color_mt("console");
  spdlog::set_level(spdlog::level::trace);
  dbg_dbg("Initialised log");

  if (argc < 2)
  {
    dbg_err("Require server root as 1st argument");
    return 1;
  }

  try
  {
    tftp_server server(argv[1], 5);
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