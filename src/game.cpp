#include <SFML/Graphics.hpp>
#include "troops.cpp"

class Game {
    sf::RenderWindow window;
    sf::Clock clock;
    float time_since_last_second;
    unsigned frames_since_last_second;

    std::optional<sf::Vector2i> mouse_position;
    bool left_click;

    const EntityBuilder entity_builder;
    std::shared_ptr<GameMap> game_map;
    std::vector<std::shared_ptr<Gang>> gangs;
    std::shared_ptr<Gang> troops;
    sf::RectangleShape grave;

public:
    Game(): 
        window(sf::RenderWindow(sf::VideoMode({800, 600}), "Die, or give it to the next!!")),
        time_since_last_second(0.0f), frames_since_last_second(0),
        mouse_position({}), left_click(false), 
        grave(sf::RectangleShape({50.0f, 50.0f})) 
    {
        grave.setFillColor(sf::Color::Black);

        constexpr sf::Vector2f inner_arena_size = {1000.0f, 1000.0f};
        const sf::Rect<float> inner_arena(-inner_arena_size / 2.0f, inner_arena_size);

        constexpr sf::Vector2f edge_size(200.0f, 200.0f);
        sf::RectangleShape outer_arena(inner_arena.size + edge_size * 2.0f);
        outer_arena.setPosition(inner_arena.position - edge_size);
        outer_arena.setFillColor(sf::Color::White);

        const sf::Vector2f chunk_size = {50.0f, 50.0f};
        const sf::Vector2u chunks((outer_arena.getSize() + chunk_size).componentWiseDiv(chunk_size));
        game_map = std::make_shared<GameMap>(chunk_size, chunks, inner_arena, outer_arena);

        troops = std::make_shared<Gang>(game_map, Team::Player);
        for(unsigned i = 0; i < 50; i++) {
            auto ptr = std::make_unique<Grunt>(entity_builder);
            Gang::addEntity(troops, std::move(ptr));
        }

        for(unsigned i = 0; i < 2; i++) {
            auto gang = std::make_shared<Gang>(game_map, Team::Enemy);
            for(unsigned j = 0; j < 5; j++) {
                auto ptr = std::make_unique<Grunt>(entity_builder);
                Gang::addEntity(gang, std::move(ptr));
            }
            gangs.push_back(gang);
        }
    }

private:
    void update() {
        while(const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();

            } else if(const auto key_event = event->getIf<sf::Event::KeyReleased>()) {
                if(key_event->code == sf::Keyboard::Key::A) debug(100);

            } else if(const auto mouse_pressed_event = event->getIf<sf::Event::MouseButtonPressed>()) {
                if(mouse_pressed_event->button == sf::Mouse::Button::Left) left_click = true;
            } else if(const auto mouse_released_event = event->getIf<sf::Event::MouseButtonReleased>()) {
                if(mouse_released_event->button == sf::Mouse::Button::Left) left_click = false;
                if(mouse_released_event->button == sf::Mouse::Button::Right) {
                    for(unsigned i = 0; i < gangs.size();) {
                        if(gangs[i]->getGravePosition().has_value()) {
                            gangs[i]->moveAllEntities(troops);
                            std::swap(gangs[i], gangs[gangs.size() - 1]);
                            gangs.pop_back();
                        } else {
                            i++;
                        }
                    }
                }

            } else if(const auto mouse_moved_event = event->getIf<sf::Event::MouseMoved>()) {
                mouse_position = mouse_moved_event->position;
            } else if(event->is<sf::Event::MouseLeft>()) {
                mouse_position = {};
            }
        }

        float dt = clock.restart().asSeconds();
        time_since_last_second += dt;
        frames_since_last_second++;
        if(time_since_last_second > 1.0f) {
            time_since_last_second -= 1.0f;
            std::cout << "fps: " << frames_since_last_second << std::endl;
            frames_since_last_second = 0;
        }

        if(const auto relative_mouse_position = mouse_position; left_click) {
            auto screen_position = window.getView().getCenter() - sf::Vector2f(window.getSize()) / 2.0f;
            auto absolute_mouse_position = screen_position + sf::Vector2f(*relative_mouse_position);
            auto clamped_mouse_position = clampPoint(absolute_mouse_position, game_map->getInnerArena());
            troops->walkTowards(clamped_mouse_position);
        } else {
            troops->stopWalking();
        }

        troops->update(dt);
        for(const auto &gang: gangs) gang->update(dt);

        if(!troops->isEmpty())
            window.setView({troops->getAveragePosition(), sf::Vector2f(window.getSize())});

        window.clear(sf::Color::Black);
        game_map->draw(window);
        for(const auto &gang: gangs) {
            if(auto grave_position = gang->getGravePosition()) {
                grave.setPosition(*grave_position - grave.getSize() / 2.0f);
                window.draw(grave);
            }
        }
        window.display();
    }

public:
    void run() {
        while(window.isOpen()) {
            update();
        }
    }
};