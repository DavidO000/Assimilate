#include <iostream>
#include <vector>
#include <SFML/Graphics.hpp>
#include "utils.cpp"
#include "entity_builder.cpp"

class Entity;
class GameMap {
    friend class Entity;
    friend class Gang;

    std::vector<Entity*> all_entities_cache;
    std::vector<std::vector<Entity*>> map;
    const sf::Vector2f chunk_size;
    const sf::Vector2u chunk_amounts;
    const sf::Rect<float> inner_arena;
    const sf::RectangleShape outer_arena;
public:
    GameMap(
        const sf::Vector2f chunk_size, const sf::Vector2u chunk_amounts, 
        const sf::Rect<float> inner_arena, const sf::RectangleShape &outer_arena
    ): chunk_size(chunk_size), chunk_amounts(chunk_amounts), inner_arena(inner_arena), outer_arena(outer_arena) {
        for(unsigned i = 0; i < chunk_amounts.x * chunk_amounts.y; i++) map.push_back({});
    }

    sf::Rect<float> getInnerArena() const {
        return inner_arena;
    }

private:
    const sf::RectangleShape &getOuterArena() const {
        return outer_arena;
    }

    sf::Vector2u getIndex(const sf::Vector2f position) const {
        const sf::Vector2f middle_to_center = chunk_size.componentWiseMul(sf::Vector2f(chunk_amounts)) / 2.0f;
        const sf::Vector2u absolute_position(position + middle_to_center);
        const sf::Vector2u index = sf::Vector2u(absolute_position.x / chunk_size.x, absolute_position.y / chunk_size.y);
        return index;
    }

    std::vector<Entity*> &getChunk(const sf::Vector2u index) {
        if(index.x >= chunk_amounts.x || index.y >= chunk_amounts.y) throw "error";
        return map[index.x * chunk_amounts.x + index.y];
    }

    class ChunkIterator {
        friend class GameMap;

        GameMap &map;
        const sf::Vector2u start;
        const sf::Vector2u end;
        sf::Vector2u position;

        ChunkIterator(GameMap &map, const sf::Vector2u start, const sf::Vector2u end): 
            map(map), start(start), end(end), position(start) {}

    public:
        std::vector<Entity*> *next() {
            if(position.x > end.x || position.y > end.y) return nullptr;

            auto chunk = &map.getChunk(position);

            position.x++;
            if(position.x > end.x) {
                position.y++;
                position.x = start.x;
            }

            return chunk;
        }
    };
    
    ChunkIterator iterateChunksInRadius(const sf::Vector2f position, const float radius) {
        const sf::Vector2u chunk_index = getIndex(position);
        const sf::Vector2f rangef = sf::Vector2f(radius, radius).componentWiseDiv(chunk_size);
        const sf::Vector2u range(ceilf(rangef.x), ceilf(rangef.y));

        const sf::Vector2u start(subSat(chunk_index.x, range.x), subSat(chunk_index.y, range.y));
        const sf::Vector2u end(
            std::min(chunk_index.x + range.x, chunk_amounts.x - 1), 
            std::min(chunk_index.y + range.y, chunk_amounts.y - 1));

        return ChunkIterator(*this, start, end);
    }

    sf::Rect<float> getBoundry() const {
        sf::Vector2f rectangle_size = chunk_size.componentWiseMul(sf::Vector2f(chunk_amounts));
        static constexpr sf::Vector2f iota(0.1f, 0.1f);
        return {-rectangle_size / 2.0f, rectangle_size - iota};
    }

public:
    void draw(sf::RenderWindow &window);
};

class Gang {
    friend class Entity;
    static constexpr float OutsideSpawnWidth = 100.0f;

    const Team team;
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
    
    sf::Vector2f makeSpawnPoint() const {
        const sf::Rect<float> arena = game_map->getInnerArena();
        const sf::Rect<float> spawnable_points[4] {
            { // up
                {arena.position.x, arena.position.y - OutsideSpawnWidth},
                {arena.size.x, OutsideSpawnWidth}
            },
            { // left
                {arena.position.x - OutsideSpawnWidth, arena.position.y},
                {OutsideSpawnWidth, arena.size.y}
            },
            { // down
                {arena.position.x, arena.position.y + arena.size.y},
                {arena.size.x, OutsideSpawnWidth}
            },
            { // right
                {arena.position.x + arena.size.x, arena.position.y},
                {OutsideSpawnWidth, arena.size.y}
            },
        };

        return getRandomRectPosition(spawnable_points[rand() % 4]);
    }

public:
    Gang(std::shared_ptr<GameMap> game_map, const Team team): game_map(game_map), team(team) {
        walk_towards = getRandomRectPosition(game_map->getInnerArena());
        if(team == Team::Player) {
            move_info.to_move = false;
        } else {
            spawn_position = makeSpawnPoint();
            move_info.to_wait.to_wait = false;
            move_info.to_wait.time_since_arrived = 0.0f;
            move_info.to_wait.time_to_wait = 0.0f;
        }
    }

    Team getTeam() const {
        return team;
    }

    bool isEmpty() const {
        return entities.size() == 0;
    }

    std::optional<sf::Vector2f> getGravePosition() const {
        return grave_position;
    }

    void stopWalking() {
        if(team == Team::Player) move_info.to_move = false;
    }

    bool shouldMove() const {
        return (team == Team::Player && move_info.to_move) || (team == Team::Enemy && !move_info.to_wait.to_wait);
    }

    void walkTowards(const sf::Vector2f new_destination) {
        walk_towards = new_destination;
        if(team == Team::Player) move_info.to_move = true;
    }

    void updateWondering(const float dt) {
        if(entities.size() == 0) return;

        if(move_info.to_wait.to_wait) {
            move_info.to_wait.time_since_arrived += dt;
            if(move_info.to_wait.time_since_arrived > move_info.to_wait.time_to_wait) {
                move_info.to_wait.to_wait = false;
            }
        }

        if(!move_info.to_wait.to_wait) {
            const float distance_from_destination = (getAveragePosition() - walk_towards).length();
            if(distance_from_destination < OutsideSpawnWidth * 2.0f) {
                move_info.to_wait.to_wait = true;
                move_info.to_wait.time_to_wait = float(rand() % 6 + 1);
                move_info.to_wait.time_since_arrived = 0.0f;
                walk_towards = getRandomRectPosition(game_map->getInnerArena());
            }
        }
    }

    static void addEntity(std::shared_ptr<Gang> gang, std::unique_ptr<Entity> entity);
    sf::Vector2f getAveragePosition();
    void moveAllEntities(std::shared_ptr<Gang> to);
    void removeDeadTroops();
    void updateCollision();
    void update(const float dt);
};

class Entity {
    friend class Gang;

    static constexpr float StandingWobbleAmplitude = 0.05f;
    static constexpr float StandingWobbleSpeed = 2.5f;
    static constexpr float MovingWobbleAmplitude = 0.075f;
    static constexpr float MovingWobbleSpeed = 12.0f;

    std::shared_ptr<Gang> gang;
    sf::RectangleShape shape;
    float wobble_position;
    float wobble_amplitude;
    unsigned health;
    float radius;
    float time_since_attacked;
    Entity *target;
    const EntityTextures &textures;

public:
    Entity(const EntityTextures &textures): textures(textures) {
        gang = nullptr;
        target = nullptr;
        time_since_attacked = 0.0f;

        shape = sf::RectangleShape(sf::Vector2f(0.0f, 0.0f));
        wobble_amplitude = StandingWobbleAmplitude;
        wobble_position = 0.0;
    }

    virtual ~Entity() {
        removeFromChunks(shape.getPosition());
        auto &all_entitites = gang->game_map->all_entities_cache;
        for(unsigned i = 0; i < all_entitites.size(); i++) {
            if(all_entitites.at(i) == this) {
                all_entitites.at(i) = all_entitites.at(all_entitites.size() - 1);
                all_entitites.pop_back(); 
                goto found;
            }
        }
        std::cerr << "Entity cache invariance not upheld" << std::endl;
        found:;
    }

    sf::Vector2f getOrigin() const {
        return shape.getPosition() + shape.getSize().componentWiseMul({1.0f, -1.0f}) / 2.0f;
    }

    bool isDead() const {
        return health == 0;
    }

    void takeDamage(const unsigned amount) {
        health = subSat(health, amount);
        if(isDead()) {
            target = nullptr;
            setTexture(textures.getDead());
            shape.setScale({1.0f, -1.0f});

            if(gang->team == Team::Enemy) {
                bool are_all_dead = true;
                for(auto &comrade: gang->entities) {
                    if(!comrade->isDead()) {
                        are_all_dead = false;
                        break;
                    }
                }
                if(are_all_dead) {
                    gang->grave_position = getOrigin();
                }
            }
        }
    }

    void takeKnockback(const sf::Vector2f from, const float amount) {
        const sf::Vector2f delta = getOrigin() - from;
        if(delta.length() == 0) return;
        move(delta.normalized() * amount);
    }

    void draw(sf::RenderWindow &window) const {
        window.draw(shape);
        const bool Debugging = true;
        if(Debugging && target != nullptr) {
            sf::RectangleShape rect({10.f, 10.0f});
            rect.setFillColor(sf::Color::Green);
            rect.setPosition(getOrigin());
            window.draw(rect);
        }
    }

    virtual float getRadius() const = 0;
    virtual float getSpeed() const = 0;
    virtual unsigned getMaximumHealth() const = 0;
    virtual unsigned getInitialHealth() const = 0;
    virtual float getAggroRadius() const = 0;
    virtual float getLoseAggroRadius() const = 0;
    virtual float getAggroSpreadRadius() const = 0;
    virtual float getAttackRadius() const = 0;
    virtual float getPrefferedAttackRadius() const = 0;
    virtual float getAttackSpeed() const = 0;
    virtual void attack() = 0;

protected:
    Entity* getTarget() {
        return target;
    }

public:
    enum class Direction { Left, Right };
private:
    Direction getDirection() const {
        return shape.getTextureRect().size.x > 0 ? Direction::Left : Direction::Right;
    }

    void setDirection(const Direction direction) {
        const auto texture_size = shape.getTexture()->getSize();
        if(direction == Direction::Left) {
            shape.setTextureRect(sf::Rect<int>(
                sf::Vector2i(0, texture_size.y), 
                sf::Vector2i(texture_size.x, -int(texture_size.y))
            ));
        } else if(direction == Direction::Right) {
            shape.setTextureRect(sf::Rect<int>(
                sf::Vector2i(texture_size.x, texture_size.y), 
                sf::Vector2i(-int(texture_size.x), -int(texture_size.y))
            ));
        }
    }

    void setTexture(const sf::Texture &texture) {
        shape.setTexture(&texture);
        shape.setSize(debug(sf::Vector2f(texture.getSize())));
        setDirection(getDirection());
    }
    
    void progressWobble(const float desired_amplitude, const float speed, const float dt) {
        wobble_position += speed * dt;
        wobble_amplitude += (desired_amplitude - wobble_amplitude) * std::clamp(dt, 0.0f, 1.0f);
        shape.setScale({1.0f, -1.0f + sin(wobble_position) * wobble_amplitude});
    }

    void addToChunks() {
        auto iterator = gang->game_map->iterateChunksInRadius(shape.getPosition(), radius);
        while(auto chunk = iterator.next()) {
            chunk->push_back(this);
        }
    }

    void removeFromChunks(const sf::Vector2f old_position) {
        auto iterator = gang->game_map->iterateChunksInRadius(old_position, radius);
        while(auto chunk = iterator.next()) {
            for(int i = 0; i < chunk->size(); i++) {
                if(chunk->at(i) == this) {
                    chunk->at(i) = chunk->at(chunk->size() - 1);
                    chunk->pop_back();
                    goto next_chunk;
                }
            }
            std::cerr << "Chunk invariance not upheld." << std::endl;
            next_chunk:; // the compiler requires a semicolon here for some reason
        }
    }

    void move(const sf::Vector2f offset) {
        const sf::Vector2u old_index = gang->game_map->getIndex(shape.getPosition());
        const sf::Vector2f old_position = shape.getPosition();
        shape.move(offset);
        shape.setPosition(clampPoint(shape.getPosition(), gang->game_map->getBoundry()));
        const sf::Vector2u new_index = gang->game_map->getIndex(shape.getPosition());
        if(old_index != new_index) {
            removeFromChunks(old_position);
            addToChunks();
        }
    }

    void walkTowards(const sf::Vector2f destination, const float dt) {
        const sf::Vector2f delta = destination - getOrigin();

        if(delta.length() == 0.0) {
            progressWobble(StandingWobbleAmplitude, StandingWobbleSpeed, dt);
            return;
        }

        const auto normalised_delta = delta.normalized() * getSpeed() * dt;
        const auto to_walk = delta.length() < normalised_delta.length() ? delta : normalised_delta;
        move(to_walk);

        const Direction new_direction = delta.x < 0.0f ? Direction::Left : Direction::Right;
        setDirection(new_direction);
        progressWobble(MovingWobbleAmplitude, MovingWobbleSpeed, dt);
    }

    float updateCollision() {
        if(isDead()) return 0.0f;

        float squabbling = 0.0f;
        auto iterator = gang->game_map->iterateChunksInRadius(shape.getPosition(), radius);
        while(const auto chunk = iterator.next()) {
            for(unsigned i = 0; i < chunk->size(); i++) {
                const auto other = chunk->at(i);
                if(other == this || other->isDead()) continue;

                const sf::Vector2f delta = getOrigin() - other->getOrigin();
                const float maximum_distance = radius + other->radius;
                const float to_push_length = maximum_distance - delta.length();
                if(to_push_length <= 0) continue;

                const auto angle = delta.length() == 0.0f ? sf::radians(float(rand())) : delta.angle();
                const sf::Vector2f to_push(to_push_length, angle);

                move(to_push * 0.5f);
                other->move(-to_push * 0.5f);
                squabbling += to_push.length();
            }
        }
        return squabbling;
    }

    void searchAggro(const float search_radius) {
        auto iterator = gang->game_map->iterateChunksInRadius(shape.getPosition(), search_radius);
        while(auto chunk = iterator.next()) {
            for(unsigned i = 0; i < chunk->size(); i++) {
                const auto other = chunk->at(i);
                if(other == this || other->isDead()) continue;
                const bool are_same_team = gang->team == other->gang->team;
                if(are_same_team) continue;

                const sf::Vector2f delta = getOrigin() - other->getOrigin();
                const float distance = delta.length();
                if(distance > getAggroRadius()) continue;

                if(target == nullptr) {
                    target = other;
                } else {
                    const float current_distance = (getOrigin() - target->getOrigin()).length();
                    if(distance < current_distance) {
                        target = other;
                    }
                }
            }
        }
    }

    void updateAggroLoss() {
        if(isDead()) return;

        if(target != nullptr) {
            if(target->isDead()) {
                target = nullptr;
                searchAggro(getLoseAggroRadius());
            } else {
                const float distance_to_target = (getOrigin() - target->getOrigin()).length();
                if(distance_to_target > getLoseAggroRadius()) {
                    target = nullptr;
                }
            }
        }
    }

    void spreadAggro() {
        for(const auto &comrade: gang->entities) {
            if(comrade.get() == this || comrade->isDead() || comrade->target != nullptr) continue;
            const float distance_to_comrade = (getOrigin() - comrade->getOrigin()).length();
            if(distance_to_comrade <= getAggroSpreadRadius()) {
                comrade->target = target;
            }
        }
    }

    void askComradesForAggro() {
        float distance_for_current_aggro = INFINITY;
        for(auto &comrade: gang->entities) {
            if(comrade.get() == this || comrade->isDead() || comrade->target == nullptr) continue;
            const float distance_to_comrade = (getOrigin() - comrade->getOrigin()).length();
            if(distance_to_comrade < distance_for_current_aggro) {
                target = comrade->target;
                distance_for_current_aggro = distance_to_comrade;
            }
        }
    }

    void updateAggroGain() {
        if(isDead()) return;
        
        if(gang->team == Team::Player) {
            const Entity* old_target = target;
            searchAggro(getAggroRadius());
            if(old_target != target && target != nullptr) {
                spreadAggro();
            }
        } else {
            searchAggro(getAggroRadius());
            if(target == nullptr) {
                askComradesForAggro();
            }
        }
    }

    void update(const float dt) {
        if(isDead()) return;

        updateAggroLoss();
        updateAggroGain();

        if(target == nullptr) {
            if(gang->shouldMove()) {
                walkTowards(gang->walk_towards, dt);
            } else {
                progressWobble(StandingWobbleAmplitude, StandingWobbleSpeed, dt);
            }
        } else {
            const float distance_to_target = (getOrigin() - target->getOrigin()).length();
            if(distance_to_target > getPrefferedAttackRadius()) {
                walkTowards(target->getOrigin(), dt);
            }
            if(distance_to_target < getAttackRadius()) {
                time_since_attacked += dt;
                if(time_since_attacked > getAttackSpeed()) {
                    attack();
                    time_since_attacked = 0.0f;
                }
            }
        }
    }
};

void Gang::addEntity(std::shared_ptr<Gang> gang, std::unique_ptr<Entity> entity) {
    entity->gang = gang;
    entity->health = entity->getMaximumHealth();
    entity->radius = entity->getRadius();

    entity->setTexture(entity->textures.getAlive(gang->team));
    entity->shape.setPosition(gang->spawn_position);
    entity->shape.setPosition(clampPoint(entity->shape.getPosition(), gang->game_map->getBoundry()));

    auto iterator = gang->game_map->iterateChunksInRadius(entity->shape.getPosition(), entity->radius);
    while(auto chunk = iterator.next()) {
        chunk->push_back(entity.get());
    }

    gang->game_map->all_entities_cache.push_back(entity.get());
    gang->entities.push_back(std::move(entity));
}

sf::Vector2f Gang::getAveragePosition() {
    sf::Vector2f average_position;
    for(const auto &entity: entities) {
        average_position += entity->getOrigin();
    }
    return average_position / float(entities.size());
}

void Gang::moveAllEntities(std::shared_ptr<Gang> to) {
    for(auto &entity: entities) {
        entity->health = entity->getMaximumHealth();
        entity->gang = to;
        entity->setTexture(entity->textures.getAlive(to->team));
    }
    to->entities.insert(
        to->entities.end(),
        std::make_move_iterator(entities.begin()),
        std::make_move_iterator(entities.end())
    );
    entities.clear();
}

void Gang::removeDeadTroops() {
    for(unsigned i = 0; i < entities.size();) {
        if(entities[i]->isDead()) {
            std::swap(entities[i], entities[entities.size() - 1]);
            entities.pop_back();
        } else {
            i++;
        }
    }
}

void Gang::updateCollision() {
    for(int i = 0; i < 10; i++) {
        float squabbling = 0.0f;
        for(auto &entity: entities) {
            squabbling += entity->updateCollision();
        }
        if(squabbling < 10.0f) break;
    }
}

void Gang::update(const float dt) {
    for(const auto &entity: entities) {
        entity->update(dt);
    }

    if(team == Team::Enemy) {
        bool any_has_target = false;
        for(auto &entity: entities) {
            if(entity->target != nullptr) {
                any_has_target = true;
                break;
            }
        }
        if(!any_has_target)
            updateWondering(dt);
    } else if(team == Team::Player) {
        removeDeadTroops();
    }

    for(auto &entity: entities) {
        entity->updateAggroLoss();
    }

    updateCollision();
}

void GameMap::draw(sf::RenderWindow &window) {
    window.draw(getOuterArena());
    std::sort(all_entities_cache.begin(), all_entities_cache.end(), 
        [](Entity *x, Entity *y) {
            if(x->isDead() && !y->isDead()) return true;
            if(!x->isDead() && y->isDead()) return false;
            return x->getOrigin().y < y->getOrigin().y; 
        });
    for(const Entity *entity: all_entities_cache) {
        entity->draw(window);
    }
}