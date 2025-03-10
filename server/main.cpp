#include <SFML/System/Vector2.hpp>
#include <iostream>
#include <optional>

#include <SFML/Network.hpp>
#include <SFML/Network/IpAddress.hpp>
#include <SFML/Network/Socket.hpp>
#include <string>
#include <unordered_map>

#include "API.h"

struct Entity {
  // Tracking
  sf::IpAddress address;
  unsigned short port;
  uint64_t last_coms;

  // Data
  std::string name;
  sf::Vector2f position;
};

using EntityID = std::string;

std::unordered_map<std::string, Entity> entities;

int main(int argc, char *argv[]) {
  START_EASYLOGGINGPP(argc, argv);

  LOG(INFO) << "Server Started";

  sf::UdpSocket socket;
  if (socket.bind(8080) == sf::Socket::Status::Error) {
    std::cout << "AHH! Failure!" << std::endl;
    return 0;
  }

  std::cout << "Socket Used: " << socket.getLocalPort() << std::endl;

  // Receive a message from anyone
  std::array<byte, 1024> buffer;
  std::size_t received = 0;

  while (true) {
    std::optional<sf::IpAddress> sender;
    unsigned short port;
    auto s =
        socket.receive(buffer.data(), buffer.size(), received, sender, port);
    if (s == sf::Socket::Status::Done) {
      Version v = Deserialise<Version>({buffer.data(), received});
      std::cout << sender->toString() << " said: " << v.major << std::endl;
    }
  }
}