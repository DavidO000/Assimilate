#include <SFML/Graphics.hpp>
#include <iostream>
#include <random>

static std::random_device rd;
static std::mt19937 gen(rd());

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
[[maybe_unused]] sf::RectangleShape debug_fmt(const sf::RectangleShape rect)  {
    std::cout << rect.getPosition().x << " " << rect.getPosition().y << ", " 
            << rect.getSize().x << " " << rect.getSize().y;
    return rect;
};

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] sf::Rect<T> debug_fmt(const sf::Rect<T> rect)  {
    std::cout << rect.position.x << " " << rect.position.y << ", " << rect.size.x << " " << rect.size.y;
    return rect;
};

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] sf::Vector2<T> debug_fmt(const sf::Vector2<T> vec)  {
    std::cout << vec.x << " " << vec.yW;
    return vec;
};

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] std::vector<T> debug_fmt(const std::vector<T> &vec)  {
    int i = 1;
    std::cout << "{";
    for(const T &element: vec) {
        debug_fmt(element);
        if(i != vec.size()) std::cout << ", ";
        i++;
    }
    std::cout << "}";
    return vec;
};

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] T debug_fmt(const T &x)  {
    std::cout << x;
    return x;
};

// cppcheck-suppress unusedFunction
[[maybe_unused]] void debug_fmt()  {
}

// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]]
T debug_defer_fmt(const char file[], int line, const char str[], T arg) {
    std::cerr << "Debug: " << file << ":" << line << " [" << str << "] = ";
    T value = debug_fmt(arg);
    std::cerr << std::endl;
    return value;
}

// cppcheck-suppress unusedFunction
[[maybe_unused]] void debug_defer_fmt(const char file[], int line) {
    std::cerr << "Debug: " << file << ":" << line << std::endl;
}

#define DBG(...) debug_defer_fmt(__FILE__, __LINE__, #__VA_ARGS__, __VA_ARGS__)
#define DBGE(...) debug_defer_fmt(__FILE__, __LINE__)