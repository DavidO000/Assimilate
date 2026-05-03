#include <exception>

class LoadTextureException: public std::exception {
    std::string dir_name;

public:
    explicit LoadTextureException(std::string dir_name_): dir_name(dir_name_) {}
};

class InvarianceException: public std::exception {
public:
    const char* explination;

    explicit InvarianceException(const char* explination_): explination(explination_) {};
};

class UnknownTeam: public std::exception {
    Team team;

public:
    explicit UnknownTeam(Team team_): team(team_) {};
};
