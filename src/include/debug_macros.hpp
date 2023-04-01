
#pragma once

#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/spdlog.h>

#ifndef RELEASE

#define LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] [%s:%#] %v"

#define PRINT_FUNCTION static_cast<const char *>(__FUNCTION__)

/* Fast variants */
#define log_trace(logger, ...)                                                                                         \
  logger->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::trace, __VA_ARGS__)
#define log_debug(logger, ...)                                                                                         \
  logger->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::debug, __VA_ARGS__)
#define log_info(logger, ...)                                                                                          \
  logger->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::info, __VA_ARGS__)
#define log_warn(logger, ...)                                                                                          \
  logger->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::warn, __VA_ARGS__)
#define log_error(logger, ...)                                                                                         \
  logger->log(spdlog::source_loc{__FILE__, __LINE__, PRINT_FUNCTION}, spdlog::level::err, __VA_ARGS__)

/* These onces lock a mutex */
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

#define LOGGER_PATTERN "[%Y-%m-%d %H:%M:%S.%e] [%^%l%$] %v"

/* Fast variants */
#define log_trace(logger, ...) logger->trace(__VA_ARGS__)
#define log_debug(logger, ...) logger->debug(__VA_ARGS__)
#define log_info(logger, ...) logger->info(__VA_ARGS__)
#define log_warn(logger, ...) logger->warn(__VA_ARGS__)
#define log_error(logger, ...) logger->error(__VA_ARGS__)
#define log_critical(logger, ...) logger->critical(__VA_ARGS__)

/* These onces lock a mutex */
#define dbg_trace(...) spdlog::get("console")->trace(__VA_ARGS__)
#define dbg_dbg(...) spdlog::get("console")->debug(__VA_ARGS__)
#define dbg_info(...) spdlog::get("console")->info(__VA_ARGS__)
#define dbg_warn(...) spdlog::get("console")->warn(__VA_ARGS__)
#define dbg_err(...) spdlog::get("console")->error(__VA_ARGS__)
#define dbg_crit(...) spdlog::get("console")->critical(__VA_ARGS__)

#endif
