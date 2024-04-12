
#include <fmt/core.h>
#include <getopt.h>
#include <signal.h>
#include <string>

#include "common/debug_macros.hpp"
#include "common/tftp.hpp"
#include "client/tftp_client.hpp"

//==========================================================
void sig_handler(int signum)
{
  dbg_info("Received signal {}", signum);
  exit(signum);
}

//==========================================================
void print_help(char *argv0)
{
  const char help_msg[] = R"({}: [OPTIONS] FILES...
  -h --host       : IP address of the TFTP server
  -i --interface  : IP address of the local interface to send requests from (optional)
  -w --write      : Write files
)";
  fmt::print(help_msg, argv0);
}

//==========================================================
int main(int argc, char **argv)
{
  signal(SIGTERM, sig_handler);
  signal(SIGABRT, sig_handler);
  signal(SIGHUP, sig_handler);

  auto err_logger = spdlog::stderr_color_mt("console");
  spdlog::set_level(spdlog::level::warn);
  dbg_info("Initialised log");

  int verbose_flag = 0;
  int help_flag    = 0;
  int write_flag   = 0;

  static struct option long_options[] = {/* These options set a flag. */
                                         {"verbose", no_argument, &verbose_flag, 1},
                                         {"help", no_argument, &help_flag, 1},
                                         {"write", no_argument, &write_flag, 1},
                                         /* These options don’t set a flag.
                                            We distinguish them by their indices. */
                                         {"host", required_argument, 0, 'h'},
                                         {"interface", required_argument, 0, 'i'},
                                         {"type", required_argument, 0, 't'},
                                         {0, 0, 0, 0}};

  std::string tftp_host{};
  std::string local_interface{};
  std::string transfer_mode{};

  while (true)
  {
    int option_index = 0;

    int c = getopt_long(argc, argv, "vwh:i:t:", long_options, &option_index);

    if (c == -1)
      break;

    switch (c)
    {
    case 'w': {
      write_flag = 1;
      break;
    }
    case 'h': {
      tftp_host = optarg;
      break;
    }
    case 'i': {
      local_interface = optarg;
      break;
    }
    case 't': {
      transfer_mode = optarg;
      break;
    }
    case 'v': {
      verbose_flag = 1;
      break;
    }
    }
  }

  std::vector<std::string> files;
  while (optind < argc)
  {
    files.emplace_back(argv[optind++]);
  }

  if (help_flag)
  {
    print_help(argv[0]);
    return 0;
  }
  if (verbose_flag)
  {
    spdlog::set_level(spdlog::level::trace);
  }
  tftp::mode_t mode = tftp::mode_t::OCTET;
  if (!transfer_mode.empty())
  {
    const auto parsed_mode = tftp::string_to_mode_t(transfer_mode);
    if (!parsed_mode)
    {
      dbg_err("Invalid transfer type '{}'", transfer_mode);
      return 1;
    }
    mode = parsed_mode.value();
  }

  try
  {
    for (const auto &file : files)
    {
      if (write_flag)
      {
        tftp_client::send_file(file, tftp_host, mode, local_interface);
      }
      else
      {
        tftp_client::get_file(file, tftp_host, mode, local_interface);
      }
    }
  }
  catch (const std::exception &err)
  {
    dbg_err("Failure : {}", err.what());
    return 1;
  }

  dbg_trace("Exiting...");
  return 0;
}