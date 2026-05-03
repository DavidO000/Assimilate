#include "utils.cpp"

enum class Team {
    Player, 
    Enemy,
};

bool isEnemyTeam(Team lhs, Team rhs);
std::ostream& operator<<(std::ostream& out, const Team &team);