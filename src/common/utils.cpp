
#include "utils.hpp"

#include <algorithm>
#include <net/if.h>
#include <sys/ioctl.h>
#include <sys/stat.h>

//========================================================
std::vector<char> utils::native_to_netascii(const std::vector<char> &data)
{
  const char CR = (char)0x0D;
  const char LF = (char)0x0A;

  std::vector<char> ret;
  ret.reserve(data.size() * 2); // Worst case where every char is a \n

  // TODO: make this more efficient
  for (const auto &c : data)
  {
    if (c == LF)
    {
      ret.push_back(CR);
      ret.push_back(LF);
    }
    else if (c == CR)
    {
      ret.push_back(CR);
      ret.push_back(0);
    }
    else
    {
      ret.push_back(c);
    }
  }

  return ret;
}

//========================================================
std::vector<char> utils::netascii_to_native(const std::vector<char> &data)
{
  const char CR = (char)0x0D;

  std::vector<char> ret;
  ret.reserve(data.size());

  // TODO: make this more efficient
  auto iter = data.cbegin();
  while (iter != data.cend())
  {
    if (*iter == CR)
    {
      if (std::next(iter) != data.cend())
      {
        const auto next = *std::next(iter);
        if (next == 0) // CR NULL to CR
        {
          ret.push_back(*iter);
          ++iter;
        }
        // CR LF to LF
      }
    }
    else
    {
      ret.push_back(*iter);
    }

    if (std::next(iter) != data.cend())
    {
      ++iter;
    }
  }

  return ret;
}

//========================================================
std::optional<struct sockaddr_in> utils::to_sockaddr_in(const std::string &addr, const uint16_t port)
{
  struct sockaddr_in sa;
  std::memset(&sa, 0, sizeof(sa));
  sa.sin_port   = htons(port);
  sa.sin_family = AF_INET;
  if (addr.empty())
  {
    sa.sin_addr.s_addr = INADDR_ANY;
  }
  else
  {
    if (!inet_pton(AF_INET, addr.c_str(), &(sa.sin_addr)))
    {
      return {};
    }
  }
  return sa;
}

//========================================================
std::ostream &operator<<(std::ostream &os, const struct sockaddr_in &sa)
{
  char addr_buf[INET_ADDRSTRLEN] = {0};
  if (inet_ntop(AF_INET, &(sa.sin_addr), addr_buf, INET_ADDRSTRLEN) == NULL)
  {
    return os << "unknown:" << htons(sa.sin_port);
  }
  else
  {
    return os << addr_buf << ":" << htons(sa.sin_port);
  }
}

//========================================================
bool utils::is_subpath(const std::filesystem::path &path, const std::filesystem::path &base)
{
  auto mismatch_pair = std::mismatch(path.begin(), path.end(), base.begin(), base.end());
  return mismatch_pair.second == base.end();
}

//========================================================
std::vector<std::string> utils::extract_c_strings_from_buffer(const std::vector<char> &buffer, const size_t offset)
{
  std::vector<std::string> ret;
  auto                     start = buffer.begin() + offset;
  for (auto end = std::find(start, buffer.end(), 0); end != buffer.end(); end = std::find(start, buffer.end(), 0))
  {
    ret.emplace_back(start, end);
    start = std::next(end);
  }
  return ret;
}

//========================================================
int utils::get_mtu(const int sd)
{
  struct ifreq res;

  if (ioctl(sd, SIOCGIFMTU, &res) < 0)
  {
    return -1;
  }

  return res.ifr_mtu;
}

//========================================================
size_t utils::get_file_size(const char *fname)
{
  struct stat s;
  if ((fname != nullptr) && !stat(fname, &s))
  {
    return s.st_size;
  }
  return 0;
}
