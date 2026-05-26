#include "troops.cpp"

Projectile::Projectile(const sf::Texture &texture, Entity &entity, const float speed): 
    team(entity.team), to_delete(false), sprite(sf::Sprite(texture)) 
{
    sf::Vector2f delta = entity.getTarget()->getOrigin() - entity.getOrigin();
    if(delta == sf::Vector2f()) {
        to_delete = true;
        velocity = delta;
    } else {
        velocity = delta.normalized() * speed;
    }

    sprite.setPosition(entity.getOrigin() - sf::Vector2f(sprite.getTexture().getSize()) / 2.0f);
}

sf::Vector2f Projectile::getOrigin() const {
    return sprite.getPosition() + sf::Vector2f(sprite.getTexture().getSize()) / 2.0f;
}

Entity::Entity(const EntityTextures &textures_):
    textures(textures_), sprite(textures.getDead()), radius(0.0f),
    wobble_position(0.0f), wobble_amplitude(StandingWobbleAmplitude),
    team(Team::Enemy), target(nullptr),
    aoe_damage(0), aoe_knockback(0.0),
    health(0), attack_counter(0.0f), time_since_attacked(INFINITY),
    time_since_revived(0.0f), time_since_was_attacked(INFINITY) {}

std::ostream &Entity::print(std::ostream &out) const {
    return out << "Textures: " << textures 
        << ", Position: " << getOrigin().x << ", " << getOrigin().y
        << ", Size: " << sprite.getTexture().getSize().x << ", " << sprite.getTexture().getSize().y
        << ", Scale: " << sprite.getScale().x << ", " << sprite.getScale().y
        << ", Radius: " << getRadius() << " , Health: " << health; 
}

std::ostream &operator<<(std::ostream &out, const Entity &entity) {
    return entity.print(out);
}

Team Entity::getTeam() const {
    return team;
}

sf::Vector2f Entity::getOrigin() const {
    sf::Vector2f size = sf::Vector2f(sprite.getTextureRect().size);
    return sprite.getPosition() + sf::Vector2f(abs(size.x / 2.0f), -radius);
}

float Entity::getDistanceBetween(const Entity &other) const {
    float distance_from_origins = (getOrigin() - other.getOrigin()).length();
    float distance_between = distance_from_origins - radius - other.radius;
    return distance_between;
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
    } else {
        time_since_was_attacked = 0.0f;
    }
}

void Entity::takeKnockback(const sf::Vector2f from, const float amount) {
    const sf::Vector2f delta = getOrigin() - from;
    if(delta.length() == 0) return;
    knocked_back += delta.normalized() * amount;
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

        sf::CircleShape circle(radius);
        circle.setPosition(getOrigin() - sf::Vector2f(radius, radius));
        circle.setFillColor(sf::Color(0, 255, 0, 32));
        window.draw(circle);
    #endif
}

Entity* Entity::getTarget() {
    return target;
}

void Entity::setAOEDamage(const unsigned amount) {
    aoe_damage = amount;
}

void Entity::setAOEKnockback(const float amount) {
    aoe_knockback = amount;
}

void Entity::throwProjectile(std::unique_ptr<Projectile> projectile) {
    projectiles.push_back(std::move(projectile));
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

void Entity::updateAggroLoss() {
    if(isDead()) return;

    if(target != nullptr && (target->isDead() || (getOrigin() - target->getOrigin()).length() > getLoseAggroRadius())) {
        target = nullptr;
        attack_counter = 0.0f;
    }
}

void Entity::walkTowards(const sf::Vector2f destination, const float dt) {
    const sf::Vector2f delta = destination - getOrigin();

    if(delta.length() <= 0.0) {
        progressWobble(StandingWobbleAmplitude, StandingWobbleSpeed, dt);
        return;
    }

    const float desired_movement = getSpeed() * dt;
    to_move += desired_movement < delta.length() ? desired_movement * delta.normalized() : delta;

    const Direction new_direction = delta.x < 0.0f ? Direction::Left : Direction::Right;
    setDirection(new_direction);
    progressWobble(MovingWobbleAmplitude, MovingWobbleSpeed, dt);
}

void Entity::update(const float dt) {
    time_since_revived += dt;
    time_since_was_attacked += dt;
    time_since_attacked += dt;

    if(time_since_was_attacked < 0.2) sprite.setColor(sf::Color(160, 160, 160));
    else sprite.setColor(sf::Color::White);

    if(time_since_attacked > getAttackDuration()) {
        setTexture(textures.getAlive(team));
    }
}



sf::Vector2f Gang::makeSpawnPoint() const {
    return getRandomRectPosition(game_map->getSpawnRects()[rand() % 4]);
}

Gang::Gang(std::shared_ptr<GameMap> game_map, const Team team): 
    team(team), game_map(game_map),
    walk_towards(getRandomRectPosition(game_map->getInnerArena()))
{
    if(team == Team::Player) {
        move_info.to_move = false;
    } else if(team == Team::Enemy) {
        spawn_position = makeSpawnPoint();
        move_info.to_wait.to_wait = false;
        move_info.to_wait.time_since_arrived = 0.0f;
        move_info.to_wait.time_to_wait = 0.0f;
    } else {
        const UnknownTeam unknown_team(team);
        throw unknown_team;
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

void Gang::addEntityInternal(std::unique_ptr<Entity> entity) {
    entity->team = team;
    entity->health = entity->getInitialHealth();
    entity->radius = entity->getRadius();

    entity->setTexture(entity->textures.getAlive(team));
    entity->sprite.setPosition(clampPoint(spawn_position, game_map->getBoundry()));

	game_map->addToChunks(*entity);

    game_map->all_entities_cache.push_back(entity.get());
    entities.push_back(std::move(entity));
}

void Gang::addEntity(EntityType entity_type) {
    switch(entity_type) {
    case EntityType::Grunt:
        addEntityInternal(std::make_unique<Grunt>());
        break;
    case EntityType::Knight:
        addEntityInternal(std::make_unique<Knight>());
        break;
    case EntityType::Samurai:
        addEntityInternal(std::make_unique<Samurai>());
        break;
    case EntityType::Mage:
        addEntityInternal(std::make_unique<Mage>());
        break;
    case EntityType::Minitroll:
        addEntityInternal(std::make_unique<Minitroll>());
        break;
    case EntityType::Troll:
        addEntityInternal(std::make_unique<Troll>());
        break;
    default:
        // impossible
        break;
    }
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
        if(distance_from_destination < GameMap::OutsideSpawnWidth) {
            move_info.to_wait.to_wait = true;
            move_info.to_wait.time_to_wait = float(rand() % 6 + 1);
            move_info.to_wait.time_since_arrived = 0.0f;
            walk_towards = getRandomRectPosition(game_map->getInnerArena());
        }
    }
}

void Gang::moveAllEntities(Gang &to) {
    for(const auto &entity: entities) {
        entity->health = entity->getInitialHealth();
        entity->team = to.team;
        entity->setTexture(entity->textures.getAlive(to.team));
        entity->attack_counter = 0.0f;
        entity->time_since_attacked = INFINITY;
        entity->time_since_was_attacked = INFINITY;
        entity->time_since_revived = 0.0f;
        entity->to_move = {0.0f, 0.0f};
        entity->knocked_back = {0.0f, 0.0f};
        entity->aoe_damage = 0;
    }
    to.entities.insert(
        to.entities.end(),
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
        if(entity->isDead()) continue;

        entity->update(dt);

		const Entity* old_target = entity->target;
		// search aggro
		auto iterator = game_map->iterateChunksInRadius(entity->getOrigin(), entity->getAggroRadius());
		while(auto chunk = iterator.next()) {
			for(unsigned i = 0; i < chunk->size(); i++) {
				const auto other = chunk->at(i);
				if(other->isDead() || !isEnemyTeam(team, other->team)) continue;

				const float distance_between = entity->getDistanceBetween(*other);
				if(distance_between > entity->getAggroRadius()) continue;

				if(entity->target == nullptr) {
					entity->target = other;
				} else {
					const float current_distance = entity->getDistanceBetween(*entity->target);
					if(distance_between < current_distance) {
						entity->target = other;
					}
				}
			}
		}

        if(team == Team::Player) {
            if(old_target != entity->target) {
				// spread aggro
                for(const auto &comrade: entities) {
					if(comrade->isDead() || comrade->target != nullptr) continue;
					const float distance_to_comrade = entity->getDistanceBetween(*comrade);
					if(distance_to_comrade <= entity->getAggroSpreadRadius()) {
						comrade->target = entity->target;
					}
				}
            }
        } else if(team == Team::Enemy) {
            if(entity->target == nullptr) {
				// ask comrades for aggro
                float distance_for_current_aggro = INFINITY;
				for(auto &comrade: entities) {
					if(comrade.get() == &*entity || comrade->isDead() || comrade->target == nullptr) continue;
					const float distance_to_comrade = entity->getDistanceBetween(*comrade);
					if(distance_to_comrade < distance_for_current_aggro) {
						entity->target = comrade->target;
						distance_for_current_aggro = distance_to_comrade;
					}
				}
            }
        }

        if(entity->target == nullptr) {
            if(shouldMove()) {
                entity->walkTowards(walk_towards, dt);
            } else {
                entity->progressWobble(Entity::StandingWobbleAmplitude, Entity::StandingWobbleSpeed, dt);
            }
        } else {
            const float distance_to_target = entity->getDistanceBetween(*entity->target);
            if(distance_to_target > entity->getPrefferedAttackRadius()) {
                entity->walkTowards(entity->target->getOrigin(), dt);
            } else {
                const sf::Vector2f delta = entity->target->getOrigin() - entity->getOrigin();
                const Entity::Direction new_direction = delta.x < 0.0f ? Entity::Direction::Left : Entity::Direction::Right;
                entity->setDirection(new_direction);
            }
            if(distance_to_target < entity->getAttackRadius()) {
                entity->attack_counter += dt;
                if(entity->attack_counter > entity->getAttackSpeed()) {
                    entity->attack();

                    const Troll *troll = dynamic_cast<Troll*>(&*entity);
                    if(troll != nullptr) {
                        game_map->craters.push_back(Crater(troll->getOrigin() + sf::Vector2f(0, troll->radius)));
                    }

                    if(entity->aoe_damage != 0 || entity->aoe_knockback != 0) {
                        auto aoe_iterator = game_map->iterateChunksInRadius(entity->getOrigin(), entity->getAttackRadius());
                        std::vector<Entity*> entities_hit;
                        while(auto chunk = aoe_iterator.next()) {
                            for(auto &other: *chunk) {
                                if(other->isDead() || other->team == entity->team) continue;
                                float distance_to_other = entity->getDistanceBetween(*other);
                                if(distance_to_other > entity->getAttackRadius()) continue;

                                bool is_in = std::any_of(entities_hit.begin(), entities_hit.end(), 
                                    [other](const Entity *entity_hit) { return entity_hit == other; });
                                
                                if(!is_in) {
                                    other->takeDamage(entity->aoe_damage);
                                    other->takeKnockback(entity->getOrigin(), entity->aoe_knockback);
                                    entities_hit.push_back(other);
                                }
                            }
                        }
                    }

                    entity->setTexture(entity->textures.getAttack(team));
                    entity->attack_counter = 0.0f;
                    entity->time_since_attacked = 0.0f;
                } else if(entity->attack_counter > entity->getAttackSpeed() - entity->getAttackPrepareDuration()) {
                    entity->setTexture(entity->textures.getPrepareAttack(team));
                }
            }
        }
    }

    if(team == Team::Enemy) {
		if(!grave_position.has_value()) {
			bool all_are_dead = std::all_of(
				entities.begin(), entities.end(), 
				[](const auto &entity) { return entity->isDead(); }
			);
			if(all_are_dead) {
				grave_position = getAveragePosition();
			}
		}

        bool all_are_free_to_go = std::all_of(
            entities.begin(), entities.end(), 
            [](const auto &entity){ return entity->target == nullptr; }
        );

        if(all_are_free_to_go) {
            updateWondering(dt);
        }
    }
}

void Gang::removeDeadTroops() {
    for(unsigned i = 0; i < entities.size();) {
        if(entities[i]->isDead()) {
            if(!removeFromVector(&game_map->all_entities_cache, &*entities[i])) {
                throw InvarianceException("Was not able to remove entity from cache!");
            }
            std::swap(entities[i], entities[entities.size() - 1]);
            entities.pop_back();
        } else {
            i++;
        }
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

std::array<sf::Rect<float>, 4> GameMap::getSpawnRects() const {
    const sf::Rect<float> arena = {outer_arena.getPosition(), outer_arena.getSize()};
    std::array<sf::Rect<float>, 4> spawnable_points {{
        { // up
            {arena.position.x, arena.position.y},
            {arena.size.x - OutsideSpawnWidth, OutsideSpawnWidth}
        },
        { // left
            {arena.position.x, arena.position.y + OutsideSpawnWidth},
            {OutsideSpawnWidth, arena.size.y - OutsideSpawnWidth}
        },
        { // down
            {arena.position.x + OutsideSpawnWidth, arena.position.y + arena.size.y - OutsideSpawnWidth},
            {arena.size.x - OutsideSpawnWidth, OutsideSpawnWidth}
        },
        { // right
            {arena.position.x + arena.size.x - OutsideSpawnWidth, arena.position.y},
            {OutsideSpawnWidth, arena.size.y - OutsideSpawnWidth}
        },
    }};
    return spawnable_points;
}

sf::Vector2u GameMap::getIndex(const sf::Vector2f position) const {
    const sf::Vector2f middle_to_center = chunk_size.componentWiseMul(sf::Vector2f(chunk_amounts)) / 2.0f;
    const sf::Vector2u absolute_position(position + middle_to_center);
    const sf::Vector2u index = sf::Vector2u(absolute_position.x / chunk_size.x, absolute_position.y / chunk_size.y);
    return index;
}

std::vector<Entity*> &GameMap::getChunk(const sf::Vector2u index) {
    if(index.x >= chunk_amounts.x || index.y >= chunk_amounts.y) throw "error";
    return map[index.x * chunk_amounts.y + index.y];
}

ChunkIterator GameMap::iterateChunksInRadius(const sf::Vector2f position, const float radius) {
    const sf::Vector2u chunk_index = getIndex(position);
    const sf::Vector2f rangef = sf::Vector2f(radius, radius).componentWiseDiv(chunk_size);
    const sf::Vector2u range(ceilf(rangef.x), ceilf(rangef.y));

    const sf::Vector2u start(
        std::min(subSat(chunk_index.x, range.x), chunk_amounts.x - 1), 
        std::min(subSat(chunk_index.y, range.y), chunk_amounts.y - 1)
    );
    const sf::Vector2u end(
        std::min(chunk_index.x + range.x, chunk_amounts.x - 1), 
        std::min(chunk_index.y + range.y, chunk_amounts.y - 1)
    );

    return ChunkIterator(*this, start, end);
}

void GameMap::addToChunks(Entity &entity) {
    auto iterator = iterateChunksInRadius(entity.getOrigin(), entity.getRadius());
    while(auto chunk = iterator.next()) {
        chunk->push_back(&entity);
    }
}

void GameMap::reset() {
    for(auto chunk: map) chunk.clear();
    all_entities_cache.clear();
    projectiles.clear();
}

void GameMap::updateProjectiles(const float dt) {
    for(unsigned i = 0; i < craters.size();) {
        craters[i].time_since += dt;

        if(craters[i].time_since > CraterPermanence) {
            craters[i] = craters.back();
            craters.pop_back();
        } else {
            i++;
        }
    }

    for(auto entity: all_entities_cache) {
        projectiles.insert(
            projectiles.end(),
            std::make_move_iterator(entity->projectiles.begin()),
            std::make_move_iterator(entity->projectiles.end())
        );
        entity->projectiles.clear();
    }

    for(unsigned i = 0; i < projectiles.size();) {
        sf::Vector2 delta = projectiles[i]->velocity * dt;
        sf::Vector2f old_position = projectiles[i]->getOrigin();
        projectiles[i]->sprite.move(delta);
        sf::Vector2f new_position = projectiles[i]->getOrigin();

        float spine_width = projectiles[i]->sprite.getTexture().getSize().x / 2.0f;
        sf::Vector2f spine_normalised = delta == sf::Vector2f() ? sf::Vector2f(1.0f, sf::radians(rand())) : delta.normalized();
        sf::Vector2f spine_rotated = { -spine_normalised.y, spine_normalised.x };

        sf::Vector2u top_left = getIndex({std::min(old_position.x, new_position.x), std::min(old_position.y, new_position.y)});
        sf::Vector2u bottom_right = getIndex({std::max(old_position.x, new_position.x), std::max(old_position.y, new_position.y)});

        auto iterator = ChunkIterator(
            *this,
            {std::clamp(top_left.x, 0U, chunk_amounts.x - 1), std::clamp(top_left.y, 0U, chunk_amounts.y - 1)},
            {std::clamp(bottom_right.x, 0U, chunk_amounts.x - 1), std::clamp(bottom_right.y, 0U, chunk_amounts.y - 1)}
        );
        while(auto chunk = iterator.next()) {
            for(auto other: *chunk) {
                if(other->isDead() || projectiles[i]->last_hit_entity == other) continue;
                sf::Vector2f other_delta = other->getOrigin() - old_position;

                sf::Vector2f projected(other_delta.dot(spine_normalised), other_delta.dot(spine_rotated));

                sf::Vector2f closest_point(
                    std::clamp(projected.x, 0.0f, delta.length()),
                    std::clamp(projected.y, -spine_width, spine_width)
                );
                float distance_to_closest_point = (closest_point - projected).length();
                if(distance_to_closest_point <= other->radius) {
                    projectiles[i]->hitEntity(other);
                    projectiles[i]->last_hit_entity = other;
                }
            }
        }

        bool to_delete = projectiles[i]->to_delete 
        || !(getBoundry().contains(projectiles[i]->getOrigin())) 
        || projectiles[i]->velocity == sf::Vector2f();

        if(to_delete) {
            std::swap(projectiles[i], projectiles[projectiles.size() - 1]);
            projectiles.pop_back();
            continue;
        } else {
            i++; // here and not in for as to not skip when deleted
        }
    }
}

void GameMap::updateMovement() {
    struct CollisionPair {
        Entity* a;
        Entity* b;
    };
    std::vector<CollisionPair> pairs;

    for(auto &entity: all_entities_cache) {
        if(entity->isDead()) continue;
        auto iterator = iterateChunksInRadius(entity->getOrigin(), entity->getRadius());
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
            sf::Vector2f amount_to_move = entity->to_move + entity->knocked_back * PerTickKnockbackRatio;
            entity->sprite.move(amount_to_move / float(MaxCollisionIterations));
        }

        for(auto &[entity, other]: pairs) {
            const sf::Vector2f delta = entity->getOrigin() - other->getOrigin();
            const float maximum_distance = entity->radius + other->radius;
            const float to_push_length = maximum_distance - delta.length();
            if(to_push_length <= 0) continue;

            const auto angle = delta.length() == 0.0f ? sf::radians(float(rand())) : delta.angle();
            const sf::Vector2f to_push(to_push_length, angle);
            
            if(entity->getWeight() == 0 && other->getWeight() == 0) continue;
            if(entity->getWeight() == INFINITY && other->getWeight() == INFINITY) continue;

            float entity_weight_proportion;
            float other_weight_proportion;

            if(entity->getWeight() == INFINITY) {
                entity_weight_proportion = 0.0;
                other_weight_proportion = 1.0;
            } else if(other->getWeight() == INFINITY) {
                entity_weight_proportion = 0.0;
                other_weight_proportion = 1.0;
            } else {
                float weight_sum = entity->getWeight() + other->getWeight();
                entity_weight_proportion = other->getWeight() / weight_sum;
                other_weight_proportion = entity->getWeight() / weight_sum;
            }

            entity->sprite.move(to_push * entity_weight_proportion);
            other->sprite.move(-to_push * other_weight_proportion);
        }
    }

    for(auto &entity: all_entities_cache) {
        entity->to_move = {0.0f, 0.0f};
        entity->knocked_back -= entity->knocked_back * PerTickKnockbackRatio;
        if(entity->isDead()) continue;
        entity->sprite.setPosition(clampPoint(entity->sprite.getPosition(), getBoundry()));
        addToChunks(*entity);
    }
}

void GameMap::draw(sf::RenderWindow &window) {
    window.draw(outer_arena);

    for(Crater crater: craters) {
        sf::Sprite crater_sprite(Crater::crater_texture);
        float percent_visible = 1 - (crater.time_since / CraterPermanence);
        crater_sprite.setColor(sf::Color(0, 0, 0, 255 * percent_visible));
        crater_sprite.setPosition(crater.position - sf::Vector2f(Crater::crater_texture.getSize()) / 2.0f);
        window.draw(crater_sprite);
    }

    #ifndef NDEBUG
        for(auto rect: getSpawnRects()) {
            sf::RectangleShape rectshape(rect.size);
            rectshape.setPosition(rect.position);
            rectshape.setFillColor(sf::Color(255, 0, 0, 32));
            window.draw(rectshape);
        }
        sf::RectangleShape rectshape(inner_arena.size);
        rectshape.setPosition(inner_arena.position);
        rectshape.setFillColor(sf::Color(255, 255, 255, 16));
        window.draw(rectshape);
    #endif

    std::sort(all_entities_cache.begin(), all_entities_cache.end(), 
        [](const Entity *x, const Entity *y) {
            if(x->isDead() && !y->isDead()) return true;
            if(!x->isDead() && y->isDead()) return false;
            return x->sprite.getPosition().y < y->sprite.getPosition().y; 
        });
    for(const Entity *entity: all_entities_cache) {
        entity->draw(window);
    }
    for(const auto &projectile: projectiles) {
        window.draw(projectile->sprite);
    }
}