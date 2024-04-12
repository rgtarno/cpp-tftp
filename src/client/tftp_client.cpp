
#include "client/tftp_client.hpp"

#include <filesystem>
#include <fstream>
#include <poll.h>

#include "common/debug_macros.hpp"
#include "common/tftp.hpp"
#include "common/utils.hpp"

//========================================================
void tftp_client::get_file(const std::string &filename, const std::string &tftp_server, const tftp::mode_t mode,
                           const std::string &local_interface)
{
  udp_connection udp;
  udp.bind(local_interface, 0);

  const tftp::rw_packet_t request(filename, tftp::packet_t::READ, mode);
  const auto              request_data = tftp::serialise_rw_packet(request);

  udp.send_to(tftp_server, 69, request_data);
  dbg_dbg("Sent request to {}:{} to read file '{}'", tftp_server, 69, filename);

  pollfd pfd = {
      .fd      = udp.sd(),
      .events  = POLLOUT,
      .revents = 0,
  };

  if ((poll(&pfd, 1, 3000) <= 0) || !(pfd.revents & POLLOUT))
  {
    dbg_warn("Did not receive reply to read request");
    return;
  }

  std::string addr;
  uint16_t    server_tid        = 0;
  uint16_t    block_number      = 1;
  const auto  first_packet_data = udp.recv_from(addr, server_tid, tftp::DATA_PKT_MAX_SIZE);
  auto        data_packet       = tftp::deserialise_data_packet(first_packet_data);
  dbg_dbg("Server tid is {}", server_tid);

  if (!data_packet)
  {
    const auto error_packet = tftp::deserialise_error_packet(first_packet_data);
    if (error_packet)
    {
      dbg_err("Server replied with error : {}", error_packet->error_msg);
    }
    else
    {
      dbg_err("Unknown error : Failed to parse packet");
    }
    return;
  }

  if (data_packet->block_number != block_number)
  {
    dbg_err("Received unexpected block number from 1st data package ({})", data_packet->block_number);
    return;
  }
  dbg_trace("Received block {} of {} bytes", data_packet->block_number, data_packet->data.size());

  const std::filesystem::path out_filename(filename);
  std::ofstream               out_file(out_filename.filename(), std::ios_base::binary);
  if (!out_file)
  {
    dbg_err("Failed to open file for writing '{}'", out_filename.c_str());
    return;
  }

  dbg_trace("Server tid is {}", server_tid);
  udp.connect(tftp_server, server_tid);

  tftp::ack_packet_t ack(block_number);

  while (true)
  {
    dbg_trace("Sending ack to block {}", block_number);
    udp.send(tftp::serialise_ack_packet(ack));
    ack.block_number = ++block_number;

    if (request.mode == tftp::mode_t::NETASCII)
    {
      const auto native_data = utils::netascii_to_native(data_packet->data);
      out_file.write(native_data.data(), native_data.size());
    }
    else
    {
      out_file.write(data_packet->data.data(), data_packet->data.size());
    }

    if (data_packet->data.size() < tftp::DATA_PKT_DATA_MAX_SIZE)
    {
      dbg_trace("Final block is {} ({} bytes)", block_number - 1, data_packet->data.size());
      break;
    }

    if (!poll(&pfd, 1, 3000) || !(pfd.revents & POLLOUT))
    {
      dbg_warn("Timed out waiting for reply, expected block number {}", block_number);
      return;
    }

    data_packet = tftp::deserialise_data_packet(udp.recv(tftp::DATA_PKT_MAX_SIZE));

    if (!data_packet)
    {
      dbg_err("Failed to parse data packet at block number {}", block_number);
      return;
    }

    if (data_packet->block_number != block_number)
    {
      dbg_err("Received unexpected block number ({}) expected {}", data_packet->block_number, block_number);
      return;
    }
    dbg_trace("Received block {} of {} bytes", data_packet->block_number, data_packet->data.size());
  }
}

//========================================================
void tftp_client::send_file(const std::string &filename, const std::string &tftp_server, const tftp::mode_t mode,
                            const std::string &local_interface)
{
  udp_connection udp;
  udp.bind(local_interface, 0);

  const tftp::rw_packet_t request(filename, tftp::packet_t::WRITE, mode);
  const auto              request_data = tftp::serialise_rw_packet(request);

  udp.send_to(tftp_server, 69, request_data);
  dbg_dbg("Sent request to {}:{} to write file '{}'", tftp_server, 63, filename);

  pollfd pfd = {
      .fd      = udp.sd(),
      .events  = POLLOUT,
      .revents = 0,
  };

  if (!poll(&pfd, 1, 3000) || !(pfd.revents & POLLOUT))
  {
    dbg_warn("Did not receive reply to write request");
    return;
  }

  std::string addr;
  uint16_t    server_tid        = 0;
  uint16_t    block_number      = 0;
  const auto  first_packet_data = udp.recv_from(addr, server_tid, tftp::DATA_PKT_MAX_SIZE);
  auto        ack_packet        = tftp::deserialise_ack_packet(first_packet_data);
  if (!ack_packet)
  {
    const auto error_packet = tftp::deserialise_error_packet(first_packet_data);
    if (error_packet)
    {
      dbg_err("Server replied with error : {}", error_packet->error_msg);
    }
    else
    {
      dbg_err("Unknown error : Failed to parse packet");
    }
    return;
  }

  if (ack_packet->block_number != block_number)
  {
    dbg_err("Received unexpected block number ({}) expected {}", ack_packet->block_number, block_number);
    return;
  }
  ++block_number;

  dbg_trace("Server tid is {}", server_tid);
  udp.connect(tftp_server, server_tid);

  tftp::data_packet_t data_packet;
  data_packet.block_number = block_number;
  data_packet.data.resize(512);

  std::ifstream in_file(filename, std::ios_base::binary);
  if (!in_file)
  {
    dbg_err("Failed to open file for reading '{}'", filename);
    return;
  }

  // Pre read in first block
  in_file.read(data_packet.data.data(), 512);
  if (!in_file)
  {
    dbg_err("Read error occured on file '{}'", filename);
    return;
  }
  data_packet.data.resize(in_file.gcount());

  if (request.mode == tftp::mode_t::NETASCII)
  {
    data_packet.data = utils::native_to_netascii(data_packet.data);
  }

  // Transfer loop
  bool finished = false;
  while (!finished)
  {
    if (data_packet.data.size() < tftp::DATA_PKT_DATA_MAX_SIZE)
    {
      dbg_trace("Sending final block {} ({} bytes)", block_number, data_packet.data.size());
      finished = true;
    }
    else
    {
      dbg_trace("Sending data packet, block {}", block_number);
    }

    udp.send(tftp::serialise_data_packet(data_packet));
    data_packet.block_number = ++block_number;

    // Read in block before we wait for ACK
    in_file.read(data_packet.data.data(), 512);
    if (!in_file && !in_file.eof())
    {
      dbg_err("Read error occured on file '{}'", filename);
      return;
    }
    data_packet.data.resize(in_file.gcount());

    if (request.mode == tftp::mode_t::NETASCII)
    {
      data_packet.data = utils::native_to_netascii(data_packet.data);
    }

    // Wait for ACK
    if (!poll(&pfd, 1, 3000) || !(pfd.revents & POLLOUT))
    {
      dbg_warn("Timed out waiting for reply, expected block number {}", block_number);
      return;
    }

    ack_packet = tftp::deserialise_ack_packet(udp.recv(tftp::ACK_PKT_MAX_SIZE));
    if (!ack_packet)
    {
      dbg_err("Failed to parse ack packet at block number {}", block_number);
      return;
    }

    if (data_packet.block_number != block_number)
    {
      dbg_err("Received unexpected block number ({}) expected {}", data_packet.block_number, block_number);
      return;
    }
  }
}