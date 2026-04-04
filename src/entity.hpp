#include <iostream>
#include <vector>
#include <algorithm>
#include <SFML/Graphics.hpp>
#include "utils.cpp"
#include "texture_holder.cpp"

class Projectile;
class Entity;
class Gang;
class GameMap;
class ChunkIterator;

class Projectile {
    friend class GameMap;

protected:
    Team team;
    bool to_delete;
    virtual void hitEntity(Entity *hit) = 0;

public:
    sf::Sprite sprite;
    sf::Vector2f velocity;
    Entity* last_hit_entity; 
    
    Projectile(const sf::Texture &texture, Entity &entity, const float speed);
    sf::Vector2f getOrigin() const;
};

class Entity {
    friend class Projectile;
    friend class Gang;
    friend class GameMap;

    static constexpr float StandingWobbleAmplitude = 0.05f;
    static constexpr float StandingWobbleSpeed = 2.5f;
    static constexpr float MovingWobbleAmplitude = 0.075f;
    static constexpr float MovingWobbleSpeed = 12.0f;

    const EntityTextures &textures;
    sf::Sprite sprite;
    float radius;

    sf::Vector2f to_move;
    sf::Vector2f knocked_back;

    float wobble_position;
    float wobble_amplitude;

    Team team;
    Entity *target;
    
    unsigned health;
    float attack_counter;
    float time_since_attacked;
    float time_since_revived;
    float time_since_was_attacked;

    std::vector<std::unique_ptr<Projectile>> projectiles;

public:
    enum class Direction { Left, Right };

    explicit Entity(const EntityTextures &textures);
    virtual ~Entity() = default;
    // This object should never be copied implicitly!
    Entity(const Entity&) = delete;
    Entity& operator=(const Entity&) = delete;
    Entity(Entity&&) = delete;
    Entity& operator=(Entity&&) = delete;
    friend std::ostream& operator<<(std::ostream& out, const Entity &entity);

    Team getTeam() const;
    sf::Vector2f getOrigin() const;
    bool isDead() const;
    void takeDamage(const unsigned amount);
    void takeKnockback(const sf::Vector2f from, const float amount);
    void draw(sf::RenderWindow &window) const;

protected:
    Entity* getTarget();
    void throwProjectile(std::unique_ptr<Projectile> projectile);

    virtual float getRadius() const = 0;
    virtual float getSpeed() const = 0;
    virtual unsigned getInitialHealth() const = 0;
    // virtual unsigned getMaximumHealth() const = 0;
    virtual float getAggroRadius() const = 0;
    virtual float getLoseAggroRadius() const = 0;
    virtual float getAggroSpreadRadius() const = 0;
    virtual float getAttackRadius() const = 0;
    virtual float getPrefferedAttackRadius() const = 0;
    virtual float getAttackSpeed() const = 0;
    virtual float getAttackPrepareDuration() const = 0;
    virtual float getAttackDuration() const = 0;
    virtual void attack() = 0;

private:
    void setDirection(const Direction direction);
    void setTexture(const sf::Texture &texture);
    void progressWobble(const float desired_amplitude, const float speed, const float dt);

    void walkTowards(const sf::Vector2f destination, const float dt);

    void updateAggroLoss();

    void update(const float dt);
};

class Gang {
    Team team;
    std::vector<std::unique_ptr<Entity>> entities;
    std::shared_ptr<GameMap> game_map;
    sf::Vector2f spawn_position;
    union MoveInfo {
        struct ToWait {
            bool to_wait;
            float time_since_arrived;
            float time_to_wait;
        } to_wait;
        bool to_move;
    } move_info;
    sf::Vector2f walk_towards;
    std::optional<sf::Vector2f> grave_position;
    
    sf::Vector2f makeSpawnPoint() const;

public:
    Gang(std::shared_ptr<GameMap> game_map, const Team team);
    ~Gang() = default;
    // This object should never be copied implicitly!
    Gang(const Gang&) = delete;
    Gang& operator=(const Gang&) = delete;
    Gang(Gang&&) = default;
    Gang& operator=(Gang&&) = default;
    friend std::ostream& operator<<(std::ostream& out, const Gang &gang);

    void addEntity(std::unique_ptr<Entity> entity);

    bool isEmpty() const;
    std::optional<sf::Vector2f> getGravePosition() const;
    sf::Vector2f getAveragePosition() const;

    bool shouldMove() const;
    void stopWalking();
    void walkTowards(const sf::Vector2f new_destination);

    void moveAllEntities(Gang &to);
    void updateWondering(const float dt);
    void updateAggroLoss();
    void update(const float dt);
    void removeDeadTroops();
};

class ChunkIterator {
    friend class GameMap;

    GameMap &map;
    const sf::Vector2u start;
    const sf::Vector2u end;
    sf::Vector2u position;

    ChunkIterator(GameMap &map, const sf::Vector2u start, const sf::Vector2u end);

public:
    // This object should never be copied implicitly!
    ChunkIterator(const ChunkIterator&) = delete;
    ChunkIterator& operator=(const ChunkIterator&) = delete;
    ChunkIterator(ChunkIterator&&) = delete;
    ChunkIterator& operator=(ChunkIterator&&) = delete;
    friend std::ostream& operator<<(std::ostream& out, const ChunkIterator &chunk_iterator);
    std::vector<Entity*> *next();
};

class GameMap {
    friend class Gang;
    friend class ChunkIterator;

    static constexpr float OutsideSpawnWidth = 100.0f;

    const sf::Vector2f chunk_size;
    const sf::Vector2u chunk_amounts;

    const sf::Rect<float> inner_arena;
    const sf::RectangleShape outer_arena;
    
    std::vector<Entity*> all_entities_cache;
    std::vector<std::vector<Entity*>> map;

    std::vector<std::unique_ptr<Projectile>> projectiles;

public:
    GameMap(
        const sf::Vector2f chunk_size, const sf::Vector2u chunk_amounts, 
        const sf::Rect<float> inner_arena, const sf::RectangleShape &outer_arena
    );
    // This object is expensive, and should never be copied implicitly!
    GameMap(const GameMap&) = delete;
    GameMap& operator=(const GameMap&) = delete;
    GameMap(GameMap&&) = default;
    GameMap& operator=(GameMap&&) = delete;
    friend std::ostream& operator<<(std::ostream& out, const GameMap &game_map);

    sf::Rect<float> getInnerArena() const;
    sf::Rect<float> getBoundry() const;
    std::array<sf::Rect<float>, 4> getSpawnRects() const;

private:
    sf::Vector2u getIndex(const sf::Vector2f position) const;
    std::vector<Entity*> &getChunk(const sf::Vector2u index);
    ChunkIterator iterateChunksInRadius(const sf::Vector2f position, const float radius);

    void addToChunks(Entity &entity);

public:
    void reset();
    void updateMovement(const float dt);
    void draw(sf::RenderWindow &window);
};
