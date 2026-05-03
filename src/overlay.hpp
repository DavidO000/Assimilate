#include <SFML/Graphics.hpp>

static constexpr float SecondsToPutOverlay = 0.05f;

enum class OverlayOrder { 
    Coming, 
    Going
};

class Overlay {
public:
    sf::Sprite sign;
    const sf::Color background_color;
    OverlayOrder order;
    float since;

    Overlay(sf::Sprite sign_, sf::Color background_color_, OverlayOrder order_);
    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;
    Overlay(Overlay&&) = delete;
    Overlay& operator=(Overlay&&) = delete;

    bool isIn();
    void set();
    void remove();
};