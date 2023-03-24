#pragma once

#include <stdio.h>
#include <string>
#include <vector>

#include "tftp.hpp"

class tftp_write_file
{
public:
  tftp_write_file();
  tftp_write_file(const std::string &filename, const tftp::mode_t mode);
  tftp_write_file(const tftp_write_file &t)           = delete;
  tftp_write_file(tftp_write_file &&t)                = delete;
  tftp_write_file &operator=(const tftp_write_file &) = delete;
  tftp_write_file &operator=(tftp_write_file &&)      = delete;
  ~tftp_write_file();

  void open(const std::string &filename, const tftp::mode_t mode);
  void write(const std::vector<char> &data);
  bool eof() const;
  bool error() const;

private:
  FILE        *_fd;
  tftp::mode_t _mode;
};