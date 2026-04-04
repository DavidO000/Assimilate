#include "entity.cpp"

class Fireball: public Projectile {
public:
    Fireball(const sf::Texture &texture, Entity &entity): 
        Projectile(texture, entity, 250.0f) {}

    void hitEntity(Entity *hit) override {
        bool is_same_team = team == hit->getTeam();
        if(!is_same_team) {
            // hit->takeKnockback(getOrigin(), 250.0f);
            hit->takeDamage(10);
            to_delete = true;
        }
    }
};

class Grunt: public Entity {
    const sf::Texture &fireball_texture;

public:
    explicit Grunt(const TextureHolder &texture_holder): 
        Entity(texture_holder.getGrunt()), fireball_texture(texture_holder.getFireball()) {}
    
    float getRadius() const override { return 25.0f; }
    float getSpeed() const override { return 500.0f; }
    unsigned getInitialHealth() const override { return 100; }
    // unsigned getMaximumHealth() const override { return 100; }
    float getAggroRadius() const override { return 100.0f; }
    float getLoseAggroRadius() const override { return 1000.0f; }
    float getAggroSpreadRadius() const override { return 100.0f; }
    float getAttackRadius() const override { return 250.0f; }
    float getPrefferedAttackRadius() const override { return 250.0f; }
    float getAttackSpeed() const override { return 1.0f; }
    float getAttackPrepareDuration() const override { return 0.25f; }
    float getAttackDuration() const override { return 0.25f; }
    void attack() override {
        auto fireball = std::make_unique<Fireball>(fireball_texture, *this);
        throwProjectile(std::move(fireball)); 
        // getTarget()->takeKnockback(getOrigin(), 250.0f);
        // getTarget()->takeDamage(30);
    }
};