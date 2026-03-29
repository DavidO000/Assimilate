#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <math.h>
#include <SFML/Graphics.hpp>
#include "entity.hpp"

Entity::Entity(const EntityTextures &textures_):
    textures(textures_), sprite(textures.getDead()), radius(0.0f),
    wobble_position(0.0f), wobble_amplitude(StandingWobbleAmplitude),
    gang(nullptr), target(nullptr), 
    health(0), time_since_attacked(0.0f),
    time_since_revived(0.0f), time_since_was_attacked(INFINITY) {}

Entity::~Entity() {
    removeFromChunks();
    if(!removeFromVector(&gang->game_map->all_entities_cache, this)) {
        std::cerr << "Entity cache invariance not upheld" << std::endl;
    }
}

std::ostream& operator<<(std::ostream& out, const Entity &entity) {
    out << "Textures: " << entity.textures 
        << ", Position: " << entity.getOrigin().x << ", " << entity.getOrigin().y
        << ", Size: " << entity.sprite.getTexture().getSize().x << ", " << entity.sprite.getTexture().getSize().y
        << ", Scale: " << entity.sprite.getScale().x << ", " << entity.sprite.getScale().y
        << ", Radius: " << entity.getRadius() << " , Health: " << entity.health; 
    return out;
}

sf::Vector2f Entity::getOrigin() const {
    return sprite.getPosition() + sf::Vector2f(radius, -radius);
}

bool Entity::isDead() const {
    return health == 0;
}

void Entity::takeDamage(const unsigned amount) {
    health = subSat(health, amount);
    if(isDead()) {
        target = nullptr;
        setTexture(textures.getDead());
        sprite.setScale({1.0f, -1.0f});
        sprite.setColor(sf::Color::White);

        if(gang->team == Team::Enemy) {
            bool are_all_dead = true;
            for(const auto &comrade: gang->entities) {
                if(!comrade->isDead()) {
                    are_all_dead = false;
                    break;
                }
            }
            if(are_all_dead) {
                gang->grave_position = getOrigin();
            }
        }
    } else {
        time_since_was_attacked = 0.0f;
    }
}

void Entity::takeKnockback(const sf::Vector2f from, const float amount) {
    const sf::Vector2f delta = getOrigin() - from;
    if(delta.length() == 0) return;
    to_move += delta.normalized() * amount;
}

void Entity::draw(sf::RenderWindow &window) const {
    window.draw(sprite);

    if(time_since_revived < 0.3f) {
        sf::ConvexShape revive_triangle(3);
        sf::Vector2f bottom = sprite.getPosition() + sf::Vector2f(sprite.getTexture().getSize()).componentWiseDiv(sf::Vector2f(2, -100));
        float distance_from = (time_since_revived - 0.15) * sprite.getTexture().getSize().x;
        float top = sprite.getTexture().getSize().y * 1.5f - (time_since_revived * time_since_revived + time_since_revived) * 40.0f;
        revive_triangle.setFillColor(sf::Color(255, 255, 255, uint8_t(0.3 / (0.3 - time_since_revived) * 255)));
        revive_triangle.setPoint(0, bottom - sf::Vector2f(0, top));
        revive_triangle.setPoint(1, bottom - sf::Vector2f(distance_from, 0));
        revive_triangle.setPoint(2, bottom + sf::Vector2f(distance_from, 0));
        window.draw(revive_triangle);
    }

    #ifndef NDEBUG
        if(target != nullptr) {
            sf::RectangleShape rect({10.f, 10.0f});
            rect.setFillColor(sf::Color::Green);
            rect.setPosition(getOrigin());
            window.draw(rect);
        }
    #endif
}

Entity* Entity::getTarget() {
    return target;
}

void Entity::setDirection(const Direction direction) {
    const auto texture_size = sprite.getTexture().getSize();
    if(direction == Direction::Left) {
        sprite.setTextureRect(sf::Rect<int>(
            sf::Vector2i(0, texture_size.y), 
            sf::Vector2i(texture_size.x, -int(texture_size.y))
        ));
    } else if(direction == Direction::Right) {
        sprite.setTextureRect(sf::Rect<int>(
            sf::Vector2i(texture_size.x, texture_size.y), 
            sf::Vector2i(-int(texture_size.x), -int(texture_size.y))
        ));
    }
}

void Entity::setTexture(const sf::Texture &texture) {
    sprite.setTexture(texture);
    setDirection(sprite.getTextureRect().size.x > 0 ? Direction::Left : Direction::Right);
}

void Entity::progressWobble(const float desired_amplitude, const float speed, const float dt) {
    wobble_position += speed * dt;
    wobble_amplitude += (desired_amplitude - wobble_amplitude) * std::clamp(dt, 0.0f, 1.0f);
    sprite.setScale({1.0f, -1.0f + sin(wobble_position) * wobble_amplitude});
}

void Entity::addToChunks() {
    auto iterator = gang->game_map->iterateChunksInRadius(getOrigin(), radius);
    while(auto chunk = iterator.next()) {
        chunk->push_back(this);
    }
}

void Entity::removeFromChunks() {
    auto iterator = gang->game_map->iterateChunksInRadius(getOrigin(), radius);
    while(auto chunk = iterator.next()) {
        if(!removeFromVector(chunk, this)) {
            std::cerr << "Chunk invariance not upheld." << std::endl;
        }
    }
}

void Entity::searchAggro(const float search_radius) {
    auto iterator = gang->game_map->iterateChunksInRadius(getOrigin(), search_radius);
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

void Entity::updateAggroLoss() {
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

void Entity::spreadAggro() {
    for(const auto &comrade: gang->entities) {
        if(comrade.get() == this || comrade->isDead() || comrade->target != nullptr) continue;
        const float distance_to_comrade = (getOrigin() - comrade->getOrigin()).length();
        if(distance_to_comrade <= getAggroSpreadRadius()) {
            comrade->target = target;
        }
    }
}

void Entity::askComradesForAggro() {
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

void Entity::updateAggroGain() {
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

void Entity::walkTowards(const sf::Vector2f destination, const float dt) {
    const sf::Vector2f delta = destination - getOrigin();

    if(delta.length() < 0.0) {
        progressWobble(StandingWobbleAmplitude, StandingWobbleSpeed, dt);
        return;
    }

    const auto normalised_delta = delta.normalized() * getSpeed() * dt;
    to_move += delta.length() < normalised_delta.length() ? delta : normalised_delta;

    const Direction new_direction = delta.x < 0.0f ? Direction::Left : Direction::Right;
    setDirection(new_direction);
    progressWobble(MovingWobbleAmplitude, MovingWobbleSpeed, dt);
}

void Entity::update(const float dt) {
    if(isDead()) return;

    updateAggroGain();

    time_since_revived += dt;
    time_since_was_attacked += dt;

    if(time_since_was_attacked < 0.2) sprite.setColor(sf::Color(160, 160, 160));
    else sprite.setColor(sf::Color::White);

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



sf::Vector2f Gang::makeSpawnPoint() const {
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

Gang::Gang(std::shared_ptr<GameMap> game_map, const Team team): 
    team(team), game_map(game_map),
    walk_towards(getRandomRectPosition(game_map->getInnerArena()))
{
    if(team == Team::Player) {
        move_info.to_move = false;
    } else {
        spawn_position = makeSpawnPoint();
        move_info.to_wait.to_wait = false;
        move_info.to_wait.time_since_arrived = 0.0f;
        move_info.to_wait.time_to_wait = 0.0f;
    }
}

std::ostream& operator<<(std::ostream& out, const Gang &gang) {
    out << "Team: " << gang.team
        << ", Entities: " << gang.entities.size()
        << ", Spawn position: " << gang.spawn_position.x << ", " << gang.spawn_position.y
        << ", Walk towards: " << gang.walk_towards.x << ", " << gang.walk_towards.y
        << ", Grave: ";
    if(const auto &grave_position = gang.grave_position) out << grave_position->x << ", " << grave_position->y;
    else out << "None";
    return out;
}

void Gang::addEntity(std::shared_ptr<Gang> gang, std::unique_ptr<Entity> entity) {
    entity->gang = gang;
    entity->health = entity->getInitialHealth();
    entity->radius = entity->getRadius();

    entity->setTexture(entity->textures.getAlive(gang->team));
    entity->sprite.setPosition(clampPoint(gang->spawn_position, gang->game_map->getBoundry()));

    entity->addToChunks();

    gang->game_map->all_entities_cache.push_back(entity.get());
    gang->entities.push_back(std::move(entity));
}

bool Gang::isEmpty() const {
    return entities.size() == 0;
}

std::optional<sf::Vector2f> Gang::getGravePosition() const {
    return grave_position;
}

sf::Vector2f Gang::getAveragePosition() const {
    sf::Vector2f average_position;
    for(const auto &entity: entities) {
        average_position += entity->getOrigin();
    }
    return average_position / float(entities.size());
}

bool Gang::shouldMove() const {
    return (team == Team::Player && move_info.to_move) || (team == Team::Enemy && !move_info.to_wait.to_wait);
}

void Gang::stopWalking() {
    if(team == Team::Player) move_info.to_move = false;
}

void Gang::walkTowards(const sf::Vector2f new_destination) {
    walk_towards = new_destination;
    if(team == Team::Player) move_info.to_move = true;
}

void Gang::updateWondering(const float dt) {
    if(entities.size() == 0) return;

    if(move_info.to_wait.to_wait) {
        move_info.to_wait.time_since_arrived += dt;
        if(move_info.to_wait.time_since_arrived > move_info.to_wait.time_to_wait) {
            move_info.to_wait.to_wait = false;
        }
    }

    if(!move_info.to_wait.to_wait) {
        const float distance_from_destination = (getAveragePosition() - walk_towards).length();
        if(distance_from_destination < OutsideSpawnWidth) {
            move_info.to_wait.to_wait = true;
            move_info.to_wait.time_to_wait = float(rand() % 6 + 1);
            move_info.to_wait.time_since_arrived = 0.0f;
            walk_towards = getRandomRectPosition(game_map->getInnerArena());
        }
    }
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

void Gang::moveAllEntities(std::shared_ptr<Gang> to) {
    for(const auto &entity: entities) {
        entity->health = entity->getInitialHealth();
        entity->gang = to;
        entity->setTexture(entity->textures.getAlive(to->team));
        entity->time_since_was_attacked = INFINITY;
        entity->time_since_revived = 0.0f;
    }
    to->entities.insert(
        to->entities.end(),
        std::make_move_iterator(entities.begin()),
        std::make_move_iterator(entities.end())
    );
    entities.clear();
}

void Gang::updateAggroLoss() {
    for(const auto &entity: entities) {
        entity->updateAggroLoss();
    }
}

void Gang::update(const float dt) {
    for(const auto &entity: entities) {
        entity->update(dt);
    }

    if(team == Team::Enemy) {
        bool any_has_target = false;
        for(const auto &entity: entities) {
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
}



ChunkIterator::ChunkIterator(GameMap &map, const sf::Vector2u start, const sf::Vector2u end): 
    map(map), start(start), end(end), position(start) {}

std::ostream& operator<<(std::ostream& out, const ChunkIterator &chunk_iterator) {
    out << "Start: " << chunk_iterator.start.x << ", " << chunk_iterator.start.y
        << ", End: " << chunk_iterator.end.x << ", " << chunk_iterator.end.y
        << ", Position: " << chunk_iterator.position.x << ", " << chunk_iterator.position.y;
    return out;
}

std::vector<Entity*> *ChunkIterator::next() {
    if(position.x > end.x || position.y > end.y) return nullptr;

    auto chunk = &map.getChunk(position);

    position.x++;
    if(position.x > end.x) {
        position.y++;
        position.x = start.x;
    }

    return chunk;
}

GameMap::GameMap(
    const sf::Vector2f chunk_size, const sf::Vector2u chunk_amounts, 
    const sf::Rect<float> inner_arena, const sf::RectangleShape &outer_arena
): chunk_size(chunk_size), chunk_amounts(chunk_amounts), inner_arena(inner_arena), outer_arena(outer_arena) {
    for(unsigned i = 0; i < chunk_amounts.x * chunk_amounts.y; i++) map.push_back({});
}

std::ostream& operator<<(std::ostream& out, const GameMap &game_map) {
    out << "Entities: " << game_map.all_entities_cache.size()
        << ", Chunk size: " << game_map.chunk_size.x << ", " << game_map.chunk_size.x
        << ", Chunk amounts: " << game_map.chunk_amounts.x << ", " << game_map.chunk_amounts.y;
    return out;
}

sf::Rect<float> GameMap::getInnerArena() const {
    return inner_arena;
}

sf::Rect<float> GameMap::getBoundry() const {
    sf::Vector2f rectangle_size = chunk_size.componentWiseMul(sf::Vector2f(chunk_amounts));
    static constexpr sf::Vector2f iota(0.1f, 0.1f);
    return {-rectangle_size / 2.0f, rectangle_size - iota};
}

sf::Vector2u GameMap::getIndex(const sf::Vector2f position) const {
    const sf::Vector2f middle_to_center = chunk_size.componentWiseMul(sf::Vector2f(chunk_amounts)) / 2.0f;
    const sf::Vector2u absolute_position(position + middle_to_center);
    const sf::Vector2u index = sf::Vector2u(absolute_position.x / chunk_size.x, absolute_position.y / chunk_size.y);
    return index;
}

std::vector<Entity*> &GameMap::getChunk(const sf::Vector2u index) {
    if(index.x >= chunk_amounts.x || index.y >= chunk_amounts.y) throw "error";
    return map[index.x * chunk_amounts.x + index.y];
}

ChunkIterator GameMap::iterateChunksInRadius(const sf::Vector2f position, const float radius) {
    const sf::Vector2u chunk_index = getIndex(position);
    const sf::Vector2f rangef = sf::Vector2f(radius, radius).componentWiseDiv(chunk_size);
    const sf::Vector2u range(ceilf(rangef.x), ceilf(rangef.y));

    const sf::Vector2u start(subSat(chunk_index.x, range.x), subSat(chunk_index.y, range.y));
    const sf::Vector2u end(
        std::min(chunk_index.x + range.x, chunk_amounts.x - 1), 
        std::min(chunk_index.y + range.y, chunk_amounts.y - 1));

    return ChunkIterator(*this, start, end);
}

void GameMap::reset() {
    all_entities_cache.clear();
}

void GameMap::updateMovement() {

    struct CollisionPair {
        Entity* a;
        Entity* b;
    };
    std::vector<CollisionPair> pairs;

    for(auto &entity: all_entities_cache) {
        if(entity->isDead()) continue;
        auto iterator = iterateChunksInRadius(entity->getOrigin(), entity->radius);
        while(const auto *const chunk = iterator.next()) {
            for(auto other: *chunk) {
                if(other <= entity || other->isDead()) continue;
                pairs.push_back({entity, other});
            }
        }
    }

    for(auto &chunk: map) chunk.clear();

    static constexpr unsigned MaxCollisionIterations = 10;
    for(unsigned i = 0; i < MaxCollisionIterations; i++) {
        for(auto &entity: all_entities_cache) {
            if(entity->isDead()) continue;
            entity->sprite.move(entity->to_move / float(MaxCollisionIterations));
        }

        for(auto &[entity, other]: pairs) {
            const sf::Vector2f delta = entity->getOrigin() - other->getOrigin();
            const float maximum_distance = entity->radius + other->radius;
            const float to_push_length = maximum_distance - delta.length();
            if(to_push_length <= 0) continue;

            const auto angle = delta.length() == 0.0f ? sf::radians(float(rand())) : delta.angle();
            const sf::Vector2f to_push(to_push_length, angle);
            entity->sprite.move(to_push * 0.5f);
            other->sprite.move(-to_push * 0.5f);
        }
    }

    for(auto &entity: all_entities_cache) {
        entity->to_move = {0.0f, 0.0f};
        if(entity->isDead()) continue;
        entity->addToChunks();
    }
}

void GameMap::draw(sf::RenderWindow &window) {
    window.draw(outer_arena);
    std::sort(all_entities_cache.begin(), all_entities_cache.end(), 
        [](const Entity *x, const Entity *y) {
            if(x->isDead() && !y->isDead()) return true;
            if(!x->isDead() && y->isDead()) return false;
            return x->sprite.getPosition().y < y->sprite.getPosition().y; 
        });
    for(const Entity *entity: all_entities_cache) {
        entity->draw(window);
    }
}