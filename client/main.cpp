#include "SFML/Window/Event.hpp"
#include <SFML/Graphics.hpp>

#include "SFML/Network/IpAddress.hpp"
#include <SFML/Network.hpp>

#include <iostream>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "API.h"

int main() {
  sf::UdpSocket socket;
  if (socket.bind(sf::Socket::AnyPort) == sf::Socket::Status::Error) {
    std::cout << "AHH! Failure!" << std::endl;
    return 0;
  }

  Version v{1, 1, 1};

  auto msg = Serialise(v);
  auto s = socket.send(msg.data(), msg.size() + 1,
                       *sf::IpAddress::getLocalAddress(), 8080);
  if (s == sf::Socket::Status::Error) {
    std::cout << "AHH! Failure Sending!" << std::endl;
  }
}