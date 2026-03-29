#include <SFML/Graphics.hpp>

static constexpr float SecondsToPutOverlay = 0.05f;
enum class OverlayOrder { Coming, Going };
class Overlay {
public:
    sf::Sprite sign;
    const sf::Color background_color;
    OverlayOrder order;
    float since;

    Overlay(sf::Sprite sign_, sf::Color background_color_, OverlayOrder order_, float since_): 
        sign(std::move(sign_)), background_color(background_color_), order(order_), since(since_) {}
    Overlay(const Overlay&) = delete;
    Overlay& operator=(const Overlay&) = delete;
    Overlay(Overlay&&) = delete;
    Overlay& operator=(Overlay&&) = delete;

    bool isIn() {
        return order == OverlayOrder::Coming;
    }

    void set() {
        since = SecondsToPutOverlay - std::clamp(since, 0.0f, SecondsToPutOverlay); 
        order = OverlayOrder::Coming;
    }

    void remove() {
        since = SecondsToPutOverlay - std::clamp(since, 0.0f, SecondsToPutOverlay); 
        order = OverlayOrder::Going;
    }
};