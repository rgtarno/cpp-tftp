#include "tftp_write_file.hpp"

#include <cassert>

#include "debug_macros.hpp"
#include "utils.hpp"

//========================================================
tftp_write_file::tftp_write_file() : _fd(NULL), _mode(tftp::mode_t::OCTET)
{
}

//========================================================
tftp_write_file::tftp_write_file(const std::string &filename, const tftp::mode_t mode) : _fd(NULL), _mode(mode)
{
  _fd = fopen(filename.c_str(), "wb");
  if (_fd == NULL)
  {
    throw std::runtime_error(utils::string_error(errno));
  }
}

//========================================================
tftp_write_file::~tftp_write_file()
{
  if (_fd != NULL)
  {
    fclose(_fd);
  }
}

//========================================================
void tftp_write_file::open(const std::string &filename, const tftp::mode_t mode)
{
  _mode = mode;
  _fd   = fopen(filename.c_str(), "w");
  if (_fd == NULL)
  {
    throw std::runtime_error(utils::string_error(errno));
  }
}

//========================================================
bool tftp_write_file::eof() const
{
  return feof(_fd);
}

//========================================================
bool tftp_write_file::error() const
{
  return ferror(_fd);
}

//========================================================
void tftp_write_file::write(const std::vector<char> &data)
{
  size_t bytes_written = 0;
  if (_mode != tftp::mode_t::OCTET)
  {
    const auto native_data = utils::netascii_to_native(data);
    while (!ferror(_fd) && !feof(_fd) && (bytes_written < native_data.size()))
    {
      bytes_written += fwrite(native_data.data(), 1, native_data.size() - bytes_written, _fd);
    }
    dbg_trace("Wrote {} bytes to file", bytes_written);
  }
  else
  {
    while (!feof(_fd) && (bytes_written < data.size()))
    {
      bytes_written += fwrite(data.data(), 1, data.size() - bytes_written, _fd);
    }
    dbg_trace("Wrote {} bytes to file", bytes_written);
  }
}
