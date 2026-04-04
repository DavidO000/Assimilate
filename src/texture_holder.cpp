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

inline void setTexture(sf::Texture &texture, const char *dir_name) {
    if(!texture.loadFromFile(dir_name)) 
        std::cerr << "Could not load texture!" << std::endl;
}

inline void setTexture(sf::Texture &texture, std::string &dir_name, const char name[]) {
    size_t regular_size = dir_name.size();
    dir_name += name;
    setTexture(texture, dir_name.c_str());
    dir_name.resize(regular_size); 
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

    explicit EntityTextures(std::string dir_name) {
        setTexture(player, dir_name, "player.png");
        setTexture(player_prepare_attack, dir_name, "player_prepare_attack.png");
        setTexture(player_attack, dir_name, "player_attack.png");

        #ifdef NDEBUG
            setTexture(enemy, dir_name, "enemy.png");
        #else
            setTexture(enemy, dir_name, "enemy_long.png");
        #endif
        setTexture(enemy_prepare_attack, dir_name, "enemy_prepare_attack.png");
        setTexture(enemy_attack, dir_name, "enemy_attack.png");

        setTexture(dead, dir_name, "dead.png");
    };

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

class TextureHolder {
    sf::Texture start_sign;
    sf::Texture paused_sign;
    sf::Texture over_sign;
    sf::Texture arena;
    sf::Texture grave;

    EntityTextures grunt;

    sf::Texture fireball;
public:
    TextureHolder(): grunt("assets/grunt/") {
        setTexture(start_sign, "assets/start_sign.png");
        setTexture(paused_sign, "assets/paused_sign.png");
        setTexture(over_sign, "assets/over_sign.png");
        setTexture(arena, "assets/arena.png");
        setTexture(grave, "assets/gravestone.png");
        setTexture(fireball, "assets/mage/fireball.png");
    }

    // This object is expensive, and should never be copied implicitly!
    ~TextureHolder() = default;
    TextureHolder(const TextureHolder&) = delete;
    TextureHolder& operator=(const TextureHolder&) = delete;
    TextureHolder(TextureHolder&&) = delete;
    TextureHolder& operator=(TextureHolder&&) = delete;
    friend std::ostream& operator<<(std::ostream& out, const TextureHolder &texture_holder) {
        out << "Grunt: " << texture_holder.grunt;
        return out;
    }

    const sf::Texture &getStartSign() const { return start_sign; }
    const sf::Texture &getPausedSign() const { return paused_sign; }
    const sf::Texture &getOverSign() const { return over_sign; }
    const sf::Texture &getArena() const { return arena; }
    const sf::Texture &getGrave() const { return grave; }

    const EntityTextures &getGrunt() const { return grunt; }

    const sf::Texture &getFireball() const { return fireball; }
};