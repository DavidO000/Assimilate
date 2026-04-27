#include <SFML/Graphics.hpp>
#include "troops.cpp"
#include "overlay.cpp"

class Game {
    float time_since_started;
    float time_since_last_spawn;
    float time_until_next_spawn;

    float time_since_last_second;
    unsigned frames_since_last_second;

    std::optional<sf::Vector2i> mouse_position;
    bool left_click;
    float zoom_factor;

    std::shared_ptr<GameMap> game_map;
    std::vector<Gang> gangs;
    Gang troops;

    static const sf::Texture start_overlay_texture;
    static const sf::Texture paused_overlay_texture;
    static const sf::Texture over_overlay_texture;
    Overlay start_overlay;
    Overlay paused_overlay;
    Overlay over_overlay;

    struct WindowData { sf::Vector2u size; sf::Vector2i position; };
    std::optional<WindowData> last_window_data;
    sf::Clock clock;
    sf::RenderWindow window; // window at the end to be as close to updating as possible

    static const sf::Texture arena_texture;
    static const sf::Texture gravestone_texture;

    static constexpr char GameName[] = "Assimilate";

    std::shared_ptr<GameMap> makeGameMap() {
        const sf::Vector2f outer_arena_size(arena_texture.getSize());
        constexpr sf::Vector2f edge_size(500.0f, 500.0f);
        const sf::Vector2f inner_arena_size = outer_arena_size - edge_size * 2.0f;
        const sf::Rect<float> inner_arena(-inner_arena_size / 2.0f, inner_arena_size);
        sf::RectangleShape outer_arena(outer_arena_size);
        outer_arena.setPosition(-outer_arena_size / 2.0f);
        outer_arena.setTexture(&arena_texture);
        constexpr sf::Vector2f chunk_size(100.0f, 100.0f);
        const sf::Vector2u chunks(
            std::ceilf(outer_arena_size.x / chunk_size.x), 
            std::ceilf(outer_arena_size.y / chunk_size.y)
        );
        return std::make_shared<GameMap>(chunk_size, chunks, inner_arena, outer_arena);
    }
public:
    static constexpr sf::Color Brown = sf::Color(188, 106, 60, 255);
    static constexpr sf::Color Gray = sf::Color(0, 0, 0, 64);
    Game():
        time_since_started(0.0f), time_since_last_spawn(0.0f), time_until_next_spawn(0.0f),
        time_since_last_second(0.0f), frames_since_last_second(0),
        left_click(false), zoom_factor(1.0f),
        game_map(makeGameMap()), troops(Gang(game_map, Team::Player)),
        start_overlay { sf::Sprite(start_overlay_texture), Brown, OverlayOrder::Coming },
        paused_overlay { sf::Sprite(paused_overlay_texture), Gray, OverlayOrder::Going },
        over_overlay { sf::Sprite(over_overlay_texture), Gray, OverlayOrder::Going },
        window(sf::RenderWindow(sf::VideoMode({800, 600}), GameName))
    {
        window.setVisible(false);
    }

    friend std::ostream& operator<<(std::ostream& out, const Game &game);

private:
    #define Overlays {std::ref(start_overlay), std::ref(paused_overlay), std::ref(over_overlay)}

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

        for(auto overlay: Overlays) {
            overlay.get().since += real_dt;
        }

        return std::min(real_dt, 1.0f / MaximumTPS);
    }

    void resetMap() {
        game_map->reset();
        time_since_started = 0.0f;

        troops = Gang(game_map, Team::Player);
        for(int i = 0; i < 5; i++)
            troops.addEntity(std::make_unique<Grunt>());

        gangs.clear();
        for(unsigned i = 0; i < 5; i++) {
            Gang gang(game_map, Team::Enemy);
            unsigned amount = rand() % 5 + 1;
                for(unsigned j = 0; j < amount; j++) {
                gang.addEntity(std::make_unique<Grunt>());
            }
            gangs.push_back(std::move(gang));
        }
    }

    void reanimateGangs() {
        for(unsigned i = 0; i < gangs.size();) {
            if(gangs[i].getGravePosition().has_value()) {
                gangs[i].moveAllEntities(troops);
                std::swap(gangs[i], gangs[gangs.size() - 1]);
                gangs.pop_back();
            } else {
                i++;
            }
        }
    }

    bool isPlaying() {
        for(auto overlay: Overlays) {
            if(overlay.get().order == OverlayOrder::Coming) return false;
        }
        return true;
    }

    bool potentiallyStart() {
        if(start_overlay.isIn()) {
            resetMap();
            start_overlay.remove();
            return true;
        } else if(over_overlay.isIn()) {
            resetMap();
            over_overlay.remove();
            return true;
        } else if(paused_overlay.isIn()) {
            paused_overlay.remove();
            return false;
        }
        return false;
    }

    void handleInput() {
        while(const std::optional event = window.pollEvent()) {
            if(event->is<sf::Event::Closed>()) {
                window.close();
            } else if(event->is<sf::Event::FocusLost>()) {
                if(isPlaying()) paused_overlay.set();

            } else if(const auto mouse_scrolled_event = event->getIf<sf::Event::MouseWheelScrolled>()) {
                if(!start_overlay.isIn())
                    zoom_factor = std::clamp(zoom_factor + mouse_scrolled_event->delta, 0.5f, 10.0f);
            } else if(const auto mouse_pressed_event = event->getIf<sf::Event::MouseButtonPressed>()) {
                if(!potentiallyStart()) {
                    if(mouse_pressed_event->button == sf::Mouse::Button::Left) left_click = true;
                }
            } else if(const auto mouse_released_event = event->getIf<sf::Event::MouseButtonReleased>()) {
                if(mouse_released_event->button == sf::Mouse::Button::Left) left_click = false;
                else if(mouse_released_event->button == sf::Mouse::Button::Right && isPlaying()) reanimateGangs();
            } else if(const auto mouse_moved_event = event->getIf<sf::Event::MouseMoved>()) {
                mouse_position = mouse_moved_event->position;
            } else if(event->is<sf::Event::MouseLeft>()) {
                mouse_position = {};

            } else if(const auto key_event = event->getIf<sf::Event::KeyPressed>()) {
                if(key_event->code == sf::Keyboard::Key::F11) {
                    if(const auto window_data = last_window_data) {
                        window.create(sf::VideoMode(window_data->size), GameName);
                        window.setPosition(window_data->position);
                        last_window_data = {};
                    } else {
                        last_window_data = {window.getSize(), window.getPosition()};
                        window.create(sf::VideoMode::getDesktopMode(), GameName, sf::Style::None);
                    }
                    
                } if(key_event->code == sf::Keyboard::Key::Escape) {
                    if(isPlaying()) paused_overlay.set();
                    else if(paused_overlay.isIn()) paused_overlay.remove();
                } else if(key_event->code == sf::Keyboard::Key::Enter) {
                    potentiallyStart();
                } else if(key_event->code == sf::Keyboard::Key::Space) {
                    potentiallyStart();
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
            Gang gang(game_map, Team::Enemy);
            int index_center = time_since_started / 10;
            int index_spread = index_center + rand() % 3 + rand() % 3 + rand() % 3 - 3;
            int index_spawn = std::clamp(index_spread, 0, 9);
            switch (index_spawn) {
            case 0:
                for(unsigned i = 0; i < 2; i++) gang.addEntity(std::make_unique<Grunt>());
                break;
            case 1:
                for(unsigned i = 0; i < 5; i++) gang.addEntity(std::make_unique<Grunt>());
                break;
            case 2:
                for(unsigned i = 0; i < 12; i++) gang.addEntity(std::make_unique<Grunt>());
                break;
            case 3:
                for(unsigned i = 0; i < 9; i++) gang.addEntity(std::make_unique<Grunt>());
                for(unsigned i = 0; i < 2; i++) gang.addEntity(std::make_unique<Knight>());
                break;
            case 4:
                for(unsigned i = 0; i < 3; i++) gang.addEntity(std::make_unique<Knight>());
                for(unsigned i = 0; i < 1; i++) gang.addEntity(std::make_unique<Mage>());
                break;
            case 5:
                for(unsigned i = 0; i < 7; i++) gang.addEntity(std::make_unique<Knight>());
                for(unsigned i = 0; i < 2; i++) gang.addEntity(std::make_unique<Mage>());
                break;
            case 6:
                for(unsigned i = 0; i < 16; i++) gang.addEntity(std::make_unique<Knight>());
                for(unsigned i = 0; i < 3; i++) gang.addEntity(std::make_unique<Mage>());
                break;
            case 7:
                for(unsigned i = 0; i < 3; i++) gang.addEntity(std::make_unique<Samurai>());
                break;
            case 8:
                for(unsigned i = 0; i < 7; i++) gang.addEntity(std::make_unique<Samurai>());
                break;
            case 9:
                for(unsigned i = 0; i < 6; i++) gang.addEntity(std::make_unique<Samurai>());
                for(unsigned i = 0; i < 1; i++) gang.addEntity(std::make_unique<Minitroll>());
                break;
            case 10:
                for(unsigned i = 0; i < 15; i++) gang.addEntity(std::make_unique<Samurai>());
                for(unsigned i = 0; i < 1; i++) gang.addEntity(std::make_unique<Minitroll>());
                break;
            case 11:
                for(unsigned i = 0; i < 24; i++) gang.addEntity(std::make_unique<Samurai>());
                for(unsigned i = 0; i < 2; i++) gang.addEntity(std::make_unique<Minitroll>());
                break;
            case 12:
                gang.addEntity(std::make_unique<Troll>());
                break;
            }
            gangs.push_back(std::move(gang));
        }
    }

    void updateEntities(const float dt) {
        if(const auto relative_mouse_position = mouse_position; left_click) {
            auto absolute_mouse_position = window.mapPixelToCoords(*relative_mouse_position);
            auto clamped_mouse_position = clampPoint(absolute_mouse_position, game_map->getInnerArena());
            troops.walkTowards(clamped_mouse_position);
        } else {
            troops.stopWalking();
        }

        // Aggro loss before updates as to not have targets be dead/possibly dissapearing troops
        for(auto &gang: gangs) gang.updateAggroLoss();
        troops.updateAggroLoss();

        for(auto &gang: gangs) gang.update(dt);
        troops.update(dt);

        troops.removeDeadTroops();

        game_map->updateProjectiles(dt);
        game_map->updateMovement();

        if(troops.isEmpty() && !over_overlay.isIn()) over_overlay.set();
    }

    void draw() {
        if(!start_overlay.isIn()) {
            window.setView({
                troops.isEmpty() ? window.getView().getCenter() : troops.getAveragePosition(), 
                sf::Vector2f(window.getSize()) * zoom_factor
            });

            window.clear(sf::Color::Black);
            game_map->draw(window);

            for(const auto &gang: gangs) {
                if(auto grave_position = gang.getGravePosition()) {
                    sf::Sprite gravestone(gravestone_texture);
                    gravestone.setPosition(*grave_position - sf::Vector2f(gravestone.getTexture().getSize()) / 2.0f);
                    window.draw(gravestone);
                }
            }
        }

        auto old_view = window.getView();
        window.setView({sf::Vector2f(window.getSize()) / 2.0f, sf::Vector2f(window.getSize())});

        for(auto overlay_data_ref: Overlays) {
            auto &overlay_data = overlay_data_ref.get();
            float since = std::clamp(overlay_data.since, 0.0f, SecondsToPutOverlay) / SecondsToPutOverlay;
            float drop_down_height = overlay_data.order == OverlayOrder::Coming ? since - 1.0f: -since;
            sf::Vector2f drop_down(0.0f, float(window.getSize().y) * drop_down_height);

            sf::RectangleShape overlay(sf::Vector2f(window.getSize()));
            overlay.setPosition(drop_down);
            overlay.setFillColor(overlay_data.background_color);
            window.draw(overlay);

            sf::Vector2f sign_location = sf::Vector2f(window.getSize() - overlay_data.sign.getTexture().getSize()) / 2.0f;
            overlay_data.sign.setPosition(sign_location + drop_down);
            window.draw(overlay_data.sign);
        }

        window.setView(old_view);

        window.display();
        window.setVisible(true);
    }

    void update() {
        float dt = getDeltaTime();
        handleInput();

        if(isPlaying() || over_overlay.isIn()) {
            if(isPlaying()) handleSpawning(dt);
            updateEntities(dt);
        }

        draw();
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

const sf::Texture Game::start_overlay_texture = getTexture("assets/start_sign.png");
const sf::Texture Game::paused_overlay_texture = getTexture("assets/paused_sign.png");
const sf::Texture Game::over_overlay_texture = getTexture("assets/over_sign.png");
const sf::Texture Game::arena_texture = getTexture("assets/arena.png");
const sf::Texture Game::gravestone_texture = getTexture("assets/gravestone.png");