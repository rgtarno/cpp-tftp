
#include <gtest/gtest.h>

#include "common/utils.hpp"
#include "tests/test_utils.hpp"

using namespace test_utils;

TEST(extract_c_strings_from_buffer, empty)
{
    const std::vector<char> data{};
    
    const auto ret = utils::extract_c_strings_from_buffer(data);
    EXPECT_EQ(ret.size(), 0);
}

TEST(extract_c_strings_from_buffer, one_string)
{
    const std::string                    fname   = "test_file.txt";
    const std::vector<char>              data    = string_to_vector_null(fname);

    const auto ret = utils::extract_c_strings_from_buffer(data);

    EXPECT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], fname);
}

TEST(extract_c_strings_from_buffer, two_strings)
{
    const std::string                    fname   = "test_file.txt";
    const std::string                    mode    = "OCTET";
    const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
    const std::vector<char>              data    = join_vectors(strings);

    const auto ret = utils::extract_c_strings_from_buffer(data);

    EXPECT_EQ(ret.size(), 2);
    EXPECT_EQ(ret[0], fname);
    EXPECT_EQ(ret[1], mode);
}

TEST(extract_c_strings_from_buffer, two_strings_one_zero_length)
{
    const std::string                    fname   = "test_file.txt";
    const std::string                    mode    = "";
    const std::vector<std::vector<char>> strings = {string_to_vector_null(fname), string_to_vector_null(mode)};
    const std::vector<char>              data    = join_vectors(strings);

    const auto ret = utils::extract_c_strings_from_buffer(data);

    EXPECT_EQ(ret.size(), 2);
    EXPECT_EQ(ret[0], fname);
    EXPECT_EQ(ret[1], mode);
}

TEST(extract_c_strings_from_buffer, two_strings_one_missing_null)
{
    const std::vector<char> data = {'t', 'h', 'i', 's', ' ', 'i', 's', '\0', 'a', ' ', 't', 'e', 's', 't'};

    const auto ret = utils::extract_c_strings_from_buffer(data);

    EXPECT_EQ(ret.size(), 1);
    EXPECT_EQ(ret[0], std::string("this is"));
}

TEST(extract_c_strings_from_buffer, three_strings_with_offset_at_start)
{
  const std::string                    a           = "first string";
  const std::string                    b           = "second string";
  const std::string                    c           = "third string";
  const std::string                    d           = "fourth string";
  const std::vector<std::vector<char>> strings     = {string_to_vector_null(a), string_to_vector_null(b),
                                                      string_to_vector_null(c), string_to_vector_null(d)};
  std::vector<char>                    data        = join_vectors(strings);
  const size_t                         OFFSET_SIZE = 13;
  std::vector<char>                    offset_dat(OFFSET_SIZE, 't');
  data.insert(data.begin(), offset_dat.begin(), offset_dat.end());

  const auto ret = utils::extract_c_strings_from_buffer(data, OFFSET_SIZE);

  EXPECT_EQ(ret.size(),  4);
  EXPECT_EQ(ret[0],  a);
  EXPECT_EQ(ret[1],  b);
  EXPECT_EQ(ret[2],  c);
  EXPECT_EQ(ret[3],  d);
}

TEST(extract_c_strings_from_buffer, many_empty_strings)
{
  const size_t                   NUM_STRINGS = 50;
  std::vector<std::vector<char>> strings;
  for (size_t i = 0; i < NUM_STRINGS; ++i)
  {
    strings.push_back(string_to_vector_null(""));
  }
  const std::vector<char> data = join_vectors(strings);

  const auto ret = utils::extract_c_strings_from_buffer(data);

  EXPECT_EQ(ret.size(), NUM_STRINGS);
  for (size_t i = 0; i < NUM_STRINGS; ++i)
  {
    EXPECT_EQ(ret[i], "");
  }
}

TEST(native_to_netascii, empty)
{
  const std::vector<char> input;
  const std::vector<char> ret =  utils::native_to_netascii(input);
  EXPECT_TRUE(ret.empty());
}

TEST(native_to_netascii, no_newline)
{
  const std::vector<char> input = string_to_vector_null("a string");
  const std::vector<char> ret =  utils::native_to_netascii(input);
  EXPECT_FALSE(ret.empty());
  EXPECT_EQ(ret, input);
}

TEST(native_to_netascii, yes_newline)
{
  const std::vector<char> input = string_to_vector_null("a string with a \n");
  const std::vector<char> ret =  utils::native_to_netascii(input);
  const std::vector<char> expected = string_to_vector_null("a string with a \r\n");
  EXPECT_FALSE(ret.empty());
  EXPECT_EQ(ret, expected);
}

TEST(native_to_netascii, multiple_newlines)
{
  const std::vector<char> input = string_to_vector_null("abc\ndef\n\n");
  const std::vector<char> ret =  utils::native_to_netascii(input);
  const std::vector<char> expected = string_to_vector_null("abc\r\ndef\r\n\r\n");
  EXPECT_FALSE(ret.empty());
  EXPECT_EQ(ret, expected);
}

TEST(netascii_to_native, empty)
{
  const std::vector<char> input;
  const std::vector<char> ret =  utils::netascii_to_native(input);
  EXPECT_TRUE(ret.empty());
}

TEST(netascii_to_native, no_newline)
{
  const std::vector<char> input = string_to_vector_null("a string");
  EXPECT_NO_THROW(
    const std::vector<char> ret =  utils::netascii_to_native(input);
    EXPECT_FALSE(ret.empty());
    EXPECT_EQ(ret, input);
  );
}

TEST(netascii_to_native, throw_on_standalone_cr)
{
  const std::vector<char> input = string_to_vector_null("a \rstring");
  EXPECT_THROW(
    const std::vector<char> ret =  utils::netascii_to_native(input);,
    std::runtime_error
  );
}

TEST(netascii_to_native, yes_newline)
{
  const std::vector<char> input = string_to_vector_null("a string with a \r\n");
  const std::vector<char> expected = string_to_vector_null("a string with a \n");
  EXPECT_NO_THROW(
    const std::vector<char> ret =  utils::netascii_to_native(input);
    EXPECT_FALSE(ret.empty());
    EXPECT_EQ(ret, expected);
  );
}

TEST(netascii_to_native, multinewline_and_cr)
{
  const std::vector<char> input = {'a','\r','\0',' ','s','t','r','i','n','g',' ','\r','\n',' ', 'w', 'i','t','h',' ','a',' ','\r','\n', '\0'};
  const std::vector<char> expected = string_to_vector_null("a\r string \n with a \n");
  EXPECT_NO_THROW(
    const std::vector<char> ret =  utils::netascii_to_native(input);
    EXPECT_FALSE(ret.empty());
    EXPECT_EQ(ret, expected);
  );
}

TEST(netascii_to_native, single_cr)
{
  const std::vector<char> input = {'\r'};
  EXPECT_THROW(
    const std::vector<char> ret =  utils::netascii_to_native(input);,
    std::runtime_error
  );
}