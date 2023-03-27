

#include "catch.hpp"

#include "test_utils.hpp"
#include "utils.hpp"

TEST_CASE("Extract std::string's from a char buffer of null terminated c strings")
{
  SECTION("2 strings")
  {
    SECTION("Typical filename")
    {

      const std::string                    fname   = "test_file.txt";
      const std::string                    mode    = "OCTET";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 2);
      REQUIRE(ret[0] == fname);
      REQUIRE(ret[1] == mode);
    }

    SECTION("Filename with path")
    {

      const std::string                    fname   = "path/to/a/deeply/nested/file/test_file.txt";
      const std::string                    mode    = "OCTET";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 2);
      REQUIRE(ret[0] == fname);
      REQUIRE(ret[1] == mode);
    }

    SECTION("Long filename")
    {

      const std::string fname = "very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_very_"
                                "very_very_very_very_long_test_file.txt";
      const std::string mode  = "OCTET";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 2);
      REQUIRE(ret[0] == fname);
      REQUIRE(ret[1] == mode);
    }

    SECTION("Zero length filename")
    {

      const std::string                    fname   = "";
      const std::string                    mode    = "OCTET";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 2);
      REQUIRE(ret[0] == fname);
      REQUIRE(ret[1] == mode);
    }

    SECTION("Zero length mode")
    {

      const std::string                    fname   = "test_file";
      const std::string                    mode    = "";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 2);
      REQUIRE(ret[0] == fname);
      REQUIRE(ret[1] == mode);
    }

    SECTION("Zero length filename and mode")
    {

      const std::string                    fname   = "";
      const std::string                    mode    = "";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 2);
      REQUIRE(ret[0] == fname);
      REQUIRE(ret[1] == mode);
    }
  }

  SECTION("Many strings")
  {
    SECTION("4 non empty strings")
    {

      const std::string                    a       = "first string";
      const std::string                    b       = "second string";
      const std::string                    c       = "third string";
      const std::string                    d       = "fourth string";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(a), string_to_vector_null(b),
                                                      string_to_vector_null(c), string_to_vector_null(d)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 4);
      REQUIRE(ret[0] == a);
      REQUIRE(ret[1] == b);
      REQUIRE(ret[2] == c);
      REQUIRE(ret[3] == d);
    }

    SECTION("4 non empty strings with an offset at start of buffer")
    {

      const std::string                    a           = "first string";
      const std::string                    b           = "second string";
      const std::string                    c           = "third string";
      const std::string                    d           = "fourth string";
      const std::vector<std::vector<char>> strings     = {string_to_vector_null(a), string_to_vector_null(b),
                                                          string_to_vector_null(c), string_to_vector_null(d)};
      std::vector<char>                    data        = join_vectors(strings);
      const size_t                         OFFSET_SIZE = 11;
      std::vector<char>                    offset_dat(OFFSET_SIZE, 't');
      data.insert(data.begin(), offset_dat.begin(), offset_dat.end());

      const auto ret = utils::extract_c_strings_from_buffer(data, OFFSET_SIZE);

      REQUIRE(ret.size() == 4);
      REQUIRE(ret[0] == a);
      REQUIRE(ret[1] == b);
      REQUIRE(ret[2] == c);
      REQUIRE(ret[3] == d);
    }

    SECTION("6 strings empty and non empty")
    {

      const std::string a = "first string";
      const std::string b = "second string";
      const std::string c = "";
      const std::string d = "fourth string";
      const std::string e = "a long strin ga;dshfjashdfjashdfjkasdlhfljkashdfa7394875349874398";
      const std::string f = "555555555555555555555555555555555555555555555555555555555555555555555555555555555";
      const std::vector<std::vector<char>> strings = {string_to_vector_null(a), string_to_vector_null(b),
                                                      string_to_vector_null(c), string_to_vector_null(d),
                                                      string_to_vector_null(e), string_to_vector_null(f)};
      const std::vector<char>              data    = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == 6);
      REQUIRE(ret[0] == a);
      REQUIRE(ret[1] == b);
      REQUIRE(ret[2] == c);
      REQUIRE(ret[3] == d);
      REQUIRE(ret[4] == e);
      REQUIRE(ret[5] == f);
    }

    SECTION("many empty strings")
    {
      const size_t                   NUM_STRINGS = 50;
      std::vector<std::vector<char>> strings;
      for (size_t i = 0; i < NUM_STRINGS; ++i)
      {
        strings.push_back(string_to_vector_null(""));
      }
      const std::vector<char> data = join_vectors(strings);

      const auto ret = utils::extract_c_strings_from_buffer(data);

      REQUIRE(ret.size() == NUM_STRINGS);
      for (size_t i = 0; i < NUM_STRINGS; ++i)
      {
        REQUIRE(ret[i] == "");
      }
    }
  }
}
