#include <iostream>
#include <SFML/Graphics.hpp>

enum class Team {
    Player, 
    Enemy,
};

class EntityTextures {
    friend class EntityBuilder;

    sf::Texture player;
    sf::Texture enemy;
    sf::Texture dead;

public:
    const sf::Texture &getAlive(const Team team) const {
        return team == Team::Player ? player : enemy;
    }

    const sf::Texture &getDead() const {
        return dead;
    }
};

class EntityBuilder {
    EntityTextures grunt;

public:
    EntityBuilder() {
        if(!grunt.player.loadFromFile("assets/player_grunt.png")) std::cerr << "Could not load texture!" << std::endl;
        if(!grunt.enemy.loadFromFile("assets/enemy_grunt.png")) std::cerr << "Could not load texture!" << std::endl;
        if(!grunt.dead.loadFromFile("assets/dead_grunt.png")) std::cerr << "Could not load texture!" << std::endl;
    }

    const EntityTextures &getGrunt() const { return grunt; }
};