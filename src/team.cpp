#include "team.hpp"

bool isEnemyTeam(Team lhs, Team rhs) {
    return lhs != rhs;
}

std::ostream& operator<<(std::ostream& out, const Team &team) {
    out << (team == Team::Player ? "Player" : "Enemy");
    return out;
}