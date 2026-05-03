#include "troops.cpp"
#include "overlay.cpp"

class Game {
    static constexpr char GameName[] = "Assimilate";
    static constexpr float MaximumTPS = 60.0f;
    static constexpr sf::Color Brown = sf::Color(188, 106, 60, 255);
    static constexpr sf::Color Gray = sf::Color(0, 0, 0, 64);

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

    std::shared_ptr<GameMap> makeGameMap();

public:
    Game();

    friend std::ostream& operator<<(std::ostream& out, const Game &game);

private:
    #define Overlays {std::ref(start_overlay), std::ref(paused_overlay), std::ref(over_overlay)}

    float getDeltaTime();
    void resetMap();
    void reanimateGangs();
    bool isPlaying();
    bool potentiallyStart();
    void handleInput();
    void handleSpawning(const float dt);
    void updateEntities(const float dt);
    void draw();
    void update();

public:
    void run();
};

const sf::Texture Game::start_overlay_texture = getTexture("assets/start_sign.png");
const sf::Texture Game::paused_overlay_texture = getTexture("assets/paused_sign.png");
const sf::Texture Game::over_overlay_texture = getTexture("assets/over_sign.png");
const sf::Texture Game::arena_texture = getTexture("assets/arena.png");
const sf::Texture Game::gravestone_texture = getTexture("assets/gravestone.png");