#include <SFML/Graphics/Color.hpp>
#include <SFML/System/Vector2.hpp>
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
} state;

void tick(float delta) {
  state.ball->set_center(state.mouse);
  // state.mouse->tick(delta);

  ImGui::Begin("Controls");
  ImGui::End();
}

void render(sf::RenderWindow *window) {
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

  sf::Clock clock{};

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
    }
    window.clear();

    auto delta_time = clock.restart();
    ImGui::SFML::Update(window, delta_time);

    tick(std::min(delta_time.asSeconds(), 1.0f / 60.0f));
    render(&window);
  }

  ImGui::SFML::Shutdown();
}
