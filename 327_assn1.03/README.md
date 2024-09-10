Assignment 1.03 - Path Finding

This project creates distance maps for future rivals. These maps use Dijkstra's algorithm to find the closest, and quickest, way to get the the player character. In this program I decided to switch to Sheaffer's code to make the implementation of Dijkstra easier on myself.

New functions:
- `dijkstra_path_char:` Uses some of Sheaffer's dijkstra_path code to find and create the distance maps.
- `validPC:` Finds a valid location for the PC.
- `placePC:` Places the pc at a valid location.
- `new_map:` Not a new function but added in placing the pc character.
- `main:` Not new but added in using dijkstra_path_char to find distance maps.

New data structures:
- `movement_costs array:` stores the costs of the terrain depending on the character
