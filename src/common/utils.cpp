
#include "utils.hpp"

#include <algorithm>

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
  for (auto iter = data.cbegin(); iter != data.cend(); ++iter)
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
  }

  return ret;
}

//========================================================
std::optional<sockaddr_in> utils::to_sockaddr_in(const std::string &addr, const uint16_t port)
{
  sockaddr_in sa;
  std::memset(&sa, 0, sizeof(sockaddr_in));
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
