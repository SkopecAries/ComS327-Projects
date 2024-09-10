Assignment 1.04 - Player Character and Trainers

This project creates the NPCs/trainers, prints them on a map, and moves them around given the specs of the assignment. The default number of NPCs is 10, but if you want you can set it by using --numtrainers <number>. There is no hardcoded max so be aware using too much may cause unrelated issues. (As noted in the specs to exit do ctrl c)

New functions:
- `print_map:` Not a new function, but a lot added to it to allow NPCs to be printed without duplicating terrain.
- `npc_cmp:` Mainly used for heaps, returns npc key next move - npc with next move.
- `init_npc:` Initialized NPCs, making sure there's at least 1 hiker and 1 rival, then randomizing what the rest are. Also randomizes their initial location.
- `move_npc:` One function that controls the movements of all npc types. Uses switch to find the type, then moves depending on that type.
- `next:` Deals with taking out an npc from the heap, moving them, adding their cost, then putting them back in.
- `main:` Not a new funciton, but I added in a while loop that runs the next function, reprints the map, then after a few seconds clears and does it all again until the user exits with ctrl c.

New data structures:
- `npc struct:` A structure that stores npc data.
- `trainer_map:` Data within the map structure that stores the npcs on their own map.
- `trainer_heap:` Data within the map structure that stores the heap info for npcs.
