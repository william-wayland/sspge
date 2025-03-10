#include "SFML/Window/Event.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>

#include "SFML/Network/IpAddress.hpp"
#include <SFML/Network.hpp>

#include <SFML/Window/Window.hpp>
#include <iostream>

#include "API.h"

class Entity {
public:
  Entity(sf::Vector2f position, float size, sf::Color color)
      : m_position(position), m_shape(size) {
    m_shape.setFillColor(color);
  }

  void Draw(sf::RenderWindow *window) { window->draw(m_shape); }

  void Tick(sf::Vector2f accelaration) {
    m_velocity += accelaration;
    m_position += m_velocity;
    m_shape.setPosition(m_position);
  }

private:
  sf::Vector2f m_position;
  sf::Vector2f m_velocity;
  sf::CircleShape m_shape;
};

void test_network() {
  Connection c(*sf::IpAddress::getLocalAddress(), 8080);

  Version v{1, 1, 1};

  auto msg = Serialise(v);
  auto s = c.Send(MessageID::Version, Serialise(v));
  if (!s) {
    std::cout << "AHH! Failure Sending!" << std::endl;
  }
}

Entity e({100.0f, 100.0f}, 10, sf::Color::Cyan);

void tick() {

  // TODO: user input
  // TODO: enity management rather than just.. global state
  // TODO: Send user input to server, get state back
  // TODO: Entity created on the server side, client just renders state

  e.Tick({0, 0});
}

void render(sf::RenderWindow *window) {
  e.Draw(window);
  window->display();
}

int main() {

  test_network();

  sf::RenderWindow window(sf::VideoMode({800, 600}), "SSPGE");
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
    render(&window);
  }
}
