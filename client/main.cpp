#include "SFML/Window/Event.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>

#include "SFML/Network/IpAddress.hpp"
#include <SFML/Network.hpp>

#include <SFML/Window/Window.hpp>
#include <iostream>

#include "easylogging++.h"
INITIALIZE_EASYLOGGINGPP

#include "API.h"

void test_network() {
  Connection c(*sf::IpAddress::getLocalAddress(), 8080);

  Version v{1, 1, 1};

  auto msg = Serialise(v);
  auto s = c.Send(MessageID::Version, Serialise(v));
  if (!s) {
    std::cout << "AHH! Failure Sending!" << std::endl;
  }
}

void tick() {}

void render(sf::Window &window) {}

int main() {

  test_network();

  sf::Window window(sf::VideoMode({800, 600}), "SSPGE");
  window.setVerticalSyncEnabled(true);

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    while (const std::optional event = window.pollEvent()) {
      // "close requested" event: we close the window
      if (event->is<sf::Event::Closed>())
        window.close();
    }

    tick();
    render(window);
  }
}
