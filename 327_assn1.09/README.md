Assignment 1.09 - Pokemon Battles

This project implements battle sequences between wild pokemon and trainers. When entering a battle sequence, if with a trainer you can fight with 1, enter bag with 2, or swap pokemon with 3. With a wild pokemon you have the same options except you can run with 3 instead, and swapping pokemon is 4. In the fight sequence, you can pick the move you'd like and fight with the enemy. In the bag you can select items to use (revive, potion, pokeball), keep in mind pokeballs can ONLY be used in a wild pokemon battle AND when they're ready to be caught. If you wish to try and run away from a wild pokemon you can press run. Lastly, if you wish to swap pokemon enter pokemon where you'll get a list of your pokemon and you can pick the one to swap to. (Note: in io_fight, io_battle, and io_poke there is a commented out ESCAPE variable and ESCAPE case if it's easier to use when grading)

New functions:
- `io_list_pokemon:` Shows a list of the PC's pokemon.
- `io_moves:` Asks the user to pick what move to use when fighting. A lot of the calulations for fighting is done in here.
- `io_fight:` Initiates a fighting sequence.
- `io_bag:` Opens up the bag to use items. Most of the time wild is set to false other than if in a wild pokemon battle AND that pokemon has fainted (ready to be caught). Other than that pokeballs CANT be used.
- `io_swap_pokemon:` Opens a menu to swap your current pokemon with any other pokemon. If a pokemon has fainted they can't be swapped.
- `io_battle:` Not new, but adjusted a lot for the battle intro, and all of the battle options for trainer battles.
- `io_pokemon:` Not new, but adjusted a lot for the battle intro, and all of the battle options for wild pokemon battles.
- `io_handle_input:` Not new, but added in a B case to open up the bag and use items outside of battles.
- `init_pc:` Not new, but added in the default item amounts for the PC to start with.
- `io_pokemart:` Not new, but made it so all items are refilled when entering.
- `io_pokemon_center:` Not new, but made it so all pokemon get fully revived when entering.

New data structures:
- `poke:` Not new, but added in a max_hp integer.
- `pc:` Not new, but added in the three items into the pc class.
