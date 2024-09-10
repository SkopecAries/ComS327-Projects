#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <math.h>

// all of the defines set for the appropriate character types
#define M_WIDTH 80
#define M_HEIGHT 21

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

// creates the boulder border for the map
void border()
{
    for (int j = 0; j < M_WIDTH; j++)
    {
        map[0][j] = BOULDER;
        map[M_HEIGHT - 1][j] = BOULDER;
    }
    for (int i = 0; i < M_HEIGHT; i++)
    {
        map[i][0] = BOULDER;
        map[i][M_WIDTH - 1] = BOULDER;
    }
}

// creates a clearing
void clearing()
{
    for (int i = 0; i < M_HEIGHT; i++)
    {
        for (int j = 0; j < M_WIDTH; j++)
        {
            map[i][j] = GRASS;
        }
    }
}

// creates water
void water()
{
    int num = rand() % 3 + 1; // number of ponds in the map (1-4)

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
                if (map[i][j] == GRASS)
                {
                    map[i][j] = WATER;
                }
            }
        }
        num--;
    }
}

// creates tall grass
void tallGrass()
{
    int num = rand() % 3 + 2; // amount of tall-grass patches (2-5)

    // generates the grass
    while (num != 0)
    {
        int size = rand() % 18 + 3; // Random tall-grass size between 3 and 20
        int x = rand() % (M_HEIGHT - size - 2) + 1;
        int y = rand() % (M_WIDTH - size - 2) + 1;

        for (int i = x; i < x + size; i++)
        {
            for (int j = y; j < y + size; j++)
            {
                if (map[i][j] == GRASS)
                {
                    map[i][j] = TALL_GRASS;
                }
            }
        }
        num--;
    }
}

// generates trees/rocks depending on the char input
void sprinkle(char c)
{
    for (int i = 1; i < M_HEIGHT - 1; i++)
    {
        for (int j = 1; j < M_WIDTH - 1; j++)
        {
            if (map[i][j] == GRASS && (rand() % 50) == 0)
            { // 2% chance to place a tree/rock
                map[i][j] = c;
            }
        }
    }
}

// creates the intersecting paths
void path()
{
    // pick random spot for "gate" on the x axis
    int x1 = (rand() % (M_HEIGHT - 2) + 1);
    int x2 = (rand() % (M_HEIGHT - 2) + 1);
    // same but on the y axis
    int y1 = (rand() % (M_WIDTH - 2) + 1);
    int y2 = (rand() % (M_WIDTH - 2) + 1);

    map[0][y1] = PATH; // north gate
    // map[M_HEIGHT - 1][y2] = PATH; // south gate (ended up just having it drawn in with the path)
    map[x1][0] = PATH; // west gate
    // map[x2][M_WIDTH - 1] = PATH;  // east gate (same as south)

    // random x and y points for paths to go towards
    int x = (rand() % (M_HEIGHT - 2) + 1);
    int y = (rand() % (M_WIDTH - 2) + 1);

    // north to south path
    for (int i = 1; i < x; i++)
    {
        map[i][y1] = PATH;
    }
    // deals with any diagonals
    int j = y1;
    for (int i = x; i < M_HEIGHT && j < M_WIDTH; i++)
    {
        if (j != y2)
        {
            if (j < y2)
            {
                j++;
            }
            if (j > y2)
            {
                j--;
            }
        }
        map[i][j] = PATH;
    }

    // west to east path
    for (int j = 1; j < y; j++)
    {
        map[x1][j] = PATH;
    }
    // deals with any diagonals
    int i = x1;
    for (int j = y; j < M_WIDTH && i < M_HEIGHT; j++)
    {
        if (i != x2)
        {
            if (i < x2)
            {
                i++;
            }
            if (i > x2)
            {
                i--;
            }
        }
        map[i][j] = PATH;
    }
}

// puts valid locations for builds to go into an array
void validBuild(int *locations, int *size)
{
    *size = 0;
    for (int i = 1; i < M_HEIGHT - 1; i++)
    {
        for (int j = 1; j < M_WIDTH - 1; j++)
        {
            // checks if a path is around
            if (map[i][j] == GRASS && (map[i - 1][j] == PATH || map[i + 1][j] == PATH || map[i][j - 1] == PATH || map[i][j + 1] == PATH))
            {
                locations[*size] = i * M_WIDTH + j;
                (*size)++;
            }
        }
    }
}

// places down the builds randomly next to the path
void placeBuilds()
{
    int locations[M_HEIGHT * M_WIDTH];
    int size;
    validBuild(locations, &size);

    // generates position for C
    int rand1 = rand() % size;
    int position = locations[rand1];
    map[position / M_WIDTH][position % M_WIDTH] = POKEMON_CENTER;
    locations[rand1] = locations[rand1 + 1];
    size--;

    // generates position for M
    rand1 = rand() % size;
    position = locations[rand1];
    map[position / M_WIDTH][position % M_WIDTH] = POKEMART;
    locations[rand1] = locations[rand1 + 1];
    size--;
}

// creates the char map within the map array
void createMap()
{
    clearing();
    water();
    tallGrass();
    border();
    path();
    placeBuilds();
    sprinkle(TREE);
    sprinkle(BOULDER);
}

// prints the map
void printMap()
{
    for (int i = 0; i < M_HEIGHT; i++)
    {
        for (int j = 0; j < M_WIDTH; j++)
        {
            putchar(map[i][j]);
        }
        putchar('\n');
    }
}

int main(int argc, char *argv[])
{
    srand(time(NULL));
    createMap();
    printMap();

    return 0;
}