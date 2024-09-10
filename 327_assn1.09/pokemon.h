#ifndef POKEMON_H
# define POKEMON_H

#include <string>
#include "db_parse.h"

class poke {
    public:
        poke();

        std::string name;
        pokemon_db Pokemon;
        pokemon_species_db poke_species;
        int hp;
        int max_hp;
        int level;
        int attack;
        int defense;
        int speed;
        int special_attack;
        int special_defense;
        int is_shiny;
        std::string gender;
        pokemon_move_db poke_moves[2];
        std::string move_name[2];
        int move_num;

        int get_level();
};

#endif