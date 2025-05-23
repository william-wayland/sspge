#pragma once

#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RectangleShape.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/Graphics/Shape.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>
#include <SFML/Window/Event.hpp>

class Entity {
public:
  Entity(const Entity &) = default;
  Entity(Entity &&) = default;
  Entity(
      sf::Vector2f position, sf::Vector2f velocity, float size, float density,
      sf::Color color
  )
    : m_position(position),
      m_velocity(velocity),
      m_mass(sf::priv::pi * size * size * density),
      m_size(size),
      m_shape(size),
      m_id([] {
        static int id = 0;
        return id++;
      }()) {
    m_shape.setFillColor(color);
  }

  // stores the forces until tick
  void push(sf::Vector2f force) {
    m_force += force;
  }

  void draw(sf::RenderWindow *window, sf::Vector2f offset = {0, 0}) {
    m_shape.setPosition(m_position + offset);
    window->draw(m_shape);
  }

  void tick(float delta_time) {
    assert(delta_time > 0);

    // Velocity changes by acceleration (here force/mass)
    m_velocity += (m_force / m_mass) * delta_time;
    m_force = {0, 0};

    // Velocity
    m_position += m_velocity * delta_time;
  }

  float mass() {
    return m_mass;
  }

  sf::Vector2f &position() {
    return m_position;
  }

  sf::Vector2f center() const {
    return m_position + (m_shape.getLocalBounds().size / 2.0f);
  }

  void set_center(sf::Vector2f center) {
    m_position = center - (m_shape.getLocalBounds().size / 2.0f);
  }

  sf::Vector2f &velocity() {
    return m_velocity;
  }

  void set_velocity(sf::Vector2f velocity) {
    m_velocity = velocity;
  }

  float radius() const {
    return m_shape.getRadius();
  }

  float diameter() {
    return 2 * m_shape.getRadius();
  }

  int id() {
    return m_id;
  }

  bool collides(const Entity &e) {
    return (center() - e.center()).length() < radius() + e.radius();
  }

  float size() {
    return m_size;
  }

  sf::CircleShape &shape() {
    return m_shape;
  }

private:
  sf::Vector2f m_force;
  sf::Vector2f m_position;
  sf::Vector2f m_velocity;
  float m_mass;
  float m_size; // essentially the radius
  sf::CircleShape m_shape;

  const int m_id;
};
