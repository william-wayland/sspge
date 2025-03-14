#pragma once

#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Packet.hpp>
#include <SFML/Network/Socket.hpp>
#include <SFML/Network/UdpSocket.hpp>
#include <SFML/Window/WindowEnums.hpp>
#include <cstdlib>
#include <span>
#include <type_traits>
#include <vector>

#include <SFML/Network.hpp>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "world.h"

using byte = uint8_t;
using bytes = std::vector<byte>;
using byte_span = std::span<byte>;

template <typename T> byte_span Serialise(T &&t) {
  static_assert(std::is_standard_layout_v<std::remove_reference_t<T>>);
  return {reinterpret_cast<byte *>(&t), sizeof(T)};
}

template <typename T> T Deserialise(const bytes &data) {
  static_assert(std::is_standard_layout_v<std::remove_reference_t<T>>);
  return *(reinterpret_cast<T *>(data.data()));
}

template <typename T> T Deserialise(std::span<byte> data) {
  static_assert(std::is_standard_layout_v<std::remove_reference_t<T>>);
  return *(reinterpret_cast<T *>(data.data()));
}

enum class MessageID : uint8_t {
  Ok = 0,
  Error,
  Version,
  StartSession,
  EntityID,
  UpdatePosition,
  World,
  QuitSession
};

struct Version {
  uint8_t major;
  uint8_t minor;
  uint8_t build;
};

struct Connection {
  Connection(sf::IpAddress address, uint32_t port)
      : socket(), address(address), port(port) {
    if (socket.bind(sf::Socket::AnyPort) == sf::Socket::Status::Error) {
      LOG(ERROR) << "Failure to bind UDP port. Exiting...";
      exit(-1);
    }
  }

  sf::UdpSocket socket;
  sf::IpAddress address;
  uint32_t port;

  // Messages are the format
  // ID
  template <typename T> bool Send(MessageID id, T &&data) {
    sf::Packet p;
    p << static_cast<uint8_t>(id);

    auto span = Serialise(data);
    p.append(span.data(), span.size());
    return socket.send(p, address, port) == sf::Socket::Status::Done;
  }
};

template <typename T> std::optional<T> Receive(sf::UdpSocket &s) {
  sf::Packet p;
  std::optional<sf::IpAddress> ip;
  unsigned short port;
  if (s.receive(p, ip, port) != sf::Socket::Status::Done) {
    return std::nullopt;
  }

  uint8_t id;
  p >> id;

  switch (static_cast<MessageID>(id)) {
  case MessageID::Version:
    ParseMessage<T>(p);
  case MessageID::Ok:
  case MessageID::Error:
  case MessageID::StartSession:
  case MessageID::EntityID:
  case MessageID::UpdatePosition:
  case MessageID::World:
  case MessageID::QuitSession:
    break;
  }
  return;
}

template <typename T> std::optional<T> ParseMessage(sf::UdpSocket &) {
  return std::nullopt;
}
