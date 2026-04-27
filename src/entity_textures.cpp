#include <iostream>
#include <SFML/Graphics.hpp>
#include "errors.cpp"

enum class Team {
    Player, 
    Enemy,
};

bool isEnemyTeam(Team lhs, Team rhs) {
    return lhs != rhs;
}

std::ostream& operator<<(std::ostream& out, const Team &team) {
    out << (team == Team::Player ? "Player" : "Enemy");
    return out;
}

const inline sf::Texture getTexture(const char *dir_name) {
    sf::Texture texture;
    if(!texture.loadFromFile(dir_name)) 
        std::cerr << "Could not load texture!" << std::endl;
    return texture;
}

inline sf::Texture getTexture(std::string &dir_name, const char name[]) {
    size_t regular_size = dir_name.size();
    dir_name += name;
    sf::Texture texture = getTexture(dir_name.c_str());
    dir_name.resize(regular_size); 
    return texture;
}

class EntityTextures {
    friend class TextureHolder;

    sf::Texture player;
    sf::Texture player_prepare_attack;
    sf::Texture player_attack;
    sf::Texture enemy;
    sf::Texture enemy_prepare_attack;
    sf::Texture enemy_attack;
    sf::Texture dead;

public:
    explicit EntityTextures(std::string dir_name):
        player(getTexture(dir_name, "player.png")),
        player_prepare_attack(getTexture(dir_name, "player_prepare_attack.png")),
        player_attack(getTexture(dir_name, "player_attack.png")),
        enemy(getTexture(dir_name, "enemy.png")),
        enemy_prepare_attack(getTexture(dir_name, "enemy_prepare_attack.png")),
        enemy_attack(getTexture(dir_name, "enemy_attack.png")),
        dead(getTexture(dir_name, "dead.png"))
    {};

public:
    const sf::Texture &getAlive(const Team team) const {
        return team == Team::Player ? player : enemy;
    }

    const sf::Texture &getPrepareAttack(const Team team) const {
        return team == Team::Player ? player_prepare_attack : enemy_prepare_attack;
    }

    const sf::Texture &getAttack(const Team team) const {
        return team == Team::Player ? player_attack : enemy_attack;
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