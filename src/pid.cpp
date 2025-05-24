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

struct State {
  sf::Vector2f mouse;
  std::unique_ptr<Entity> ball;
  sf::Vector2f integral = {0, 0};
  sf::Vector2f prev_error = {0, 0};

  float p_term = 1.0f;
  float i_term = 1.0f;
  float d_term = 0.5f;

} state;

void tick(float delta) {
  auto error = state.mouse - state.ball->center();

  auto proportional = state.p_term * error;

  state.integral += delta * error;
  auto integral = state.i_term * state.integral;

  auto derivitive = state.d_term * (error - state.prev_error) / delta;
  state.prev_error = error;

  state.ball->set_center(
      state.ball->center() + (proportional + integral + derivitive) * delta
  );
  // state.mouse->tick(delta);
}

void render(sf::RenderWindow *window) {
  ImGui::Begin("Controls");
  ImGui::SliderFloat("P", &state.p_term, -1.0f, 1.0f);
  ImGui::SliderFloat("I", &state.i_term, -1.0f, 1.0f);
  ImGui::SliderFloat("D", &state.d_term, -1.0f, 1.0f);
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
      }

      if (auto e = event->getIf<sf::Event::MouseMoved>()) {
        state.mouse = {
            static_cast<float>(e->position.x), static_cast<float>(e->position.y)
        };
        std::cout << state.mouse.x << std::endl;
      }
      if (auto e = event->getIf<sf::Event::MouseButtonPressed>()) {
        if (e->button == sf::Mouse::Button::Right)
          run = !run;
      }
    }
    window.clear();

    auto delta_time = clock.restart();
    ImGui::SFML::Update(window, delta_time);

    if (run)
      tick(std::min(delta_time.asSeconds(), 1.0f / 60.0f));
    render(&window);
  }

  ImGui::SFML::Shutdown();
}
