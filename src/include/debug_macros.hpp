
#pragma once

#include "spdlog/sinks/stdout_color_sinks.h"
#include "spdlog/spdlog.h"

#ifndef RELEASE

#define PRINT_FUNCTION static_cast<const char *>(__FUNCTION__)

#define dbg_trace(...)                                                                                                 \
  spdlog::get("console")->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::trace, __VA_ARGS__)
#define dbg_dbg(...)                                                                                                   \
  spdlog::get("console")->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::debug, __VA_ARGS__)
#define dbg_info(...)                                                                                                  \
  spdlog::get("console")->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::info, __VA_ARGS__)
#define dbg_warn(...)                                                                                                  \
  spdlog::get("console")->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::warn, __VA_ARGS__)
#define dbg_err(...)                                                                                                   \
  spdlog::get("console")->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::err, __VA_ARGS__)
#define dbg_crit(...)                                                                                                  \
  spdlog::get("console")->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::critical,         \
                              __VA_ARGS__)

#else

#define dbg_trace(...) spdlog::get("console")->trace(__VA_ARGS__)
#define dbg_dbg(...) spdlog::get("console")->debug(__VA_ARGS__)
#define dbg_info(...) spdlog::get("console")->info(__VA_ARGS__)
#define dbg_warn(...) spdlog::get("console")->warn(__VA_ARGS__)
#define dbg_err(...) spdlog::get("console")->error(__VA_ARGS__)
#define dbg_crit(...) spdlog::get("console")->critical(__VA_ARGS__)

#endif
