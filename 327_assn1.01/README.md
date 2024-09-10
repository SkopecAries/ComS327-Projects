Assignment 1.01 - Terrain Generation

This project is a procedural map generator designed to create maps reminiscent of the Pokémon game series. It features various terrain types, including water bodies, tall grass, clearings, paths, and places important structures like Pokémon Centers and Poké Marts.

Key functions:
- `border()`: Creates a boulder border around the map.
- `clearing()`: Initializes the map with grass.
- `water()`: Randomly generates water bodies.
- `tallGrass()`: Places patches of tall grass.
- `sprinkle(char c)`: Randomly "sprinkles" in trees/rocks.
- `path()`: Creates intersecting paths.
- `validBuild()`: Determines valid locations for buildings.
- `placeBuilds()`: Places Pokémon Centers and Poké Marts.
- `createMap()`: Integrates all map creation functions.
- `printMap()`: Displays the generated map.

Data structures:
- `map[][]`: A 2D char array representing the map.