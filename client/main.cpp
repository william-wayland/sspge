#include "SFML/Window/Event.hpp"
#include <SFML/Graphics.hpp>
#include <SFML/Graphics/CircleShape.hpp>
#include <SFML/Graphics/Color.hpp>
#include <SFML/Graphics/RenderWindow.hpp>
#include <SFML/System/Angle.hpp>
#include <SFML/System/Clock.hpp>
#include <SFML/System/Time.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window.hpp>

#include "SFML/Network/IpAddress.hpp"
#include <SFML/Network.hpp>

#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Window.hpp>
#include <cmath>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <limits>
#include <random>
#include <vector>

#include "API.h"
#include "world.h"

// Rather than terminal velocity here, air resistance from size causing drag
// static constexpr float DRAG_COEF = 1.0e-8f;
static constexpr float ELASTICITY = 1.00f;

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

  void draw(sf::RenderWindow *window) {
    window->draw(m_shape);
  }

  void tick(float delta_time) {
    assert(delta_time > 0);

    // Velocity changes by accelleration (here force/mass)
    m_velocity += (m_force / m_mass) * delta_time;
    m_force = {0, 0};

    // World Gravity
    // m_velocity.y += WORLD_GRAVITY * delta_time;

    // Drag
    // m_velocity += -m_velocity * m_velocity.length() * DRAG_COEF *
    //               (float)m_shape.getLocalBounds().size.x;

    // Velocity
    m_position += m_velocity * delta_time;
    m_shape.setPosition(m_position);
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

private:
  sf::Vector2f m_force;
  sf::Vector2f m_position;
  sf::Vector2f m_velocity;
  float m_mass;
  sf::CircleShape m_shape;

  const int m_id;
};

struct State {
  std::vector<Entity> entities;
  uint64_t frame = 0;
  int bounce = 0;
  std::optional<float> energy = std::nullopt;
} state;

void collide_with_walls(Entity &e) {
  const float dist_from_ground = WORLD_HEIGHT - e.position().y - e.diameter();
  if (dist_from_ground <= 0) {
    e.velocity().y *= -1 * ELASTICITY;
    e.position().y = WORLD_HEIGHT - e.diameter();
  }

  if (e.position().y <= 0) {
    e.velocity().y *= -1 * ELASTICITY;
    e.position().y = 0;
  }

  if (e.position().x <= 0) {
    e.velocity().x *= -1 * ELASTICITY;
    e.position().x = 0;
    state.bounce += 1;
  }

  const float dist_from_right = WORLD_WIDTH - e.position().x - e.diameter();
  if (dist_from_right <= 0) {
    e.velocity().x *= -1 * ELASTICITY;
    e.position().x = WORLD_WIDTH - e.diameter();
  }
}

void collide_with_entity(Entity &a, Entity &b) {
  if (!a.collides(b))
    return;

  sf::Vector2f normal = (b.center() - a.center()).normalized();

  // move balls apart until they're no longer touching
  for (int i = 0; i < 100; i++) {
    float overlap =
        a.radius() + b.radius() - (a.center() - b.center()).length();
    if (overlap <= 0) {
      break;
    }

    float sa = a.velocity().length();
    float sb = b.velocity().length();
    if (sa == 0 && sb == 0) {
      std::cout << "Velocity is zero... no way to resolve overlap for now."
                << std::endl;
      return;
    }

    a.position() -= overlap * normal * sa / (sa + sb);
    b.position() += overlap * normal * sb / (sa + sb);
  }

  if (a.collides(b)) {
    std::cout << "Entities still are colliding after moving apart."
              << std::endl;
  }

  static uint64_t last_collision_frame = state.frame;
  if (last_collision_frame == state.frame - 1) {
    std::cout << "Maybe ignore this collision... too soon since last one."
              << std::endl;
    return;
  }
  last_collision_frame = state.frame;

  // inverse total mass
  const float itm = 1.0 / (a.mass() + b.mass());

  // the normal/tangent components of the collision.
  const sf::Vector2f tangent = normal.rotatedBy(sf::degrees(-90));
  const sf::Vector2f van = a.velocity().projectedOnto(normal);
  const sf::Vector2f vbn = b.velocity().projectedOnto(normal);
  const sf::Vector2f vat = a.velocity().projectedOnto(tangent);
  const sf::Vector2f vbt = b.velocity().projectedOnto(tangent);

  // Derived from conservation of momentum
  a.velocity() =
      (a.mass() - b.mass()) * itm * van + 2 * b.mass() * itm * vbn + vat;
  b.velocity() =
      (b.mass() - a.mass()) * itm * vbn + 2 * a.mass() * itm * van + vbt;
}

// returns the potential energy of the two entities
float gravity(Entity &a, Entity &b) {
  static constexpr float GRAVITY = 1e2;
  sf::Vector2f dist = a.center() - b.center();

  sf::Vector2f force =
      GRAVITY * a.mass() * b.mass() * dist.normalized() / dist.lengthSquared();

  a.push(-force);
  b.push(force);

  return GRAVITY * a.mass() * b.mass() / dist.length();
}

// Uniquely combines elements in v
template <typename T>
void combine(std::vector<T> &v, std::function<void(T &, T &)> f) {
  if (v.size() < 2)
    return;

  uint32_t j = 1;
  for (uint32_t i = 0; i < v.size() - 1;) {
    for (; j < v.size(); j++) {
      f(v[i], v[j]);
    }
    i++;
    j = i + 1;
  }
}

void tick(float delta_time) {
  float energy = 0;

  // kinetic
  for (Entity &e : state.entities) {
    energy += e.mass() * e.velocity().lengthSquared() * 0.5;
  }

  combine<Entity>(state.entities, [&](Entity &a, Entity &b) -> void {
    collide_with_entity(a, b);

    // Gravitational potential
    energy += gravity(a, b);
  });

  if (!state.energy) {
    state.energy = energy;
    std::cout << "Total Energy of System: " << energy << std::endl;
  } else if (*state.energy != energy) {
    // std::cout << "Energy of System Changed: "
    //           << (*state.energy - current_energy) << std::endl;
  }

  for (Entity &e : state.entities) {
    collide_with_walls(e);
    e.tick(delta_time);
  }
}

void render(sf::RenderWindow *window) {
  for (auto &e : state.entities) {
    e.draw(window);
  }
  window->display();
}

void test_network() {
  Connection c(*sf::IpAddress::getLocalAddress(), 8080);

  Version v{1, 1, 1};

  auto s = c.Send(MessageID::Version, Serialise(v));
  if (!s) {
    std::cout << "AHH! Failure Sending!" << std::endl;
  }
}

void reset() {
  state = State{};

  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_real_distribution<float> wg(100, WORLD_WIDTH - 100);
  std::uniform_real_distribution<float> hg(100, WORLD_HEIGHT - 100);
  std::uniform_real_distribution<float> sg(5, 20);
  std::uniform_real_distribution<float> dg(1.0f, 5.0f);
  std::uniform_int_distribution<int> cg(0, 255);

  for (int i = 0; i < 10; i++) {
    state.entities.emplace_back(
        sf::Vector2f{wg(e), hg(e)}, sf::Vector2f{0, 0}, sg(e), dg(e),
        sf::Color(cg(e), cg(e), cg(e))
    );
  }

  // state.entities.emplace_back(
  //     sf::Vector2f{150.0f, 200.0f}, sf::Vector2f{0, 0}, 50, 5,
  //     sf::Color::Green
  // );
  // state.entities.emplace_back(
  //     sf::Vector2f{300.0f, 290.0f}, sf::Vector2f{-50, 0}, 50, 5,
  //     sf::Color::Red
  // );
}

int main() {
  test_network();

  sf::RenderWindow window(sf::VideoMode({800, 600}), "SSPGE");
  // window.setVerticalSyncEnabled(true);

  sf::Clock clock;
  float delta_time;

  reset();

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    while (const std::optional event = window.pollEvent()) {
      // "close requested" event: we close the window
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (event->is<sf::Event::KeyPressed>()) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
          window.close();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
          reset();
        }
      }
    }
    window.clear();

    delta_time = std::min(clock.restart().asSeconds(), 1.0f / 60.0f);
    tick(delta_time);
    render(&window);
    state.frame += 1;
  }
}
