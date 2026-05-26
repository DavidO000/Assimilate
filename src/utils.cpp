#include <iostream>
#include <random>
#include <algorithm>
#include <math.h>

#include <SFML/Graphics.hpp>

// static std::random_device rd;
// static std::mt19937 gen(rd());

template<typename T> T subSat(const T x, const T y) {
    return y > x ? 0 : x - y;
}

template<typename T> sf::Vector2<T> clampPoint(const sf::Vector2<T> point, const sf::Rect<T> rectangle) {
    const auto top_left_corner = rectangle.position;
    const auto bottom_right_corner = top_left_corner + rectangle.size;
    return {
        std::clamp(point.x, top_left_corner.x, bottom_right_corner.x),
        std::clamp(point.y, top_left_corner.y, bottom_right_corner.y)
    };
}

template<typename T> sf::Vector2<T> getRandomRectPosition(const sf::Rect<T> rect) {
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
[[maybe_unused]] void debug_fmt()  {}



// cppcheck-suppress unusedFunction
template<typename T> [[maybe_unused]] T debug_defer_fmt(const char file[], int line, const char str[], T arg) {
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

// cppcheck-suppress unusedFunction
template<typename T>
class DebugReport {
    char *file;
    std::optional<int> line;
    std::optional<T> copied_object;

public:
    DebugReport(): file(nullptr), line(), copied_object() {}
    
    DebugReport &with_file(const char *p_file) { file = const_cast<char*>(p_file); return *this; }
    DebugReport &with_line(int p_line) { line = p_line; return *this; }
    DebugReport &with_object(T p_object) { copied_object = p_object; return *this; }

    friend std::ostream& operator<<(std::ostream& out, const DebugReport &debug_report) {
        if(debug_report.file == nullptr && !debug_report.line && !debug_report.copied_object) {
            out << "Empty debug report.";
        } else {
            if(debug_report.file != nullptr) {
                out << "On file " << debug_report.file << " ";
            }
            if(debug_report.line) {
                out << "On line " << *debug_report.line << " ";
            }
            if(debug_report.copied_object) {
                out << "We have " << *debug_report.copied_object << ".";
            }
        }
        return out;
    }
};

#define DBGReport(x) DebugReport<x>().with_file(&__FILE__[0]).with_line(__LINE__) 