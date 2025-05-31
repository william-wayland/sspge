#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
#include <SFML/Window/Event.hpp>
#include <SFML/Window/Mouse.hpp>
#include <imgui-SFML.h>
#include <imgui.h>

#include <easylogging++.h>
#include <memory>
INITIALIZE_EASYLOGGINGPP

#include <SFML/Window/Joystick.hpp>
#include <SFML/Window/Keyboard.hpp>
#include <SFML/Window/Window.hpp>

#include "entity.h"
#include "world.h"

using Vector = sf::Vector2<double>;

template <typename T> class PID {
public:
  PID(T initial, double p = 1.0f, double i = 0.0f, double d = 0.0f)
    : p(p), i(i), d(d), m_point(initial), m_integral(), m_previous_error() {
  }

  T update(T setpoint, double dt) {
    const auto error = setpoint - m_point;
    m_integral += error;

    m_point += (p * error + i * m_integral * dt +
                d * (error - m_previous_error) / dt) /
               scale;
    m_previous_error = error;
    return m_point;
  }

  T point() const {
    return m_point;
  }

  void set_point(T state) {
    m_point = state;
  }

  T integral() const {
    return m_integral;
  }

  double p = 1.0f;
  double i = 0.0f;
  double d = 0.0f;
  double scale = 100000.0f;

private:
  T m_point;
  T m_integral;
  T m_previous_error;
};

struct State {
  Vector mouse;
  std::unique_ptr<Entity> ball;
  PID<Vector> pid;

  float down_force = 1.0f;

} state{{0.0f, 0.0f}, nullptr, {{0.0f, 0.0f}}};

void tick(float delta) {

  auto p = state.pid.update(state.mouse, delta);
  state.ball->set_center({static_cast<float>(p.x), static_cast<float>(p.y)});

  state.ball->push({0.0f, state.down_force * 1000.0f});
  state.ball->tick(delta);
}

void render(sf::RenderWindow *window) {
  const double min = 0.0;
  const double max_pi = 100.0;
  const double max_d = 30.0;

  ImGui::Begin("Controls");
  ImGui::SliderScalar("P", ImGuiDataType_Double, &state.pid.p, &min, &max_pi);
  ImGui::SliderScalar("I", ImGuiDataType_Double, &state.pid.i, &min, &max_pi);
  ImGui::SliderScalar("D", ImGuiDataType_Double, &state.pid.d, &min, &max_d);

  ImGui::SliderFloat("Down Force", &state.down_force, 0.0f, 100.0f);

  ImGui::Text(
      "Ball Position: %f,%f", state.ball->center().x, state.ball->center().y
  );
  ImGui::Text("Mouse Position: %f,%f", state.mouse.x, state.mouse.y);
  ImGui::Text(
      "Integral: %f,%f", state.pid.integral().x, state.pid.integral().y
  );
  // ImGui::Text("Integral: %f,%f", state.integral.x, state.integral.y);
  ImGui::End();

  state.ball->draw(window);
  ImGui::SFML::Render(*window);

  window->display();
}

int main() {
  sf::RenderWindow window(sf::VideoMode({WORLD_WIDTH, WORLD_HEIGHT}), "PID");
  // window.setVerticalSyncEnabled(true);

  if (!ImGui::SFML::Init(window))
    return -1;

  state.ball = std::make_unique<Entity>(
      sf::Vector2f{0, 0}, sf::Vector2f{0, 0}, 20, 1, sf::Color::Red
  );
  state.ball->set_center({(float)WORLD_WIDTH / 2, (float)WORLD_HEIGHT / 2});

  sf::Clock clock{};
  bool run = false;
  bool step = false;

  while (window.isOpen()) {
    while (const std::optional event = window.pollEvent()) {
      ImGui::SFML::ProcessEvent(window, *event);

      // "close requested" event: we close the window
      if (event->is<sf::Event::Closed>()) {
        window.close();
      }

      if (auto e = event->getIf<sf::Event::KeyPressed>()) {
        if (e->code == sf::Keyboard::Key::Escape) {
          window.close();
        }
        if (e->code == sf::Keyboard::Key::S) {
          step = true;
        }
      }

      if (auto e = event->getIf<sf::Event::MouseMoved>()) {
        state.mouse = {
            static_cast<float>(e->position.x), static_cast<float>(e->position.y)
        };
      }
      if (auto e = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Right)
          run = !run;
      }
    }
    window.clear();

    auto delta_time = clock.restart();
    ImGui::SFML::Update(window, delta_time);

    if (run || step)
      tick(std::min(delta_time.asSeconds(), 1.0f / 60.0f));

    step = false;
    render(&window);
  }

  ImGui::SFML::Shutdown();
}
