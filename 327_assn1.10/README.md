Assignment 1.10 - Cake Decorator

This project we were able to pick whatever we wanted to do, as long as it was about as complex as our previous assignments. I decided to make a cake decorator game, where the user starts out by picking how many cake layers they want, their cake flavor, then they sift through decoration types and decorate their cake by the layer! When they're done they hit 'd' and pick their topper, then they can see their final cake result displayed. I picked this because it seemed really fun and a lot more my style. I also wanted to do something that was more on the creativity side, and could be played multiple times with different experiences.

New functions:
- `init_ncurses:` Sets up ncurses environment for the game.
- `pick_layers:` Handles input for choosing the number of cake layers (1-6).
- `pick_flavor:` Allows the user to pick the cake flavor which changes the cake's color scheme.
- `pick_topper:` Enables selection of a cake topper from provided topper options.
- `display_final_cake:` Displays the fully decorated cake with it's chosen topper on top of a nice clean plate.
- `createLayers:` Initialized cake layers depending on what the user picked.
- `addDecoration:` Applies a chosen decoration to a specified layer of the cake.
- `display:` Displays the cake.
- `getLayerCount:` Returns the amount of layers.
- `main:` Executes the game and full game loop.

New data structures:
- `cake:` Class that manages cake layers and decorations. Has a private vector for storing storing layer designs, and public methods include creating layers, adding decorations, and displaying the cake.
- `decoration:` Struct used to store information about decorations. Contains a string for the name, and a wchar_t for the symbol.
