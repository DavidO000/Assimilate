#include "errors.cpp"

inline sf::Texture getTexture(const char *dir_name);
inline sf::Texture getTexture(std::string &dir_name, const char name[]);

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
    explicit EntityTextures(std::string dir_name);
    const sf::Texture &getAlive(const Team team) const;
    const sf::Texture &getPrepareAttack(const Team team) const;
    const sf::Texture &getAttack(const Team team) const;
    const sf::Texture &getDead() const;

    // This object is expensive, and should never be copied implicitly!
    ~EntityTextures() = default;
    EntityTextures(const EntityTextures&) = delete;
    EntityTextures& operator=(const EntityTextures&) = delete;
    EntityTextures(EntityTextures&&) = delete;
    EntityTextures& operator=(EntityTextures&&) = delete;
    friend std::ostream& operator<<(std::ostream& out, const EntityTextures &entity_textures);
};