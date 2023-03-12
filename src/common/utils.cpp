
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
  for (const auto &c : data)
  {
    if (c == CR)
    {
      continue;
    }
    else
    {
      ret.push_back(c);
    }
  }

  return ret;
}
