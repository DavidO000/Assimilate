#include "entity.cpp"

class Fireball: public Projectile {
    static const sf::Texture texture;

public:
    explicit Fireball(Entity &entity): Projectile(texture, entity, 500.0f) {}

    void hitEntity(Entity *hit) override {
        bool is_same_team = team == hit->getTeam();
        if(!is_same_team) {
            hit->takeDamage(20);
            to_delete = false;
        }
    }
};
const sf::Texture Fireball::texture = getTexture("assets/mage/fireball.png");



class Grunt: public Entity {
    static const EntityTextures textures;

public:
    explicit Grunt(const EntityTextures &textures_ = textures): Entity(textures_) {}
    
    float getRadius() const override { return 16.0f; }
    float getSpeed() const override { return 300.0f; }
    float getWeight() const override { return 1.0f; }
    unsigned getInitialHealth() const override { return 100; }
    float getAggroRadius() const override { return 16.0f; }
    float getLoseAggroRadius() const override { return 250.0f; }
    float getAggroSpreadRadius() const override { return 100.0f; }
    float getAttackRadius() const override { return 32.0f; }
    float getPrefferedAttackRadius() const override { return 24.0f; }
    float getAttackSpeed() const override { return 0.8f; }
    float getAttackPrepareDuration() const override { return 0.15f; }
    float getAttackDuration() const override { return 0.25f; }
    void attack() override {
        getTarget()->takeDamage(30);
    }
};
const EntityTextures Grunt::textures = EntityTextures("assets/grunt/");



class Knight: public Grunt {
    static const EntityTextures textures;

public:
    explicit Knight(const EntityTextures &textures_ = textures): Grunt(textures_) {}
    
    float getRadius() const override { return 25.0f; }
    float getSpeed() const override { return 250.0f; }
    float getWeight() const override { return 4.0f; }
    unsigned getInitialHealth() const override { return 400; }
    float getAggroRadius() const override { return 25.0f; }
    // float getLoseAggroRadius() const override { return 250.0f; }
    // float getAggroSpreadRadius() const override { return 100.0f; }
    float getAttackRadius() const override { return 35.0f; }
    float getPrefferedAttackRadius() const override { return 25.0f; }
    float getAttackSpeed() const override { return 1.5f; }
    float getAttackPrepareDuration() const override { return 0.35f; }
    float getAttackDuration() const override { return 0.35f; }
    void attack() override {
        getTarget()->takeDamage(120);
    }
};
const EntityTextures Knight::textures = EntityTextures("assets/knight/");

class Mage: public Knight {
    static const EntityTextures textures;

public:
    explicit Mage(const EntityTextures &textures_ = textures): Knight(textures_) {}
    
    // float getRadius() const override { return 25.0f; }
    // float getSpeed() const override { return 350.0f; }
    // float getWeight() const override { return 5.0f; }
    unsigned getInitialHealth() const override { return 500; }
    float getAggroRadius() const override { return 100.0f; }
    float getLoseAggroRadius() const override { return 400.0f; }
    float getAggroSpreadRadius() const override { return 100.0f; }
    float getAttackRadius() const override { return 350.0f; }
    float getPrefferedAttackRadius() const override { return 350.0f; }
    float getAttackSpeed() const override { return 1.7f; }
    float getAttackPrepareDuration() const override { return 0.5f; }
    float getAttackDuration() const override { return 0.5f; }
    void attack() override {
        throwProjectile(std::make_unique<Fireball>(*this));
    }
};
const EntityTextures Mage::textures = EntityTextures("assets/mage/");


class Samurai: public Knight {
    static const EntityTextures textures;

public:
    explicit Samurai(const EntityTextures &textures_ = textures): Knight(textures_) {}
    
    // float getRadius() const override { return 25.0f; }
    float getSpeed() const override { return 150.0f; }
    float getWeight() const override { return 10.0f; }
    unsigned getInitialHealth() const override { return 800; }
    float getAggroRadius() const override { return 200.0f; }
    float getLoseAggroRadius() const override { return 350.0f; }
    // float getAggroSpreadRadius() const override { return 100.0f; }
    float getAttackRadius() const override { return 100.0f; }
    float getPrefferedAttackRadius() const override { return 200.0f; }
    float getAttackSpeed() const override { return 0.375f; }
    float getAttackPrepareDuration() const override { return 0.125f; }
    float getAttackDuration() const override { return 0.125f; }
    void attack() override {
        getTarget()->takeDamage(45);
        getTarget()->takeKnockback(getOrigin(), -10.0);
    }
};
const EntityTextures Samurai::textures = EntityTextures("assets/samurai/");



class Minitroll: public Knight {
    static const EntityTextures textures;

public:
    explicit Minitroll(const EntityTextures &textures_ = textures): Knight(textures_) {}
    
    float getRadius() const override { return 35.0f; }
    float getSpeed() const override { return 200.0f; }
    float getWeight() const override { return 25.0f; }
    unsigned getInitialHealth() const override { return 2250; }
    // float getAggroRadius() const override { return 25.0f; }
    // float getLoseAggroRadius() const override { return 250.0f; }
    // float getAggroSpreadRadius() const override { return 100.0f; }
    // float getAttackRadius() const override { return 35.0f; }
    // float getPrefferedAttackRadius() const override { return 25.0f; }
    float getAttackSpeed() const override { return 1.7f; }
    float getAttackPrepareDuration() const override { return 0.375f; }
    float getAttackDuration() const override { return 0.375f; }
    void attack() override {
        setAOEDamage(175);
        setAOEKnockback(100.0);
    }
};
const EntityTextures Minitroll::textures = EntityTextures("assets/minitroll/");



class Troll: public Minitroll {
    static const EntityTextures textures;

public:
    explicit Troll(const EntityTextures &textures_ = textures): Minitroll(textures_) {}
    
    float getRadius() const override { return 85.0f; }
    float getSpeed() const override { return 150.0f; }
    float getWeight() const override { return 100.0f; }
    unsigned getInitialHealth() const override { return 10000; }
    // float getAggroRadius() const override { return 25.0f; }
    // float getLoseAggroRadius() const override { return 250.0f; }
    // float getAggroSpreadRadius() const override { return 100.0f; }
    // float getAttackRadius() const override { return 35.0f; }
    // float getPrefferedAttackRadius() const override { return 25.0f; }
    float getAttackSpeed() const override { return 2.5f; }
    float getAttackPrepareDuration() const override { return 0.375f; }
    float getAttackDuration() const override { return 0.25f; }
    void attack() override {
        setAOEDamage(250);
        setAOEKnockback(150.0);
    }
};
const EntityTextures Troll::textures = EntityTextures("assets/troll/");