#include <iostream>
#include <vector>
#include <algorithm>
#include <limits>
#include <SFML/Graphics.hpp>
#include "entity.hpp"

Entity::Entity(const EntityTextures &textures): 
    textures(textures), gang(nullptr), target(nullptr), 
    wobble_position(0.0f), wobble_amplitude(StandingWobbleAmplitude),
    time_since_attacked(0.0f) {}

Entity::~Entity() {
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

sf::Vector2f Entity::getOrigin() const {
    return shape.getPosition() + shape.getSize().componentWiseMul({1.0f, -1.0f}) / 2.0f;
}

bool Entity::isDead() const {
    return health == 0;
}

void Entity::takeDamage(const unsigned amount) {
    health = subSat(health, amount);
    if(isDead()) {
        target = nullptr;
        setTexture(textures.getDead());
        shape.setScale({1.0f, -1.0f});

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
    }
}

void Entity::takeKnockback(const sf::Vector2f from, const float amount) {
    const sf::Vector2f delta = getOrigin() - from;
    if(delta.length() == 0) return;
    move(delta.normalized() * amount);
}

void Entity::draw(sf::RenderWindow &window) const {
    window.draw(shape);
    const bool Debugging = true;
    if(Debugging && target != nullptr) {
        sf::RectangleShape rect({10.f, 10.0f});
        rect.setFillColor(sf::Color::Green);
        rect.setPosition(getOrigin());
        window.draw(rect);
    }
}

Entity* Entity::getTarget() {
    return target;
}

void Entity::setTexture(const sf::Texture &texture) {
    shape.setTexture(&texture);
    shape.setSize(sf::Vector2f(texture.getSize()));
    setDirection(getDirection());
}

void Entity::setDirection(const Direction direction) {
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

Entity::Direction Entity::getDirection() const {
    return shape.getTextureRect().size.x > 0 ? Direction::Left : Direction::Right;
}

void Entity::progressWobble(const float desired_amplitude, const float speed, const float dt) {
    wobble_position += speed * dt;
    wobble_amplitude += (desired_amplitude - wobble_amplitude) * std::clamp(dt, 0.0f, 1.0f);
    shape.setScale({1.0f, -1.0f + sin(wobble_position) * wobble_amplitude});
}

void Entity::addToChunks() {
    auto iterator = gang->game_map->iterateChunksInRadius(shape.getPosition(), radius);
    while(auto chunk = iterator.next()) {
        chunk->push_back(this);
    }
}

void Entity::removeFromChunks(const sf::Vector2f old_position) {
    auto iterator = gang->game_map->iterateChunksInRadius(old_position, radius);
    while(auto chunk = iterator.next()) {
        for(unsigned i = 0; i < chunk->size(); i++) {
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

void Entity::move(const sf::Vector2f offset) {
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

void Entity::walkTowards(const sf::Vector2f destination, const float dt) {
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

float Entity::updateCollision() {
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

void Entity::searchAggro(const float search_radius) {
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

void Entity::update(const float dt) {
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

Gang::Gang(std::shared_ptr<GameMap> game_map, const Team team): team(team), game_map(game_map) {
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

void Gang::addEntity(std::shared_ptr<Gang> gang, std::unique_ptr<Entity> entity) {
    entity->gang = gang;
    entity->health = entity->getInitialHealth();
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

void Gang::updateCollision() {
    static constexpr unsigned MaxCollisionIterations = 10;
    static constexpr float MinSquabblingCutoff = 10.0;
    for(unsigned i = 0; i < MaxCollisionIterations; i++) {
        float squabbling = 0.0f;
        for(auto &entity: entities) {
            squabbling += entity->updateCollision();
        }
        if(squabbling < MinSquabblingCutoff) break;
    }
}

void Gang::moveAllEntities(std::shared_ptr<Gang> to) {
    for(auto &entity: entities) {
        entity->health = entity->getInitialHealth();
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

    for(auto &entity: entities) {
        entity->updateAggroLoss();
    }

    updateCollision();
}



ChunkIterator::ChunkIterator(GameMap &map, const sf::Vector2u start, const sf::Vector2u end): 
    map(map), start(start), end(end), position(start) {}

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

sf::Rect<float> GameMap::getInnerArena() const {
    return inner_arena;
}

const sf::RectangleShape &GameMap::getOuterArena() const {
    return outer_arena;
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

void GameMap::draw(sf::RenderWindow &window) {
    window.draw(getOuterArena());
    std::sort(all_entities_cache.begin(), all_entities_cache.end(), 
        [](const Entity *x, const Entity *y) {
            if(x->isDead() && !y->isDead()) return true;
            if(!x->isDead() && y->isDead()) return false;
            return x->getOrigin().y < y->getOrigin().y; 
        });
    for(const Entity *entity: all_entities_cache) {
        entity->draw(window);
    }
}