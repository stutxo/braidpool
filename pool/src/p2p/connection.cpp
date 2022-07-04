/**
 * Copyright (c) 2021 braidpool developers (see AUTHORS)
 *
 * This file is part of braidpool.
 *
 * braidpool is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * braidpool is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with braidpool.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "p2p/connection.hpp"

#include <boost/asio/awaitable.hpp>
#include <boost/asio/detached.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/thread.hpp>
#include <iostream>
#include <memory>
#include <p2p/define.hpp>

#include "system.hpp"
#include "util/log.hpp"

using boost::asio::buffer;
using boost::asio::detached;
using boost::asio::use_awaitable;

namespace bp {
namespace p2p {

connection::connection(tcp::socket&& sock, connections_mgr& manager)
    : socket_(std::move(sock)), mgr_(manager) {
  LOG_DEBUG << "Connection created...";
}

connection::~connection() {
  LOG_DEBUG << "Connection destroyed...";
  shutdown();
}

awaitable<void> connection::send_to_peer(std::string message) {
  try {
    LOG_INFO << "Sending: " << message;
    co_await async_write(socket_, buffer(message), use_awaitable);
  } catch (const std::exception& e) {
    LOG_DEBUG << "failing to send to peer";
    LOG_DEBUG << e.what();
    shutdown();
    co_return;
  }
}

void connection::start() {
  LOG_DEBUG << "Starting receive...";
  co_spawn(socket_.get_executor(), receive_from_peer(), detached);
}

awaitable<void> connection::receive_from_peer() {
  try {
    for (std::string read_msg;;) {
      LOG_DEBUG << "In read loop";
      auto num_bytes_read = co_await boost::asio::async_read_until(
          socket_, boost::asio::dynamic_buffer(read_msg, 1024), "\r\n",
          use_awaitable);
      LOG_INFO << "Received: " << read_msg;

      if (read_msg == "ping\r\n") {
        co_await send_to_peer("pong\r\n");
      }

      read_msg.erase(0, num_bytes_read);
    }
  } catch (const std::exception& e) {
    LOG_DEBUG << "failing to receive from peer";
    LOG_DEBUG << e.what();
    shutdown();
    co_return;
  }
}

void connection::shutdown() {
  LOG_DEBUG << "Connection shutdown...";
  if (socket_.is_open()) {
    LOG_DEBUG << "Closing socket...";
    socket_.close();
    LOG_DEBUG << "Socket closed...";
    // mgr_.remove_connection(boost::shared_from_this());
    LOG_DEBUG << "Removed connection...";
  }
}

}  // namespace p2p
}  // namespace bp
