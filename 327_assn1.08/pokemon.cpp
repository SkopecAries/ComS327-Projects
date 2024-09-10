#include <string>
#include <ncurses.h>
#include "pokemon.h"
#include "db_parse.h"
#include "Assignment1_08.h"

poke::poke() {
    int r_id = rand() % 1093;
    this->Pokemon = pokemon[r_id];
    this->level = get_level();

    int i = 0;
    while (species[i].id != Pokemon.species_id) {
        i++;
    }
    this->poke_species = species[i];
    this->name = poke_species.identifier;

    if (rand() % 8192 == 0) {
        is_shiny = 1;
    } else {
        is_shiny = 0;
    }

    if (rand() % 2 == 0) {
        gender = "female";
    } else {
        gender = "male";
    }

    i = 0;
    while (pokemon_stats[i].pokemon_id != Pokemon.id) {
        i++;
    }

    // finding hp
    int j = 0;
    while (pokemon_stats[i + j].stat_id != 1) {
        j++;
    }
    int base = pokemon_stats[i + j].base_stat;
    int iv = rand() % 16;
    this->hp = ((((base + iv) * 2) * level) / 100) + level + 10;

    // finding attack
    j = 0;
    while (pokemon_stats[i + j].stat_id != 2) {
        j++;
    }
    base = pokemon_stats[i + j].base_stat;
    iv = rand() % 16;
    this->attack = ((((base + iv) * 2) * level) / 100) + 5;

    // finding defense
    j = 0;
    while (pokemon_stats[i + j].stat_id != 3) {
        j++;
    }
    base = pokemon_stats[i + j].base_stat;
    iv = rand() % 16;
    this->defense = ((((base + iv) * 2) * level) / 100) + 5;

    // finding special-attack
    j = 0;
    while (pokemon_stats[i + j].stat_id != 4) {
        j++;
    }
    base = pokemon_stats[i + j].base_stat;
    iv = rand() % 16;
    this->special_attack = ((((base + iv) * 2) * level) / 100) + 5;

    // finding special-defense
    j = 0;
    while (pokemon_stats[i + j].stat_id != 5) {
        j++;
    }
    base = pokemon_stats[i + j].base_stat;
    iv = rand() % 16;
    this->special_defense = ((((base + iv) * 2) * level) / 100) + 5;

    // finding speed
    j = 0;
    while (pokemon_stats[i + j].stat_id != 6) {
        j++;
    }
    base = pokemon_stats[i + j].base_stat;
    iv = rand() % 16;
    this->speed = ((((base + iv) * 2) * level) / 100) + 5;

    // finding moves
    i = 0;
    int num_moves = 0;
    while (pokemon_moves[i].pokemon_id != Pokemon.species_id) {
        i++;
    }
    j = 0;
    while (pokemon_moves[i + j].pokemon_move_method_id != 1 || pokemon_moves[i + j].level > this->level) {
        j++;
        // if it doesn't find a corresponding move within it's species increase the level
        if (pokemon_moves[i + j].pokemon_id != Pokemon.species_id) {
            j = 0;
            level++;
        }
    }
    this->poke_moves[num_moves] = pokemon_moves[i + j];

    j = 0;
    while(moves[j].id != poke_moves[0].move_id) {
        j++;
    }
    this->move_name[num_moves] = moves[j].identifier;
    num_moves++;
    move_num = 1;

    // now checking for a second move
    while (i < 528239 && num_moves < 2) {
        while (i < 528239 && pokemon_moves[i].pokemon_id != Pokemon.species_id) {
            i++;
        }
        if (i >= 528239) break;

        if ((pokemon_moves[i].pokemon_move_method_id == 1) && (pokemon_moves[i].level <= this->level)) {
            // Ensure it's not the same as the first move
            if ((num_moves > 0 && pokemon_moves[i].move_id != poke_moves[0].move_id)) {
                this->poke_moves[num_moves] = pokemon_moves[i];

                j = 0;
                while (j < 845 && moves[j].id != pokemon_moves[i].move_id) {
                    j++;
                }
                if (j < 845) {
                    this->move_name[num_moves] = moves[j].identifier;
                    num_moves++;
                    move_num = 2;
                }
            }
        }
        i++;
    }
};

int poke::get_level() {
    int man_dist = (abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)) + abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)));
    int minimum, maximum;

    if (man_dist <= 200) {
        minimum = 1;
        maximum = man_dist / 2;
        if (maximum < 1) {
            maximum = 1;
        }
    } else {
        minimum = (man_dist - 200) / 2;
        maximum = 100;
        if (minimum < 1) {
            minimum = 1;
        }
    }

    return (rand() % (maximum - minimum + 1)) + minimum;
};