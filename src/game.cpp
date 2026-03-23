#include <SFML/Graphics.hpp>
#include "troops.cpp"

class Game {
    sf::RenderWindow window;
    sf::Clock clock;
    float time_since_started;
    float time_since_last_spawn;
    float time_until_next_spawn;

    float time_since_last_second;
    unsigned frames_since_last_second;

    std::optional<sf::Vector2i> mouse_position;
    bool left_click;
    float zoom_factor;

    const EntityBuilder entity_builder;
    std::shared_ptr<GameMap> game_map;
    std::vector<std::shared_ptr<Gang>> gangs;
    std::shared_ptr<Gang> troops;

    sf::Sprite grave;

public:
    Game(): 
        window(sf::RenderWindow(sf::VideoMode({800, 600}), "Die, or give it to the next!!")),
        time_since_started(0.0f), time_since_last_spawn(0.0f), time_until_next_spawn(0.0f),
        time_since_last_second(0.0f), frames_since_last_second(0),
        mouse_position({}), left_click(false), zoom_factor(1.0f),
        grave(sf::Sprite(entity_builder.getGrave()))
    {
        constexpr sf::Vector2f inner_arena_size = {5000.0f, 5000.0f};
        const sf::Rect<float> inner_arena(-inner_arena_size / 2.0f, inner_arena_size);

        constexpr sf::Vector2f edge_size(1500.0f, 1500.0f);
        sf::RectangleShape outer_arena(inner_arena.size + edge_size * 2.0f);
        outer_arena.setPosition(inner_arena.position - edge_size);
        outer_arena.setTexture(&entity_builder.getArena());

        const sf::Vector2f chunk_size = {100.0f, 100.0f};
        const sf::Vector2u chunks((outer_arena.getSize() + chunk_size).componentWiseDiv(chunk_size));
        game_map = std::make_shared<GameMap>(chunk_size, chunks, inner_arena, outer_arena);

        troops = std::make_shared<Gang>(game_map, Team::Player);
        for(unsigned i = 0; i < 5; i++) {
            auto ptr = std::make_unique<Grunt>(entity_builder);
            Gang::addEntity(troops, std::move(ptr));
        }

        for(unsigned i = 0; i < 10; i++) {
            auto gang = std::make_shared<Gang>(game_map, Team::Enemy);
            unsigned amount = rand() % 5 + 1;
            for(unsigned j = 0; j < amount; j++) {
                auto ptr = std::make_unique<Grunt>(entity_builder);
                Gang::addEntity(gang, std::move(ptr));
            }
            gangs.push_back(gang);
        }
    }

    friend std::ostream& operator<<(std::ostream& out, const Game &game);

private:
    void update() {
        while(const std::optional event = window.pollEvent()) {
            if (event->is<sf::Event::Closed>()) {
                window.close();
            } else if(const auto key_event = event->getIf<sf::Event::KeyReleased>()) {
                if(key_event->code == sf::Keyboard::Key::A) debug(100);
            } else if(const auto mouse_scrolled_event = event->getIf<sf::Event::MouseWheelScrolled>()) {
                zoom_factor = std::clamp(zoom_factor + mouse_scrolled_event->delta, 0.5f, 10.0f);
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
        time_since_last_spawn += dt;
        time_since_started += dt;
        if(time_since_last_spawn > time_until_next_spawn) {
            time_since_last_spawn = 0.0f;
            time_until_next_spawn = rand() % 8 + 2;
            for(unsigned i = 0; i < 1; i++) {
                auto gang = std::make_shared<Gang>(game_map, Team::Enemy);
                unsigned amount = std::sqrt(time_since_started) + 1;
                for(unsigned j = 0; j < amount; j++) {
                    auto ptr = std::make_unique<Grunt>(entity_builder);
                    Gang::addEntity(gang, std::move(ptr));
                }
                gangs.push_back(gang);
            }
        }

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

        // Aggro loss before updates as to not have targets be dead/possibly dissapearing troops
        troops->updateAggroLoss();
        for(const auto &gang: gangs) gang->updateAggroLoss();

        troops->update(dt);
        for(const auto &gang: gangs) gang->update(dt);

        game_map->updateCollisions();

        if(!troops->isEmpty())
            window.setView({troops->getAveragePosition(), sf::Vector2f(window.getSize()) * zoom_factor});

        window.clear(sf::Color::Black);
        game_map->draw(window);
        for(const auto &gang: gangs) {
            if(auto grave_position = gang->getGravePosition()) {
                grave.setPosition(*grave_position - sf::Vector2f(grave.getTexture().getSize()) / 2.0f);
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

std::ostream& operator<<(std::ostream& out, const Game &game) {
    out << "Mouse position: ";
    if(const auto &mouse_position = game.mouse_position) out << mouse_position->x << ", " << mouse_position->y;
    else out << "None";
    out << ", Left clicked: " << game.left_click;
    return out;
}