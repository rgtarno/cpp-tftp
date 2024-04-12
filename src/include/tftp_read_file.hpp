#pragma once

#include <stdio.h>
#include <string>
#include <vector>

#include "tftp.hpp"

class tftp_read_file
{
public:
  tftp_read_file();
  tftp_read_file(const std::string &filename, const tftp::mode_t mode);
  tftp_read_file(const tftp_read_file &t) = delete;
  tftp_read_file(tftp_read_file &&t)      = delete;
  tftp_read_file &operator=(const tftp_read_file &) = delete;
  tftp_read_file &operator=(tftp_read_file &&) = delete;
  ~tftp_read_file();

  void open(const std::string &filename, const tftp::mode_t mode);
  void read_in_to(std::vector<char> &ret, const size_t size_bytes);
  bool eof() const;
  bool error() const;

private:
  FILE             *_fd;
  tftp::mode_t      _mode;
  std::vector<char> _overflow_buffer;
};