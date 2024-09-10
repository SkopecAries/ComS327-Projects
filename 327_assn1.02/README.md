Assignment 1.02 - Moving from map to map

This project allows a player to move from map to map, and generates new ones as needed. The key inputs are n to go north, s to go south, e to go east, w to go west, and f x y (for x, y coordinates) to fly to a specific map in the world. The main changes from the previous program was adding in these movement abilities, adjusting path gates so they properly align, and adjusting how often the pokemon builds appear depending on a distance formula.

New functions:
- `initializeMap(Map *map)`: Helps with initializing the map.
- `moveSouth(int *x, int *y)`: Moves the player south.
- `moveNorth(int *x, int *y)`: Moves the player north.
- `moveWest(int *x, int *y)`: Moves the player west.
- `moveEast(int *x, int *y)`: Moves the player east.
- `fly(int *x, int *y)`: Flies a player to a specified x, y location.
- `gameLoop()`: A loop that handles user input and structures the actual game.

New data structures:
- `Map struct`: A structure for the independent maps to keep important variable data.

note: For the edge of the world maps there are no gates where they shouldn't be but the paths may go towards a particular direction making it seem like there is one. (I replaced the gates with boulders so there are none)