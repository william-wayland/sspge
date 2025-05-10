#include <imgui-SFML.h>
#include <imgui.h>

#include "SFML/Window/Event.hpp"
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

#include <SFML/Network.hpp>
#include <SFML/Network/IpAddress.hpp>

#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Window.hpp>

#include <functional>
#include <iostream>
#include <random>
#include <vector>

#include "API.h"
#include "world.h"

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

  void draw(sf::RenderWindow *window) {
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

struct State {
  std::vector<Entity> entities;
  uint64_t frame = 0;
  int bounce = 0;
  std::optional<float> energy = std::nullopt;
  sf::Vector2f center_of_mass;

  sf::Vector2f camera_position = {0.0, 0.0};

  bool enable_gravity = true;
  float gravity = 1e2;

  bool enable_walls = true;

  float elasticity = 1.0f;
  float drag = 0.0f;
} state;

void reset_state() {
  state.frame = 0;
  state.energy = std::nullopt;
  state.entities.clear();
}

void reset_small() {
  reset_state();

  std::random_device r;
  std::default_random_engine e(r());
  std::uniform_real_distribution<float> wg(100, WORLD_WIDTH - 100);
  std::uniform_real_distribution<float> hg(100, WORLD_HEIGHT - 100);
  std::uniform_real_distribution<float> sg(5, 20);
  std::uniform_real_distribution<float> dg(1.0f, 5.0f);
  std::uniform_int_distribution<int> cg(0, 255);

  for (int i = 0; i < 100; i++) {
    state.entities.emplace_back(
        sf::Vector2f{wg(e), hg(e)}, sf::Vector2f{0, 0}, sg(e), dg(e),
        sf::Color(cg(e), cg(e), cg(e))
    );
  }
}

void reset_big() {
  reset_state();

  state.entities.emplace_back(
      sf::Vector2f{150.0f, 200.0f}, sf::Vector2f{0, 0}, 50, 50, sf::Color::Green
  );
  state.entities.emplace_back(
      sf::Vector2f{300.0f, 290.0f}, sf::Vector2f{-50, 0}, 50, 50, sf::Color::Red
  );
}

void reset_orbit() {
  reset_state();

  state.entities.emplace_back(
      sf::Vector2f{450.0f, 450.0f}, sf::Vector2f{0, 0}, 50, 500,
      sf::Color::Green
  );
  state.entities.emplace_back(
      sf::Vector2f{250.0f, 450.0f}, sf::Vector2f{0, 1600}, 50, 0.05f,
      sf::Color::Red
  );
}

void drag_entity(Entity &e) {
  e.velocity() *= 1 - state.drag;
}

void collide_with_walls(Entity &e) {
  const float dist_from_ground = WORLD_HEIGHT - e.position().y - e.diameter();
  if (dist_from_ground <= 0) {
    e.velocity().y *= -1 * state.elasticity;
    e.position().y = WORLD_HEIGHT - e.diameter();
  }

  if (e.position().y <= 0) {
    e.velocity().y *= -1 * state.elasticity;
    e.position().y = 0;
  }

  if (e.position().x <= 0) {
    e.velocity().x *= -1 * state.elasticity;
    e.position().x = 0;
    state.bounce += 1;
  }

  const float dist_from_right = WORLD_WIDTH - e.position().x - e.diameter();
  if (dist_from_right <= 0) {
    e.velocity().x *= -1 * state.elasticity;
    e.position().x = WORLD_WIDTH - e.diameter();
  }
}

void collide_with_entity(Entity &a, Entity &b) {
  if (!a.collides(b))
    return;

  const sf::Vector2f normal = (b.center() - a.center()).normalized();
  const sf::Vector2f tangent = {-normal.y, normal.x};

  // move balls apart until they're no longer touching
  float overlap = a.radius() + b.radius() - (a.center() - b.center()).length();

  float sa = a.velocity().length();
  float sb = b.velocity().length();
  if (sa == 0 && sb == 0) {
    a.position() -= overlap * normal * 0.5f;
    b.position() += overlap * normal * 0.5f;
  } else {
    a.position() -= overlap * normal * sa / (sa + sb);
    b.position() += overlap * normal * sb / (sa + sb);
  }

  // inverse total mass
  const float itm = 1.0 / (a.mass() + b.mass());

  // the normal/tangent components of the collision.
  const sf::Vector2f van = a.velocity().projectedOnto(normal);
  const sf::Vector2f vbn = b.velocity().projectedOnto(normal);
  const sf::Vector2f vat = a.velocity().projectedOnto(tangent);
  const sf::Vector2f vbt = b.velocity().projectedOnto(tangent);

  // Derived from conservation of momentum
  a.velocity() = ((a.mass() - b.mass()) * itm * van + 2 * b.mass() * itm * vbn
                 ) * state.elasticity +
                 vat;
  b.velocity() = ((b.mass() - a.mass()) * itm * vbn + 2 * a.mass() * itm * van
                 ) * state.elasticity +
                 vbt;
}

// returns the potential energy of the two entities
float gravity(Entity &a, Entity &b) {
  sf::Vector2f dist = a.center() - b.center();

  sf::Vector2f force = state.gravity * a.mass() * b.mass() * dist.normalized() /
                       dist.lengthSquared();

  a.push(-force);
  b.push(force);

  return state.gravity * a.mass() * b.mass() / dist.length();
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
    if (state.enable_gravity) {
      energy += gravity(a, b);
    }
  });

  if (!state.energy) {
    state.energy = energy;
    std::cout << "Total Energy of System: " << energy << std::endl;
  }

  sf::Vector2f mv = {0, 0};
  float mt = 0;
  for (Entity &e : state.entities) {
    if (state.enable_walls)
      collide_with_walls(e);

    drag_entity(e);
    e.tick(delta_time);

    mv += e.mass() * e.center();
    mt += e.mass();
  }

  if (mt != 0) {
    state.center_of_mass = mv / mt;
  }

  ImGui::Begin("Controls");

  ImGui::SliderFloat("Drag", &state.drag, -1.0f, 1.0f);
  ImGui::SliderFloat("Elasticity", &state.elasticity, 0.0f, 2.0f);

  ImGui::SliderFloat("Camera (x)", &state.camera_position.x, -1000.0f, 1000.0f);
  ImGui::SliderFloat("Camera (y)", &state.camera_position.y, -1000.0f, 1000.0f);

  ImGui::Checkbox("Enable Gravity", &state.enable_gravity);

  if (state.enable_gravity) {
    ImGui::SliderFloat("Gravity", &state.gravity, 0.0f, 1e3f);
    ImGui::Separator();
  }

  ImGui::Checkbox("Enable Walls", &state.enable_walls);

  if (ImGui::Button("Little Balls")) {
    reset_small();
  }
  if (ImGui::Button("Big Balls")) {
    reset_big();
  }

  if (ImGui::Button("Orbit")) {
    reset_orbit();
  }
  ImGui::End();
}

void render(sf::RenderWindow *window) {
  for (auto &e : state.entities) {
    e.shape().setPosition(e.position() + state.camera_position);
    e.draw(window);
  }

  sf::RectangleShape s{{6, 6}};
  s.setFillColor(sf::Color::White);
  s.setPosition(
      state.center_of_mass + state.camera_position - sf::Vector2f{3, 3}
  );
  window->draw(s);

  ImGui::SFML::Render(*window);
  window->display();
}

int main() {
  sf::RenderWindow window(sf::VideoMode({WORLD_WIDTH, WORLD_HEIGHT}), "SSPGE");
  // window.setVerticalSyncEnabled(true);

  if (!ImGui::SFML::Init(window))
    return -1;

  sf::Clock clock{};
  reset_state();

  // run the program as long as the window is open
  while (window.isOpen()) {
    // check all the window's events that were triggered since the last
    // iteration of the loop
    while (const std::optional event = window.pollEvent()) {
      ImGui::SFML::ProcessEvent(window, *event);

      // "close requested" event: we close the window
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (event->is<sf::Event::KeyPressed>()) {
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Escape)) {
          window.close();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::R)) {
          reset_state();
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Right)) {
          state.camera_position.x += 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Left)) {
          state.camera_position.x -= 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Down)) {
          state.camera_position.y += 1.0f;
        }
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Key::Up)) {
          state.camera_position.y -= 1.0f;
        }
      }
    }
    window.clear();

    auto delta_time = clock.restart();
    ImGui::SFML::Update(window, delta_time);

    tick(std::min(delta_time.asSeconds(), 1.0f / 60.0f));
    render(&window);
    state.frame += 1;
  }

  ImGui::SFML::Shutdown();
}
