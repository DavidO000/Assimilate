#include <iostream>
#include <SFML/Graphics.hpp>

enum class Team {
    Player, 
    Enemy,
};

std::ostream& operator<<(std::ostream& out, const Team &team) {
    out << (team == Team::Player ? "Player" : "Enemy");
    return out;
}

class EntityTextures {
    friend class EntityBuilder;

    sf::Texture player;
    sf::Texture enemy;
    sf::Texture dead;

    EntityTextures() = default; // requires 2 step initialisation
public:
    const sf::Texture &getAlive(const Team team) const {
        return team == Team::Player ? player : enemy;
    }

    const sf::Texture &getDead() const {
        return dead;
    }

    // This object is expensive, and should never be copied implicitly!
    ~EntityTextures() = default;
    EntityTextures(const EntityTextures&) = delete;
    EntityTextures& operator=(const EntityTextures&) = delete;
    EntityTextures(EntityTextures&&) = delete;
    EntityTextures& operator=(EntityTextures&&) = delete;
    friend std::ostream& operator<<(std::ostream& out, const EntityTextures &entity_textures) {
        auto player_size = entity_textures.player.getSize();
        auto enemy_size = entity_textures.enemy.getSize();
        auto dead_size = entity_textures.dead.getSize();
        out << "player: " << player_size.x << ", " << player_size.y 
            << " enemy: " << enemy_size.x << ", " << enemy_size.y
            << " dead: " << dead_size.x << ", " << dead_size.y;
        return out;
    }
};

class EntityBuilder {
    sf::Texture start_sign;
    sf::Texture paused_sign;
    sf::Texture over_sign;
    sf::Texture arena;
    sf::Texture grave;

    EntityTextures grunt;

public:
    EntityBuilder() {
        if(!start_sign.loadFromFile("assets/start_sign.png")) std::cerr << "Could not load texture!" << std::endl;
        if(!paused_sign.loadFromFile("assets/paused_sign.png")) std::cerr << "Could not load texture!" << std::endl;
        if(!over_sign.loadFromFile("assets/over_sign.png")) std::cerr << "Could not load texture!" << std::endl;
        if(!arena.loadFromFile("assets/arena.png")) std::cerr << "Could not load texture!" << std::endl;
        if(!grave.loadFromFile("assets/gravestone.png")) std::cerr << "Could not load texture!" << std::endl;

        if(!grunt.player.loadFromFile("assets/player_grunt.png")) std::cerr << "Could not load texture!" << std::endl;
        #ifdef NDEBUG
            if(!grunt.enemy.loadFromFile("assets/enemy_grunt.png")) std::cerr << "Could not load texture!" << std::endl;
        #else
            if(!grunt.enemy.loadFromFile("assets/enemy_grunt_long.png")) std::cerr << "Could not load texture!" << std::endl;
        #endif
        if(!grunt.dead.loadFromFile("assets/dead_grunt.png")) std::cerr << "Could not load texture!" << std::endl;
    }

    // This object is expensive, and should never be copied implicitly!
    ~EntityBuilder() = default;
    EntityBuilder(const EntityBuilder&) = delete;
    EntityBuilder& operator=(const EntityBuilder&) = delete;
    EntityBuilder(EntityBuilder&&) = delete;
    EntityBuilder& operator=(EntityBuilder&&) = delete;
    friend std::ostream& operator<<(std::ostream& out, const EntityBuilder &entity_builder) {
        out << "Grunt: " << entity_builder.grunt;
        return out;
    }

    const sf::Texture &getStartSign() const { return start_sign; }
    const sf::Texture &getPausedSign() const { return paused_sign; }
    const sf::Texture &getOverSign() const { return over_sign; }
    const sf::Texture &getArena() const { return arena; }
    const sf::Texture &getGrave() const { return grave; }

    const EntityTextures &getGrunt() const { return grunt; }
};