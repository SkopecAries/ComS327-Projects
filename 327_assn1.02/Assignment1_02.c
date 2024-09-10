#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <string.h>
#include <stdbool.h>

#define M_WIDTH 80
#define M_HEIGHT 21
#define WORLD_SIZE 401
#define CENTER 200

#define BOULDER '%'
#define PATH '#'
#define TALL_GRASS ':'
#define GRASS '.'
#define WATER '~'
#define TREE '^'
#define POKEMON_CENTER 'C'
#define POKEMART 'M'

// array for the map
char map[M_HEIGHT][M_WIDTH];

typedef struct map
{
    int x, y;
    char **tiles; // 2D array for map tiles
    int northGate, southGate, eastGate, westGate;
    int hasBeenVisited;
} Map;

Map *world[WORLD_SIZE][WORLD_SIZE] = {{NULL}}; // set all to NULL

// initializes the map
void initializeMap(Map *map)
{
    map->tiles = malloc(M_HEIGHT * sizeof(char *));
    if (map->tiles == NULL) {
        // Handle memory allocation failure
        exit(1); // Or any appropriate error handling
    }
    for (int i = 0; i < M_HEIGHT; i++) {
        map->tiles[i] = malloc(M_WIDTH * sizeof(char));
        if (map->tiles[i] == NULL) {
            // Handle memory allocation failure
            // Free already allocated rows and the tiles array itself, then exit or handle error
            for (int j = 0; j < i; j++) {
                free(map->tiles[j]);
            }
            free(map->tiles);
            exit(1); // Or any appropriate error handling
        }
    }
}

// creates the boulder border for the map
void border(Map *map)
{
    for (int j = 0; j < M_WIDTH; j++)
    {
        map->tiles[0][j] = BOULDER;
        map->tiles[M_HEIGHT - 1][j] = BOULDER;
    }
    for (int i = 0; i < M_HEIGHT; i++)
    {
        map->tiles[i][0] = BOULDER;
        map->tiles[i][M_WIDTH - 1] = BOULDER;
    }
}

// creates a clearing
void clearing(Map *map)
{
    for (int i = 0; i < M_HEIGHT; i++)
    {
        for (int j = 0; j < M_WIDTH; j++)
        {
            map->tiles[i][j] = GRASS;
        }
    }
}

// creates water
void water(Map *map)
{
    int num = rand() % 3 + 2; // number of ponds in the map (1-4)

    // generates the ponds
    while (num != 0)
    {
        int pondSize = rand() % 13 + 3; // Random pond size between 3 and 15
        int x = rand() % (M_HEIGHT - pondSize - 2) + 1;
        int y = rand() % (M_WIDTH - pondSize - 2) + 1;

        for (int i = x; i < x + pondSize; i++)
        {
            for (int j = y; j < y + pondSize; j++)
            {
                if (map->tiles[i][j] == GRASS)
                {
                    map->tiles[i][j] = WATER;
                }
            }
        }
        num--;
    }
}

// creates tall grass
void tallGrass(Map *map)
{
    int num = rand() % 3 + 3; // amount of tall-grass patches (2-5)

    // generates the grass
    while (num != 0)
    {
        int size = rand() % 16 + 2; // Random tall-grass size between 2 and 18
        int x = rand() % (M_HEIGHT - size - 2) + 1;
        int y = rand() % (M_WIDTH - size - 2) + 1;

        for (int i = x; i < x + size; i++)
        {
            for (int j = y; j < y + size; j++)
            {
                if (map->tiles[i][j] == GRASS)
                {
                    map->tiles[i][j] = TALL_GRASS;
                }
            }
        }
        num--;
    }
}

// generates trees/rocks depending on the char input
void sprinkle(char c, Map *map)
{
    for (int i = 1; i < M_HEIGHT - 1; i++)
    {
        for (int j = 1; j < M_WIDTH - 1; j++)
        {
            if (map->tiles[i][j] == GRASS && (rand() % 50) == 0)
            { // 2% chance to place a tree/rock
                map->tiles[i][j] = c;
            }
        }
    }
}

// creates the intersecting paths
void path(Map *map)
{
    // check surrounding maps
    int mapX = map->x;
    int mapY = map->y;
    int x1, x2, y1, y2;
    // checks if surrounding maps are generated
    if (mapX < 400 || mapY < 400 || mapX > 0 || mapX > 0) {
        if (world[mapX][mapY + 1] != NULL) { // map north of this one
            // if it's generated find proper gates
            map->northGate = world[mapX][mapY + 1]->southGate;
            y1 = map->northGate;
        } else { // if it's NULL pick a random spot for the gate
            y1 = (rand() % (M_WIDTH - 2) + 1);
            map->northGate = y1;
        } if (world[mapX][mapY - 1] != NULL) { // south
            map->southGate = world[mapX][mapY - 1]->northGate;
            y2 = map->southGate;
        } else {
            y2 = (rand() % (M_WIDTH - 2) + 1);
            map->southGate = y2;
        } if (world[mapX + 1][mapY] != NULL) { // east
            map->eastGate = world[mapX + 1][mapY]->westGate;
            x2 = map->eastGate;
        } else {
            x2 = (rand() % (M_HEIGHT - 2) + 1);
            map->eastGate = x2;
        } if (world[mapX - 1][mapY] != NULL) { // west
            map->westGate = world[mapX - 1][mapY]->eastGate;
            x1 = map->westGate;
        } else {
            x1 = (rand() % (M_HEIGHT - 2) + 1);
            map->westGate = x1;
        }
    }
 
    map->tiles[0][y1] = PATH; // north gate
    map->tiles[M_HEIGHT - 1][y2] = PATH; // south gate
    map->tiles[x1][0] = PATH; // west gate
    map->tiles[x2][M_WIDTH - 1] = PATH;  // east gate


    // random x and y points for paths to go towards
    int x = (rand() % (M_HEIGHT - 3) + 1);
    int y = (rand() % (M_WIDTH - 3) + 1);

    // north to south path
    for (int i = 0; i < x; i++)
    {
        map->tiles[i][y1] = PATH;
    }
    int startY, endY;
    if (y1 < y2) {
        startY = y1;
        endY = y2;
    } else {
        startY = y2;
        endY = y1;
    }
    for (int j = startY; j <= endY; j++) {
        map->tiles[x][j] = PATH;
    }
    for (int i = x + 1; i < M_HEIGHT; i++) {
        map->tiles[i][y2] = PATH;
    }

    // west to east path
    for (int j = 1; j < y; j++)
    {
        map->tiles[x1][j] = PATH;
    }
    int startX, endX;
    if (x1 < x2) {
        startX = x1;
        endX = x2;
    } else {
        startX = x2;
        endX = x1;
    }
    for (int i = startX; i <= endX; i++) {
        map->tiles[i][y] = PATH;
    }
    for (int j = y + 1; j < M_WIDTH; j++) {
        map->tiles[x2][j] = PATH;
    }

    // for gates that shouldn't be there
    if (mapX == 400) {
        map->tiles[x2][M_WIDTH - 1] = BOULDER;
    } if (mapY == 400) {
        map->tiles[0][y1] = BOULDER;
    } if (mapX == 0) {
        map->tiles[x1][0] = BOULDER;
    } if (mapY == 0) {
        map->tiles[M_HEIGHT - 1][y2] = BOULDER;
    }
}

// puts valid locations for builds to go into an array
void validBuild(int *locations, int *size, Map *map)
{
    *size = 0;
    for (int i = 1; i < M_HEIGHT - 1; i++)
    {
        for (int j = 1; j < M_WIDTH - 1; j++)
        {
            // checks if a path is around
            if (map->tiles[i][j] == GRASS && (map->tiles[i - 1][j] == PATH || map->tiles[i + 1][j] == PATH || map->tiles[i][j - 1] == PATH || map->tiles[i][j + 1] == PATH))
            {
                locations[*size] = i * M_WIDTH + j;
                (*size)++;
            }
        }
    }
}

// places down the builds randomly next to the path and depending on the distance from center
void placeBuilds(Map *map, int mapX, int mapY) {
    int locations[M_HEIGHT * M_WIDTH];
    int size;
    validBuild(locations, &size, map);

    // ensure if it's in the center it has both (do what it did before)
    if (mapX == CENTER && mapY == CENTER) {
        // generates position for C
        int rand1 = rand() % size;
        int position = locations[rand1];
        map->tiles[position / M_WIDTH][position % M_WIDTH] = POKEMON_CENTER;
        locations[rand1] = locations[rand1 + 1];
        size--;

        // generates position for M
        rand1 = rand() % size;
        position = locations[rand1];
        map->tiles[position / M_WIDTH][position % M_WIDTH] = POKEMART;
        locations[rand1] = locations[rand1 + 1];
        size--;

        return;
    }

    // Calculate the Manhattan distance from the center
    int d = abs(mapX - CENTER) + abs(mapY - CENTER);
    double probability;

    // Formula from the assignment description
    if (d < 200) {
        probability = (-45.0 * d / 200.0 + 50.0);
    } else {
        probability = 0.05; // Flat 5% chance for distances 200+
    }

    // Decide whether to place a Pokemon Center
    double randPercent = rand() % 100;
    if (randPercent < probability) {
        int rand2 = rand() % size;
        int position = locations[rand2];
        map->tiles[position / M_WIDTH][position % M_WIDTH] = POKEMON_CENTER;
        locations[rand2] = locations[rand2 + 1];
        size--;
        // int randIndex = rand() % size;
        // int position = locations[randIndex];
        // map->tiles[position / M_WIDTH][position % M_WIDTH] = POKEMON_CENTER;

        // locations[randIndex] = locations[size - 1];
        // size--;
    }

    // Re-calculate random percent for Pokemart placement decision
    randPercent = rand() % 100;
    if (randPercent < probability) {
        int rand2 = rand() % size;
        int position = locations[rand2];
        map->tiles[position / M_WIDTH][position % M_WIDTH] = POKEMART;
        locations[rand2] = locations[rand2 + 1];
        size--;
        // int randIndex = rand() % size;
        // int position = locations[randIndex];
        // map->tiles[position / M_WIDTH][position % M_WIDTH] = POKEMART;
    }
}

// creates the char map within the map array
Map* createMap(int x, int y) {
    Map* newMap = malloc(sizeof(Map));
    if (newMap) {
        initializeMap(newMap);
        newMap->x = x;
        newMap->y = y;
        newMap->northGate = -1;
        newMap->southGate = -1;
        newMap->eastGate = -1;
        newMap->westGate = -1;
        newMap->hasBeenVisited = 1;
        clearing(newMap);
        water(newMap);
        tallGrass(newMap);
        border(newMap);
        path(newMap);
        placeBuilds(newMap, x, y);
        sprinkle(TREE, newMap);
        sprinkle(BOULDER, newMap);
    }
    return newMap;
}

// prints the map
void printMap(Map *map)
{
    putchar('\n');
    for (int i = 0; i < M_HEIGHT; i++)
    {
        for (int j = 0; j < M_WIDTH; j++)
        {
            putchar(map->tiles[i][j]);
        }
        putchar('\n');
    }
}

// moves the player south
void moveSouth(int *x, int *y) {
    // Decrease y to move south, but check for world bounds
    if (*y > 0) { // Ensure we're not at the top edge
        *y -= 1; // Move south

        // Check if the map at the new position has been generated/visited
        if (world[*x][*y] == NULL) {
            // Create the map since it hasn't been visited
            world[*x][*y] = createMap(*x, *y);
            if (world[*x][*y] == NULL) {
                printf("Failed to generate new map.\n");
                return; // Early return if map generation fails
            }
        }

        printMap(world[*x][*y]);
        printf("Moved to new position: (%d, %d)\n", *x - CENTER, *y - CENTER);
    } else {
        // If moving south would go out of bounds, print an error or handle as needed
        printMap(world[*x][*y]);
        printf("Cannot move further south. You're at the edge of the world.\n");
    }
}

// moves the player north
void moveNorth(int *x, int *y) {
    // Increase y to move north, but check for world bounds
    if (*y < WORLD_SIZE - 1) { // Ensure we're not at the bottom edge
        *y += 1; // Move north

        // Check if the map at the new position has been generated/visited
        if (world[*x][*y] == NULL) {
            // Create the map since it hasn't been visited
            world[*x][*y] = createMap(*x, *y);
            if (world[*x][*y] == NULL) {
                printf("Failed to generate new map.\n");
                return; // Early return if map generation fails
            }
        }

        printMap(world[*x][*y]);
        printf("Moved to new position: (%d, %d)\n", *x - CENTER, *y - CENTER);
    } else {
        // If moving north would go out of bounds, print an error or handle as needed
        printMap(world[*x][*y]);
        printf("Cannot move further north. You're at the edge of the world.\n");
    }
}

// moves the player west
void moveWest(int *x, int *y) {
    // Decrease x to move west, but check for world bounds
    if (*x > 0) { // Ensure we're not at the left edge
        *x -= 1; // Move west

        // Check if the map at the new position has been generated/visited
        if (world[*x][*y] == NULL) {
            // Create the map since it hasn't been visited
            world[*x][*y] = createMap(*x, *y);
            if (world[*x][*y] == NULL) {
                printf("Failed to generate new map.\n");
                return; // Early return if map generation fails
            }
        }

        printMap(world[*x][*y]);
        printf("Moved to new position: (%d, %d)\n", *x - CENTER, *y - CENTER);
    } else {
        // If moving west would go out of bounds, print an error or handle as needed
        printMap(world[*x][*y]);
        printf("Cannot move further west. You're at the edge of the world.\n");
    }
}

// moves the player east
void moveEast(int *x, int *y) {
    // Increase x to move east, but check for world bounds
    if (*x < WORLD_SIZE - 1) { // Ensure we're not at the right edge
        *x += 1; // Move east

        // Check if the map at the new position has been generated/visited
        if (world[*x][*y] == NULL) {
            // Create the map since it hasn't been visited
            world[*x][*y] = createMap(*x, *y);
            if (world[*x][*y] == NULL) {
                printf("Failed to generate new map.\n");
                return; // Early return if map generation fails
            }
        }

        printMap(world[*x][*y]);
        printf("Moved to new position: (%d, %d)\n", *x - CENTER, *y - CENTER);
    } else {
        // If moving east would go out of bounds, print an error or handle as needed
        printMap(world[*x][*y]);
        printf("Cannot move further east. You're at the edge of the world.\n");
    }
}

// flies the player to a specified x, y location
void fly(int *x, int *y) {
    // user should input a value -200 to 200 so adjusting x and y for actual bounds
    *x += CENTER;
    *y += CENTER;

    // check for world bounds
    if (*x < WORLD_SIZE || *x >= 0 || *y < WORLD_SIZE || *y >= 0) {

        // Check if the map at the new position has been generated/visited
        if (world[*x][*y] == NULL) {
            // Create the map since it hasn't been visited
            world[*x][*y] = createMap(*x, *y);
            if (world[*x][*y] == NULL) {
                printf("Failed to generate new map.\n");
                return; // Early return if map generation fails
            }
        }

        printMap(world[*x][*y]);
        printf("Moved to new position: (%d, %d)\n", *x - CENTER, *y - CENTER);
    } else {
        // If moving would go out of bounds, print an error or handle as needed
        printMap(world[*x][*y]);
        printf("Cannot move further. You're at the edge of the world.\n");
    }
}

// handles user input and is the actual game
void gameLoop() {
    char command[10];
    int x = WORLD_SIZE / 2, y = WORLD_SIZE / 2; // Start at the center
    world[x][y] = createMap(x, y);
    printMap(world[x][y]);
    printf("Position: (%d, %d)\n", x - CENTER, y - CENTER);

    while (1) {
        printf("Enter command: ");
        scanf("%s", command);

        if (strcmp(command, "n") == 0) {
            moveNorth(&x, &y);
        } else if (strcmp(command, "s") == 0) {
            moveSouth(&x, &y);
        } else if (strcmp(command, "e") == 0) {
            moveEast(&x, &y);
        } else if (strcmp(command, "w") == 0) {
            moveWest(&x, &y);
        } else if (strcmp(command, "q") == 0) {
            break; // Exit loop
        } else if (strcmp(command, "f") == 0) {
            printf("Enter x and y coordinates to fly to: ");
            int result = scanf("%d %d", &x, &y);
            if (result < 2) { // Check if reading x and y was successful
                printf("Error, please enter the x, y coordinates to fly to.\n");
                // Clear the input buffer
                while (getchar() != '\n');
            } else if (x > 200 || y > 200 || x < -200 || y < -200) { // out of bounds
                printf("Error, please enter value between -200 to 200.\n");
            } else {
                fly(&x, &y);
            }
        } else {
            printf("Invalid command.\n");
        }
    }
    // free the world before exiting
    for (int i = 0; i < WORLD_SIZE; i++) {
        for (int j = 0; j < WORLD_SIZE; j++) {
            free(world[i][j]);
        }
    }
}


int main(int argc, char *argv[])
{  
    srand(time(NULL));
    gameLoop();

    return 0;
}