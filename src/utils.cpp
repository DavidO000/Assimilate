#include <SFML/Graphics.hpp>
#include <iostream>

template<typename T> T subSat(const T x, const T y) {
    return y > x ? 0 : x - y;
}

sf::Vector2f clampPoint(const sf::Vector2f point, const sf::Rect<float> rectangle) {
    const auto top_left_corner = rectangle.position;
    const auto bottom_right_corner = top_left_corner + rectangle.size;
    return {
        std::clamp(point.x, top_left_corner.x, bottom_right_corner.x),
        std::clamp(point.y, top_left_corner.y, bottom_right_corner.y)
    };
}

sf::Vector2f getRandomRectPosition(const sf::Rect<float> rect) {
    return {
        rand() % int(rect.size.x) + rect.position.x,
        rand() % int(rect.size.y) + rect.position.y
    };
}

template<typename T> bool removeFromVector(std::vector<T> *vector, const T &element) {
    for(unsigned i = 0; i < vector->size(); i++) {
        if(vector->at(i) == element) {
            std::swap(vector->at(i), vector->at(vector->size() - 1));
            vector->pop_back();
            return true;
        }
    }
    return false;
}

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] sf::Rect<T> debug(const sf::Rect<T> rect)  {
    std::cout << "debug: " << rect.position.x << " " << rect.position.y << ", " << rect.size.x << " " << rect.size.y << std::endl;
    return rect;
};

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] sf::Vector2<T> debug(const sf::Vector2<T> vec)  {
    std::cout << "debug: " << vec.x << " " << vec.y << std::endl;
    return vec;
};

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] T debug(const T &x)  {
    std::cout << "debug: " << x << std::endl;
    return x;
};