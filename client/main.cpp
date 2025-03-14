#include "SFML/Window/Event.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>

#include "SFML/Network/IpAddress.hpp"
#include <SFML/Network.hpp>

#include <SFML/Window/Window.hpp>
#include <cmath>
#include <cstdlib>
#include <iostream>
#include <limits>
#include <vector>

#include "API.h"
#include "world.h"

class Entity {
public:
  Entity(const Entity &) = default;
  Entity(Entity &&) = default;
  Entity &operator=(const Entity &) = default;
  Entity &operator=(Entity &&) = default;
  Entity(sf::Vector2f position, float size, sf::Color color)
    : m_position(position),
      m_velocity(0.001, 0),
      m_shape(size),
      m_mass(sf::priv::pi * size * size)
  {
    m_shape.setFillColor(color);
  }

  void draw(sf::RenderWindow *window)
  {
    window->draw(m_shape);
  }

  void tick(float delta_time, sf::Vector2f accelaration)
  {
    assert(delta_time > 0);

    // Input Acceleration against mass
    m_velocity += accelaration * m_mass * delta_time;

    // Gravity
    m_velocity.y += WORLD_GRAVITY * delta_time;

    // Drag
    m_velocity += -m_velocity * m_velocity.length() * DRAG_COEF *
                  (float)m_shape.getLocalBounds().size.x;

    // Velocity
    m_position += m_velocity * delta_time;

    const float height_from_ground =
        WORLD_HEIGHT - m_position.y - m_shape.getLocalBounds().size.y;
    if (height_from_ground <= 0) {
      m_velocity.y *= -1 * ELASTICITY;
      m_position.y += height_from_ground;
    }

    m_shape.setPosition(m_position);
  }

private:
  sf::Vector2f m_position;
  sf::Vector2f m_velocity;
  float m_mass;
  sf::CircleShape m_shape;

  // Rather than terminal velocuty here, air resistance from size causing drag
  static constexpr float DRAG_COEF = 1.0e-8f;
  static constexpr float ELASTICITY = 0.9f;
};

std::vector<Entity> entities{
    {{100.0f, 500.0f}, 10, sf::Color::Cyan},
    {{350.0f, 000.0f}, 1, sf::Color::Yellow},
    {{100.0f, 100.0f}, 20, sf::Color::Green},
    {{700.0f, 1.0f}, 50, sf::Color::Red}
};

void tick(float delta_time)
{
  for (auto &e : entities) {
    e.tick(delta_time, {0.0, 0.0});
  }
}

void render(sf::RenderWindow *window)
{
  for (auto &e : entities) {
    e.draw(window);
  }
  window->display();
}

void test_network()
{
  Connection c(*sf::IpAddress::getLocalAddress(), 8080);

  Version v{1, 1, 1};

  auto msg = Serialise(v);
  auto s = c.Send(MessageID::Version, Serialise(v));
  if (!s) {
    std::cout << "AHH! Failure Sending!" << std::endl;
  }
}

int main()
{
  test_network();

  sf::RenderWindow window(sf::VideoMode({800, 600}), "SSPGE");
  // window.setVerticalSyncEnabled(true);

  sf::Clock clock;
  float delta_time;

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    while (const std::optional event = window.pollEvent()) {
      // "close requested" event: we close the window
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }
    }
    window.clear();

    delta_time = std::min(clock.restart().asSeconds(), 1.0f / 60.0f);
    tick(delta_time);
    render(&window);
  }
}
