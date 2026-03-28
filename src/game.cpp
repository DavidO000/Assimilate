#include <SFML/Graphics.hpp>
#include "troops.cpp"

enum class GameState {
    Start,
    Playing,
    Paused,
    Over,
};

class Game {
    sf::RenderWindow window;
    std::optional<sf::Vector2u> last_window_size;
    sf::Clock clock;

    GameState game_state;

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
    sf::Sprite start_sign;
    sf::Sprite paused_sign;
    sf::Sprite over_sign;

    static constexpr char GameName[] = "Assimilate";
public:
    Game(): 
        window(sf::RenderWindow(sf::VideoMode({800, 600}), GameName)),
        game_state(GameState::Start),
        time_since_started(0.0f), time_since_last_spawn(0.0f), time_until_next_spawn(0.0f),
        time_since_last_second(0.0f), frames_since_last_second(0),
        left_click(false), zoom_factor(1.0f),
        grave(sf::Sprite(entity_builder.getGrave())),
        start_sign(sf::Sprite(entity_builder.getStartSign())),
        paused_sign(sf::Sprite(entity_builder.getPausedSign())),
        over_sign(sf::Sprite(entity_builder.getOverSign()))
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
    }

    friend std::ostream& operator<<(std::ostream& out, const Game &game);

private:
    static constexpr float MaximumTPS = 60.0f;

    float getDeltaTime() {
        float real_dt = clock.restart().asSeconds();
        time_since_last_second += real_dt;
        frames_since_last_second++;
        if(time_since_last_second > 1.0f) {
            time_since_last_second -= 1.0f;
            std::cout << "fps: " << frames_since_last_second << std::endl;
            frames_since_last_second = 0;
        }
        return std::min(real_dt, 1.0f / MaximumTPS);
    }

    void resetMap() {
        game_map->reset();

        troops = std::make_shared<Gang>(game_map, Team::Player);
        for(unsigned i = 0; i < 5; i++) {
            auto ptr = std::make_unique<Grunt>(entity_builder);
            Gang::addEntity(troops, std::move(ptr));
        }

        gangs.clear();
        for(unsigned i = 0; i < 5; i++) {
            auto gang = std::make_shared<Gang>(game_map, Team::Enemy);
            unsigned amount = rand() % 5 + 1;
                for(unsigned j = 0; j < amount; j++) {
                auto ptr = std::make_unique<Grunt>(entity_builder);
                Gang::addEntity(gang, std::move(ptr));
            }
            gangs.push_back(gang);
        }
    }

    void reanimateGangs() {
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

    void handleInput() {
        while(const std::optional event = window.pollEvent()) {
            if(event->is<sf::Event::Closed>()) {
                window.close();
            } else if(event->is<sf::Event::FocusLost>()) {
                if(game_state == GameState::Playing) game_state = GameState::Paused;

            } else if(const auto mouse_scrolled_event = event->getIf<sf::Event::MouseWheelScrolled>()) {
                zoom_factor = std::clamp(zoom_factor + mouse_scrolled_event->delta, 0.5f, 10.0f);
            } else if(const auto mouse_pressed_event = event->getIf<sf::Event::MouseButtonPressed>()) {
                if(mouse_pressed_event->button == sf::Mouse::Button::Left) left_click = true;
            } else if(const auto mouse_released_event = event->getIf<sf::Event::MouseButtonReleased>()) {
                if(mouse_released_event->button == sf::Mouse::Button::Left) left_click = false;
                else if(mouse_released_event->button == sf::Mouse::Button::Right 
                    && game_state == GameState::Playing) reanimateGangs();
            } else if(const auto mouse_moved_event = event->getIf<sf::Event::MouseMoved>()) {
                mouse_position = mouse_moved_event->position;
            } else if(event->is<sf::Event::MouseLeft>()) {
                mouse_position = {};

            } else if(const auto key_event = event->getIf<sf::Event::KeyPressed>()) {
                if(key_event->code == sf::Keyboard::Key::F11) {
                    if(const auto window_size = last_window_size) {
                        window.create(sf::VideoMode(*window_size), GameName);
                        last_window_size = {};
                    } else {
                        last_window_size = window.getSize();
                        window.create(sf::VideoMode::getDesktopMode(), GameName, sf::Style::None);
                    }
                    
                } if(key_event->code == sf::Keyboard::Key::Escape) {
                    if(game_state == GameState::Playing) game_state = GameState::Paused;
                    else if(game_state == GameState::Paused) game_state = GameState::Playing;
                } else if(key_event->code == sf::Keyboard::Key::Enter) {
                    if(game_state == GameState::Start || game_state == GameState::Over) {
                        resetMap();
                        game_state = GameState::Playing;
                    }
                }
            }
        }
    }

    void handleSpawning(const float dt) {
        time_since_last_spawn += dt;
        time_since_started += dt;
        if(time_since_last_spawn > time_until_next_spawn) {
            time_since_last_spawn = 0.0f;
            time_until_next_spawn = rand() % 8 + 2;
            for(unsigned i = 0; i < 1; i++) {
                auto gang = std::make_shared<Gang>(game_map, Team::Enemy);
                unsigned amount = std::pow(time_since_started, 0.75) + 1;
                for(unsigned j = 0; j < amount; j++) {
                    auto ptr = std::make_unique<Grunt>(entity_builder);
                    Gang::addEntity(gang, std::move(ptr));
                }
                gangs.push_back(gang);
            }
        }
    }

    void updateEntities(const float dt) {
        if(const auto relative_mouse_position = mouse_position; left_click) {
            auto absolute_mouse_position = window.mapPixelToCoords(*relative_mouse_position);
            auto clamped_mouse_position = clampPoint(absolute_mouse_position, game_map->getInnerArena());
            troops->walkTowards(clamped_mouse_position);
        } else {
            troops->stopWalking();
        }

        // Aggro loss before updates as to not have targets be dead/possibly dissapearing troops
        for(const auto &gang: gangs) gang->updateAggroLoss();
        troops->updateAggroLoss();

        // First you have the enemies attack, then you have the troops potentially be removed 
        for(const auto &gang: gangs) gang->update(dt);
        troops->update(dt);

        game_map->updateMovement();

        if(troops->isEmpty()) game_state = GameState::Over;
    }

    void drawStart() {
        static constexpr sf::Color Brown(188,106,60);
        window.setView({sf::Vector2f(window.getSize()) / 2.0f, sf::Vector2f(window.getSize())});

        window.clear(Brown);
        start_sign.setPosition(sf::Vector2f(window.getSize() - start_sign.getTexture().getSize()) / 2.0f);
        window.draw(start_sign);
        window.display();
    }

    void drawInGame() {
        window.setView({
            troops->isEmpty() ? window.getView().getCenter() : troops->getAveragePosition(), 
            sf::Vector2f(window.getSize()) * zoom_factor
        });

        window.clear(sf::Color::Black);
        game_map->draw(window);
        for(const auto &gang: gangs) {
            if(auto grave_position = gang->getGravePosition()) {
                grave.setPosition(*grave_position - sf::Vector2f(grave.getTexture().getSize()) / 2.0f);
                window.draw(grave);
            }
        }

        if(game_state == GameState::Paused || game_state == GameState::Over) {
            const auto old_view = window.getView(); 
            window.setView({sf::Vector2f(window.getSize()) / 2.0f, sf::Vector2f(window.getSize())});

            sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
            overlay.setFillColor(sf::Color(0, 0, 0, 64));
            window.draw(overlay);

            if(game_state == GameState::Paused) {
                paused_sign.setPosition(sf::Vector2f(window.getSize() - paused_sign.getTexture().getSize()) / 2.0f);
                window.draw(paused_sign);
            } else if(game_state == GameState::Over) {
                over_sign.setPosition(sf::Vector2f(window.getSize() - over_sign.getTexture().getSize()) / 2.0f);
                window.draw(over_sign);
            }

            window.setView(old_view);
        }

        window.display();
    }

    void update() {
        float dt = getDeltaTime();
        handleInput();

        if(game_state == GameState::Playing || game_state == GameState::Over) {
            if(game_state == GameState::Playing) handleSpawning(dt);
            updateEntities(dt);
        }

        if(game_state == GameState::Start) {
            drawStart();
        } else {
            drawInGame();
        }
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