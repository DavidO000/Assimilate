#include "entity.cpp"

class Grunt: public Entity {
public:
    Grunt(const EntityBuilder &entity_builder): Entity(entity_builder.getGrunt()) {}
    
    float getRadius() const override { return 25.0f; }
    float getSpeed() const override { return 250.0f; }
    unsigned getMaximumHealth() const override { return 100; }
    unsigned getInitialHealth() const override { return 100; }
    float getAggroRadius() const override { return 100.0f; }
    float getLoseAggroRadius() const override { return 1000.0f; }
    float getAggroSpreadRadius() const override { return 100.0f; }
    float getAttackRadius() const override { return 150.0f; }
    float getPrefferedAttackRadius() const override { return 50.0f; }
    float getAttackSpeed() const override { return 1.0f; }
    void attack() override {
        getTarget()->takeKnockback(getOrigin(), 10.0f);
        getTarget()->takeDamage(30);
    }
};