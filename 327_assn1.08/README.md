Assignment 1.08 - Loading Pokemon

This project loads in pokemon. When stepping onto tall grass aka : you have a 10% chance of encountering a pokemon which just displays its info for now. The PC also can pick from 3 random starter pokemon at the very start as well. NPCs also spawn in with their own pokemon, and can have up to 6. When in a battle with an NPC it reflects the number of pokemon they have, and their first pokemon's info.

New functions:
- `poke:` Generates a new pokemon and all of it's data/information.
- `get_level:` Calculate's a pokemon's level depending on distance from the origin.
- `io_pokemon:` Displays the pokemon encounter window when the player encounters a pokemon.
- `io_battle:` Not new, but I added in the NPC's # of pokemon and their main pokemon's info during a battle.
- `move_pc_dir:` Not new, but added in a 10% chance the PC will encounter a pokemon in tall grass.
- `game_loop:` Not new, but added in the starter pokemon selection screen.
- `new_char_other/new_swimmer/new_rival/new_hiker:` Not new, but added in generating pokemon for NPCs, with a chance to have up to 6 (60% to add another each time).

New data structures:
- `poke:` Class in the new pokemon.h and pokemon.cpp files that stores a pokemon's information.
- `character:` Not new but added a pokemon vector for characters.