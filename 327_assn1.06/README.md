Assignment 1.06 - Moving to Neighboring Maps and Porting to C++

This project ported my previous C code to C++. It also added back in the ability to move to neighboring maps via gates (n, s, e, w) or by flying which can be done by pressing f and entering in the coorisponding coordinates.

New functions:
- `new_map:` Not a new function, but slightly adjusted the end to initiate npcs for a new map and do pathfinding.
- `move_pc:` Not a new function, but adjusted so that when the pc is on a gate they'll move into a neighboring map. Their x/y positions are also updated so when they move to a new map they'll be on the opposite gate. (Ex: moving to a map using the north gate will spawn the PC on the new map's south gate) Also added pathfinding for whenever PC moves.
- `fly:` Once the user presses f this function executes, asks for an x and y coordinate (must be between -200 and 200), then changes to that coorisponding map. Also randomly spawns the PC onto a path in that new map.
- `trainer_list:` Shows a list of trainers and their distance from the pc. The user can scroll up/down the list using the up/down arrows if there's more npcs listed and they can escape with the escape key.
- `main:` Not a new funciton, but I added in the ability to press f and use the fly function. There's also a commented out print statement I used for debugging to track pc position and map position if it's needed or desired.

New data structures:
- `Character:` Added in a character class that has the positions of the character. The npc and pc classes now use inheritance based on this class and are updated as such.
