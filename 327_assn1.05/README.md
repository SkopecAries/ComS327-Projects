Assignment 1.05 - User Interface

This project implement the ncurses library to add a colorful display and a user interface. As stated in the specs 7 or y moves the PC to the upper left, 8 or k up, 9 or u upper right, 6 or l right, 3 or n lower right, 2 or j down, 1 or b lower left, and 4 or h left. To enter a building you must be on it and press > and to exit press <. To wait a turn press 5 or space or '.'. Finally to open the trainer list press t and scroll up with the up arrow and down with the down arrow, and of course escape to escape. Once you want to quit press q.

New functions:
- `print_map:` Not a new function, but adjusted printing for ncurses and added colors.
- `battle:` A function for when a pc and npc initiate a battle sequence. For now it just says the player won and can escape using the escape key.
- `move_pc:` A function that controls the pc's movements. Takes an x and y value (and m integer incase a message needs displayed) and attempt to move the pc to that position if possible.
- `enter_building:` Executes once the pc enters a building. Pops up a window saying what build type they're in and you can exit with the < key.
- `trainer_list:` Shows a list of trainers and their distance from the pc. The user can scroll up/down the list using the up/down arrows if there's more npcs listed and they can escape with the escape key.
- `main:` Not a new funciton, but I added the whole user interface regarding the keys.

New data structures:
- `None!`
