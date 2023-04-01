#pragma once

#include <time.h>

class timer
{
public:
  timer();
  timer(const timer &)            = delete;
  timer(timer &&)                 = delete;
  timer &operator=(const timer &) = delete;
  timer &operator=(timer &&)      = delete;
  ~timer();

  void arm_timer(const time_t seconds);
  void disarm_timer();
  bool has_expired();
  int  fd() const;

private:
  int _fd;
};