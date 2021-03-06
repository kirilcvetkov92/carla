// Copyright (c) 2017 Computer Vision Center (CVC) at the Universitat Autonoma
// de Barcelona (UAB).
//
// This work is licensed under the terms of the MIT license.
// For a copy, see <https://opensource.org/licenses/MIT>.

#pragma once

#include "carla/Debug.h"
#include "carla/streaming/Token.h"
#include "carla/streaming/detail/Types.h"

#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/ip/udp.hpp>

namespace carla {
namespace streaming {
namespace detail {

#pragma pack(push, 1)

  struct token_data {
    stream_id_type stream_id;

    uint16_t port;

    enum class protocol : uint8_t {
      not_set,
      tcp,
      udp
    } protocol = protocol::not_set;

    enum class address : uint8_t {
      not_set,
      ip_v4,
      ip_v6
    } address_type = address::not_set;

    union {
      boost::asio::ip::address_v4::bytes_type v4;
      boost::asio::ip::address_v6::bytes_type v6;
    } address;
  };

#pragma pack(pop)

  static_assert(
      sizeof(token_data) == sizeof(Token::data),
      "Size shouldn't be more than"
      "  v6 address  : 128"
      "  + state     :  16"
      "  + port      :  16"
      "  + stream id :  32"
      "  -----------------"
      "                192");

  /// Serializes a stream endpoint. Contains all the necessary information for a
  /// client to subscribe to a stream.
  class token_type {
  private:

    template <typename P>
    static constexpr auto get_protocol() {
      return std::is_same<P, boost::asio::ip::tcp>::value ?
          token_data::protocol::tcp :
          token_data::protocol::udp;
    }

    void set_address(const boost::asio::ip::address &addr);

    boost::asio::ip::address get_address() const;

    template <typename P>
    boost::asio::ip::basic_endpoint<P> get_endpoint() const {
      DEBUG_ASSERT(is_valid());
      DEBUG_ASSERT(get_protocol<P>() == _token.protocol);
      return {get_address(), _token.port};
    }

    template <typename P>
    explicit token_type(
        stream_id_type stream_id,
        const boost::asio::ip::basic_endpoint<P> &ep) {
      _token.stream_id = stream_id;
      _token.port = ep.port();
      _token.protocol = get_protocol<P>();
      set_address(ep.address());
    }

  public:

    token_type() = default;
    token_type(const token_type &) = default;

    token_type(const Token &rhs);

    operator Token() const;

    auto get_stream_id() const {
      return _token.stream_id;
    }

    auto get_port() const {
      return _token.port;
    }

    bool is_valid() const {
      return ((_token.protocol != token_data::protocol::not_set) &&
             (_token.address_type != token_data::address::not_set));
    }

    bool address_is_v4() const {
      return _token.address_type == token_data::address::ip_v4;
    }

    bool address_is_v6() const {
      return _token.address_type == token_data::address::ip_v6;
    }

    bool protocol_is_udp() const {
      return _token.protocol == token_data::protocol::udp;
    }

    bool protocol_is_tcp() const {
      return _token.protocol == token_data::protocol::tcp;
    }

    boost::asio::ip::udp::endpoint to_udp_endpoint() const {
      return get_endpoint<boost::asio::ip::udp>();
    }

    boost::asio::ip::tcp::endpoint to_tcp_endpoint() const {
      return get_endpoint<boost::asio::ip::tcp>();
    }

  private:

    friend class Dispatcher;

    token_data _token;
  };

} // namespace detail
} // namespace streaming
} // namespace carla
