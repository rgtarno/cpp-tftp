
#include "timer.hpp"
#include <stdint.h>
#include <sys/timerfd.h>
#include <unistd.h>

#include "debug_macros.hpp"
#include "utils.hpp"

//========================================================
timer::timer() :
    _fd(-1)
{
  _fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK);
  if (_fd < 0)
  {
    throw std::runtime_error(utils::string_error(errno));
  }
}

//========================================================
timer::~timer()
{
  if (_fd > 0)
  {
    close(_fd);
  }
}

//========================================================
int timer::fd() const
{
  return _fd;
}

//========================================================
void timer::arm_timer(const time_t seconds)
{
  struct itimerspec new_value;
  new_value.it_value.tv_sec     = seconds;
  new_value.it_value.tv_nsec    = 0;
  new_value.it_interval.tv_sec  = 0;
  new_value.it_interval.tv_nsec = 0;
  if (timerfd_settime(_fd, 0, &new_value, NULL) == -1)
  {
    dbg_warn("Failed to arm timer : {}", utils::string_error(errno));
  }
}

//========================================================
void timer::disarm_timer()
{
  struct itimerspec new_value;
  new_value.it_value.tv_sec     = 0;
  new_value.it_value.tv_nsec    = 0;
  new_value.it_interval.tv_sec  = 0;
  new_value.it_interval.tv_nsec = 0;
  if (timerfd_settime(_fd, 0, &new_value, NULL) == -1)
  {
    dbg_warn("Failed to disarm timer : {}", utils::string_error(errno));
  }
}

//========================================================
bool timer::has_expired()
{
  uint64_t      exp = 0;
  const ssize_t ret = read(_fd, &exp, sizeof(uint64_t));
  return (ret > 0) && (exp > 0);
}