

#include <gtest/gtest.h>

#include "common/tftp.hpp"
#include "tests/test_utils.hpp"

TEST(tftp_serdes_tests, good_rw_packet_octect)
{
  const std::vector<char> data = {0x00, 0x01, '/', 'r', 'o', 'o', 't', '/', 'd',
                                  'i', 'r', '\0', 'o', 'c', 't', 'e', 't', '\0'};

  const auto packet = tftp::deserialise_rw_packet(data);

  EXPECT_TRUE(packet.has_value());
  EXPECT_EQ(packet->filename, "/root/dir");
  EXPECT_EQ(packet->mode, tftp::mode_t::OCTET);
}

TEST(tftp_serdes_tests, good_rw_packet_netascii)
{
  const std::vector<char> data = {0x00, 0x01, '/', 'r', 'o', 'o', 't', '/', 'd', 'i', 'r',
                                  '\0', 'n', 'e', 't', 'a', 's', 'c', 'i', 'i', '\0'};

  const auto packet = tftp::deserialise_rw_packet(data);

  EXPECT_TRUE(packet.has_value());
  EXPECT_EQ(packet->filename, "/root/dir");
  EXPECT_EQ(packet->mode, tftp::mode_t::NETASCII);
}

TEST(tftp_serdes_tests, rw_packet_with_wrong_packet_type)
{
  const std::vector<char> data = {0x00, 0x00, '/', 'r', 'o', 'o', 't', '/', 'd',
                                  'i', 'r', '\0', 'o', 'c', 't', 'e', 't', '\0'};

  const auto packet = tftp::deserialise_rw_packet(data);

  EXPECT_FALSE(packet.has_value());
}

TEST(tftp_serdes_tests, rw_packet_with_invalid_mode)
{
  const std::vector<char> data = {0x00, 0x01, '/', 'r', 'o', 'o', 't', '/', 'd', 'i', 'r',
                                  '\0', 'n', 'o', 't', 'a', 'm', 'o', 'd', 'e', '\0'};

  const auto packet = tftp::deserialise_rw_packet(data);

  EXPECT_FALSE(packet.has_value());
}

TEST(tftp_serdes_tests, rw_packet_no_mode)
{
  const std::vector<char> data = {0x00, 0x01, '/', 'r', 'o', 'o', 't', '/', 'd', 'i', 'r', '\0'};

  const auto packet = tftp::deserialise_rw_packet(data);

  EXPECT_FALSE(packet.has_value());
}

TEST(tftp_serdes_tests, rw_packet_no_filepath)
{
  const std::vector<char> data = {0x00, 0x01, 'o', 'c', 't', 'e', 't'};

  const auto packet = tftp::deserialise_rw_packet(data);

  EXPECT_FALSE(packet.has_value());
}

TEST(tftp_serdes_tests, rw_packet_with_1_options_pair)
{
  const std::string                    filename = "/root/path/to/a/directory";
  const std::string                    mode     = "netascii";
  const std::string                    opt1_key = "option_one";
  const std::string                    opt1_val = "test value";
  const std::vector<std::vector<char>> strings  = {{0x00, 0x01},
                                                  string_to_vector_null(filename),
                                                  string_to_vector_null(mode),
                                                  string_to_vector_null(opt1_key),
                                                  string_to_vector_null(opt1_val)};
  const std::vector<char>              payload  = join_vectors(strings);

  const auto packet = tftp::deserialise_rw_packet(payload);

  auto uppercase = [](const std::string &input) -> std::string {
    auto ret = input;
    std::transform(ret.begin(), ret.end(),
                   ret.begin(), ::toupper);
    return ret;
  };

  EXPECT_TRUE(packet.has_value());
  EXPECT_EQ(packet->filename, filename);
  EXPECT_EQ(packet->mode, tftp::mode_t::NETASCII);
  EXPECT_EQ(packet->options.size(), 1);
  EXPECT_EQ(packet->options.at(0).first, uppercase(opt1_key));
  EXPECT_EQ(packet->options.at(0).second, uppercase(opt1_val));
}

TEST(tftp_serdes_tests, rw_packet_with_3_options_pair)
{
  const std::string                    filename = "/root/path/to/a/directory";
  const std::string                    mode     = "netascii";
  const std::string                    opt1_key = "option_one";
  const std::string                    opt1_val = "test value";
  const std::string                    opt2_key = "option_two";
  const std::string                    opt2_val = "A LONG OPTION VALUE";
  const std::string                    opt3_key = "option_tree";
  const std::string                    opt3_val = "0xCAFEBABE";
  const std::vector<std::vector<char>> strings  = {{0x00, 0x01},
                                                  string_to_vector_null(filename),
                                                  string_to_vector_null(mode),
                                                  string_to_vector_null(opt1_key),
                                                  string_to_vector_null(opt1_val),
                                                  string_to_vector_null(opt2_key),
                                                  string_to_vector_null(opt2_val),
                                                  string_to_vector_null(opt3_key),
                                                  string_to_vector_null(opt3_val)};
  const std::vector<char>              payload  = join_vectors(strings);

  const auto packet = tftp::deserialise_rw_packet(payload);

  auto uppercase = [](const std::string &input) -> std::string {
    auto ret = input;
    std::transform(ret.begin(), ret.end(),
                   ret.begin(), ::toupper);
    return ret;
  };

  EXPECT_TRUE(packet.has_value());
  EXPECT_EQ(packet->filename, filename);
  EXPECT_EQ(packet->mode, tftp::mode_t::NETASCII);
  EXPECT_EQ(packet->options.size(), 3);
  EXPECT_EQ(packet->options.at(0).first, uppercase(opt1_key));
  EXPECT_EQ(packet->options.at(0).second, uppercase(opt1_val));
  EXPECT_EQ(packet->options.at(1).first, uppercase(opt2_key));
  EXPECT_EQ(packet->options.at(1).second, uppercase(opt2_val));
  EXPECT_EQ(packet->options.at(2).first, uppercase(opt3_key));
  EXPECT_EQ(packet->options.at(2).second, uppercase(opt3_val));
}