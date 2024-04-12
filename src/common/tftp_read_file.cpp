#include "tftp_read_file.hpp"

#include <cassert>

#include "utils.hpp"

//========================================================
tftp_read_file::tftp_read_file() :
    _fd(NULL), _mode(tftp::mode_t::OCTET), _overflow_buffer{}
{
}

//========================================================
tftp_read_file::tftp_read_file(const std::string &filename, const tftp::mode_t mode) :
    _fd(NULL), _mode(mode), _overflow_buffer{}
{
  _fd = fopen(filename.c_str(), "r");
  if (_fd == NULL)
  {
    throw std::runtime_error(utils::string_error(errno));
  }
}

//========================================================
tftp_read_file::~tftp_read_file()
{
  if (_fd != NULL)
  {
    fclose(_fd);
  }
  assert(_overflow_buffer.empty());
}

//========================================================
void tftp_read_file::open(const std::string &filename, const tftp::mode_t mode)
{
  _mode = mode;
  _fd   = fopen(filename.c_str(), "r");
  if (_fd == NULL)
  {
    throw std::runtime_error(utils::string_error(errno));
  }
}

//========================================================
bool tftp_read_file::eof() const
{
  return _overflow_buffer.empty() && feof(_fd);
}

//========================================================
bool tftp_read_file::error() const
{
  return ferror(_fd);
}

//========================================================
void tftp_read_file::read_in_to(std::vector<char> &ret, const size_t size_bytes)
{
  size_t read_bytes = 0;
  ret.resize(size_bytes);

  if (_mode != tftp::mode_t::OCTET)
  {
    const size_t bytes_from_file = size_bytes - _overflow_buffer.size();
    while (!ferror(_fd) && !feof(_fd) && (read_bytes < bytes_from_file))
    {
      read_bytes += fread(ret.data() + read_bytes, 1, bytes_from_file, _fd);
    }
    ret.resize(read_bytes);

    std::vector<char> netascii_data = utils::native_to_netascii(ret);
    ret.clear();
    if (!_overflow_buffer.empty())
    {
      ret.insert(ret.begin(), _overflow_buffer.begin(), _overflow_buffer.end());
      _overflow_buffer.clear();
    }
    if (!netascii_data.empty())
    {
      ret.insert(ret.end(), netascii_data.begin(), netascii_data.end());
    }

    if (ret.size() > size_bytes)
    {
      _overflow_buffer.insert(_overflow_buffer.begin(), ret.begin() + size_bytes, ret.end());
      ret.resize(size_bytes);
    }
  }
  else
  {
    while (!ferror(_fd) && !feof(_fd) && (read_bytes < size_bytes))
    {
      read_bytes += fread(ret.data() + read_bytes, 1, size_bytes - read_bytes, _fd);
    }
    ret.resize(read_bytes);
  }
}
