
#include <string>
#include <vector>

namespace test_utils
{

  template <typename T>
  std::vector<T> join_vectors(const std::vector<std::vector<T>> &data)
  {
    std::vector<T> ret;
    for (const auto &d : data)
    {
      ret.insert(ret.end(), d.begin(), d.end());
    }
    return ret;
  }

  inline std::vector<char> string_to_vector_null(const std::string &str)
  {
    std::vector<char> ret(str.begin(), str.end());
    ret.push_back(0);
    return ret;
  }
}