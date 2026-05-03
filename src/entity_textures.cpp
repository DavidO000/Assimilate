#include "entity_textures.hpp"

bool isEnemyTeam(Team lhs, Team rhs) {
    return lhs != rhs;
}

std::ostream& operator<<(std::ostream& out, const Team &team) {
    out << (team == Team::Player ? "Player" : "Enemy");
    return out;
}

inline sf::Texture getTexture(const char *dir_name) {
    sf::Texture texture;
    if(!texture.loadFromFile(dir_name)) 
        throw LoadTextureException(dir_name);
    return texture;
}

inline sf::Texture getTexture(std::string &dir_name, const char name[]) {
    size_t regular_size = dir_name.size();
    dir_name += name;
    sf::Texture texture = getTexture(dir_name.c_str());
    dir_name.resize(regular_size); 
    return texture;
}


EntityTextures::EntityTextures(std::string dir_name) {
    std::optional<LoadTextureException> exception;
    try { player = getTexture(dir_name, "player.png"); } catch(LoadTextureException e) { exception = e; };
    try { player_prepare_attack = getTexture(dir_name, "player_prepare_attack.png"); } catch(LoadTextureException e) { exception = e; };
    try { player_attack = getTexture(dir_name, "player_attack.png"); } catch(LoadTextureException e) { exception = e; };
    try { enemy = getTexture(dir_name, "enemy.png"); } catch(LoadTextureException e) { exception = e; };
    try { enemy_prepare_attack = getTexture(dir_name, "enemy_prepare_attack.png"); } catch(LoadTextureException e) { exception = e; };
    try { enemy_attack = getTexture(dir_name, "enemy_attack.png"); } catch(LoadTextureException e) { exception = e; };
    try { dead = getTexture(dir_name, "dead.png"); } catch(LoadTextureException e) { exception = e; };
    if(exception) throw *exception;
}

const sf::Texture &EntityTextures::getAlive(const Team team) const {
    return team == Team::Player ? player : enemy;
}

const sf::Texture &EntityTextures::getPrepareAttack(const Team team) const {
    return team == Team::Player ? player_prepare_attack : enemy_prepare_attack;
}

const sf::Texture &EntityTextures::getAttack(const Team team) const {
    return team == Team::Player ? player_attack : enemy_attack;
}

const sf::Texture &EntityTextures::getDead() const {
    return dead;
}

std::ostream& operator<<(std::ostream& out, const EntityTextures &entity_textures) {
    auto player_size = entity_textures.player.getSize();
    auto enemy_size = entity_textures.enemy.getSize();
    auto dead_size = entity_textures.dead.getSize();
    out << "player: " << player_size.x << ", " << player_size.y 
        << " enemy: " << enemy_size.x << ", " << enemy_size.y
        << " dead: " << dead_size.x << ", " << dead_size.y;
    return out;
}