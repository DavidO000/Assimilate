#include "overlay.hpp"

Overlay::Overlay(sf::Sprite sign_, sf::Color background_color_, OverlayOrder order_): 
    sign(std::move(sign_)), background_color(background_color_), order(order_), since(INFINITY) {}

bool Overlay::isIn() {
    return order == OverlayOrder::Coming;
}

void Overlay::set() {
    since = SecondsToPutOverlay - std::clamp(since, 0.0f, SecondsToPutOverlay); 
    order = OverlayOrder::Coming;
}

void Overlay::remove() {
    since = SecondsToPutOverlay - std::clamp(since, 0.0f, SecondsToPutOverlay); 
    order = OverlayOrder::Going;
}