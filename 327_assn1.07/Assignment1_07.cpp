#include <ncurses.h>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <climits>
#include <sys/time.h>
#include <cassert>
#include <unistd.h>
#include <iostream>
#include <vector>
#include <fstream>
#include <sstream>
#include <sys/stat.h>

#include "heap.h"

#define malloc(size) ({          \
  void *_tmp;                    \
  assert((_tmp = malloc(size))); \
  _tmp;                          \
})

class path_t {
  public:
    heap_node_t *hn;
    uint8_t pos[2];
    uint8_t from[2];
    int32_t cost;
};

typedef enum dim {
  dim_x,
  dim_y,
  num_dims
} dim_t;

typedef int16_t pair_t[num_dims];

#define MAP_X              80
#define MAP_Y              21
#define MIN_TREES          10
#define MIN_BOULDERS       10
#define TREE_PROB          95
#define BOULDER_PROB       95
#define WORLD_SIZE         401

#define MOUNTAIN_SYMBOL       '%'
#define BOULDER_SYMBOL        '0'
#define TREE_SYMBOL           '4'
#define FOREST_SYMBOL         '^'
#define GATE_SYMBOL           '#'
#define PATH_SYMBOL           '#'
#define POKEMART_SYMBOL       'M'
#define POKEMON_CENTER_SYMBOL 'C'
#define TALL_GRASS_SYMBOL     ':'
#define SHORT_GRASS_SYMBOL    '.'
#define WATER_SYMBOL          '~'
#define ERROR_SYMBOL          '&'
#define PC_SYMBOL             '@'
#define HIKER_SYMBOL          'h'
#define RIVAL_SYMBOL          'r'
#define PACER_SYMBOL          'p'
#define SENTRY_SYMBOL         's'
#define WANDERER_SYMBOL       'w'
#define EXPLORER_SYMBOL       'e'

#define DIJKSTRA_PATH_MAX (INT_MAX / 2)

#define mappair(pair) (m->map[pair[dim_y]][pair[dim_x]])
#define mapxy(x, y) (m->map[y][x])
#define heightpair(pair) (m->height[pair[dim_y]][pair[dim_x]])
#define heightxy(x, y) (m->height[y][x])

typedef enum __attribute__ ((__packed__)) terrain_type {
  ter_boulder,
  ter_tree,
  ter_path,
  ter_mart,
  ter_center,
  ter_grass,
  ter_clearing,
  ter_mountain,
  ter_forest,
  ter_water,
  ter_gate,
  num_terrain_types,
  ter_debug
} terrain_type_t;

typedef enum __attribute__ ((__packed__)) character_type {
  char_pc,
  char_hiker,
  char_rival,
  char_pacer,
  char_wanderer,
  char_sentry,
  char_explorer,
  num_character_types
} character_type_t;

class Character {
  public:
    pair_t pos;
};

class npc_t : public Character {
  public:
    character_type_t type;
    int next_move; // current time plus the character’s movement cost
    int sequence_num;
    pair_t dir; // direction x and y
    int defeated; // marks if npc has been defeated or not
};

class pc_t : public Character {
  public:
};

class map_t {
  public:
    terrain_type_t map[MAP_Y][MAP_X];
    uint8_t height[MAP_Y][MAP_X];
    int8_t n, s, e, w;
    npc_t *trainer_map[MAP_Y][MAP_X];
    heap_t trainer_heap;
};

class queue_node_t {
  public:
    int x, y;
    struct queue_node *next;
};

class world_t {
  public:
    map_t *world[WORLD_SIZE][WORLD_SIZE];
    pair_t cur_idx;
    map_t *cur_map;
    /* Place distance maps in world, not map, since *
    * we only need one pair at any given time.     */
    int hiker_dist[MAP_Y][MAP_X];
    int rival_dist[MAP_Y][MAP_X];
    pc_t pc;
};

struct pokemon {
  int id;
  char identifier[30];
  int species_id;
  int height;
  int weight;
  int base_expierence;
  int order;
  int is_default;
};

struct moves {
  int id;
  char identifier[35];
  int generation_id;
  int type_id;
  int power;
  int pp;
  int accuracy;
  int priority;
  int target_id;
  int damage_class_id;
  int effect_id;
  int effect_chance;
  int contest_type_id;
  int contest_effect_id;
  int super_contest_effect_id;
};

struct pokemon_moves {
  int pokemon_id;
  int version_group_id;
  int move_id;
  int pokemon_move_method_id;
  int level;
  int order;
};

struct pokemon_species {
  int id;
  char identifier[30];
  int generation_id;
  int evolves_from_species_id;
  int evolution_chain_id;
  int color_id;
  int shape_id;
  int habitat_id;
  int gender_rate;
  int capture_rate;
  int base_happiness;
  int is_baby;
  int hatch_counter;
  int has_gender_differences;
  int growth_rate_id;
  int forms_switchable;
  int is_legendary;
  int is_mythical;
  int order;
  int conquest_order;
};

struct experience {
  int growth_rate_id;
  int level;
  int experience;
};

struct type_names {
  int type_id;
  int local_language_id;
  char name[30];
};

struct pokemon_stats {
  int pokemon_id;
  int stat_id;
  int base_stat;
  int effort;
};

struct stats {
  int id;
  int damage_class_id;
  char identifier[15];
  int is_battle_only;
  int game_index;
};

struct pokemon_types {
  int pokemon_id;
  int type_id;
  int slot;
};

/* Even unallocated, a WORLD_SIZE x WORLD_SIZE array of pointers is a very *
 * large thing to put on the stack.  To avoid that, world is a global.     */
world_t world;
int numtrainers = 10;
int poke_file;


/* Just to make the following table fit in 80 columns */
// adjusting temporarily so pc can't go on gates for now
#define IM DIJKSTRA_PATH_MAX
int32_t move_cost[num_character_types][num_terrain_types] = {
//  boulder,tree,path,mart,center,grass,clearing,mountain,forest,water,gate
  { IM, IM, 10, 10, 10, 20, 10, IM, IM, IM, 10 }, // pc
  { IM, IM, 10, 50, 50, 15, 10, 15, 15, IM, IM }, // hiker
  { IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM }, // rival
  { IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM }, // other
  { IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM },
  { IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM },
  { IM, IM, 10, 50, 50, 20, 10, IM, IM, IM, IM }
};
#undef IM

static int32_t path_cmp(const void *key, const void *with) {
  return ((path_t *) key)->cost - ((path_t *) with)->cost;
}

static int32_t edge_penalty(int8_t x, int8_t y)
{
  return (x == 1 || y == 1 || x == MAP_X - 2 || y == MAP_Y - 2) ? 2 : 1;
}

// used what shaeffer said in week 7 notes
void init_term() {
  initscr();
  raw();
  noecho();
  curs_set(0);
  keypad(stdscr, TRUE);
  start_color();
  init_pair(COLOR_RED, COLOR_RED, COLOR_BLACK);
  init_pair(COLOR_GREEN, COLOR_GREEN, COLOR_BLACK);
  init_pair(COLOR_YELLOW, COLOR_YELLOW, COLOR_BLACK);
  init_pair(COLOR_BLUE, COLOR_BLUE, COLOR_BLACK);
  init_pair(COLOR_MAGENTA, COLOR_MAGENTA, COLOR_BLACK);
  init_pair(COLOR_CYAN, COLOR_CYAN, COLOR_BLACK);
  init_pair(COLOR_WHITE, COLOR_WHITE, COLOR_BLACK);
}

static void dijkstra_path(map_t *m, pair_t from, pair_t to)
{
  static path_t path[MAP_Y][MAP_X], *p;
  static uint32_t initialized = 0;
  heap_t h;
  uint32_t x, y;

  if (!initialized) {
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        path[y][x].pos[dim_y] = y;
        path[y][x].pos[dim_x] = x;
      }
    }
    initialized = 1;
  }
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      path[y][x].cost = DIJKSTRA_PATH_MAX;
    }
  }

  path[from[dim_y]][from[dim_x]].cost = 0;

  heap_init(&h, path_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      path[y][x].hn = heap_insert(&h, &path[y][x]);
    }
  }

  while ((p = (path_t *)heap_remove_min(&h))) {
    p->hn = NULL;

    if ((p->pos[dim_y] == to[dim_y]) && p->pos[dim_x] == to[dim_x]) {
      for (x = to[dim_x], y = to[dim_y];
           (x != (uint32_t)from[dim_x]) || (y != (uint32_t)from[dim_y]);
           p = &path[y][x], x = p->from[dim_x], y = p->from[dim_y]) {
        /* Don't overwrite the gate */
        if (x != (uint32_t)to[dim_x] || y != (uint32_t)to[dim_y]) {
          mapxy(x, y) = ter_path;
          heightxy(x, y) = 0;
        }
      }
      heap_delete(&h);
      return;
    }

    if ((path[p->pos[dim_y] - 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1)))) {
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] - 1));
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] - 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] - 1]
                                           [p->pos[dim_x]    ].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] - 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] - 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] - 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] - 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] - 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] - 1].hn);
    }
    if ((path[p->pos[dim_y]    ][p->pos[dim_x] + 1].hn) &&
        (path[p->pos[dim_y]    ][p->pos[dim_x] + 1].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y])))) {
      path[p->pos[dim_y]][p->pos[dim_x] + 1].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x] + 1, p->pos[dim_y]));
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y]    ][p->pos[dim_x] + 1].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y]    ]
                                           [p->pos[dim_x] + 1].hn);
    }
    if ((path[p->pos[dim_y] + 1][p->pos[dim_x]    ].hn) &&
        (path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost >
         ((p->cost + heightpair(p->pos)) *
          edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1)))) {
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].cost =
        ((p->cost + heightpair(p->pos)) *
         edge_penalty(p->pos[dim_x], p->pos[dim_y] + 1));
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_y] = p->pos[dim_y];
      path[p->pos[dim_y] + 1][p->pos[dim_x]    ].from[dim_x] = p->pos[dim_x];
      heap_decrease_key_no_replace(&h, path[p->pos[dim_y] + 1]
                                           [p->pos[dim_x]    ].hn);
    }
  }
}

static int build_paths(map_t *m)
{
  pair_t from, to;

  /*  printf("%d %d %d %d\n", m->n, m->s, m->e, m->w);*/

  if (m->e != -1 && m->w != -1) {
    from[dim_x] = 1;
    to[dim_x] = MAP_X - 2;
    from[dim_y] = m->w;
    to[dim_y] = m->e;

    dijkstra_path(m, from, to);
  }

  if (m->n != -1 && m->s != -1) {
    from[dim_y] = 1;
    to[dim_y] = MAP_Y - 2;
    from[dim_x] = m->n;
    to[dim_x] = m->s;

    dijkstra_path(m, from, to);
  }

  if (m->e == -1) {
    if (m->s == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->w == -1) {
    if (m->s == -1) {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->n == -1) {
    if (m->e == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->s;
      to[dim_y] = MAP_Y - 2;
    }

    dijkstra_path(m, from, to);
  }

  if (m->s == -1) {
    if (m->e == -1) {
      from[dim_x] = 1;
      from[dim_y] = m->w;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    } else {
      from[dim_x] = MAP_X - 2;
      from[dim_y] = m->e;
      to[dim_x] = m->n;
      to[dim_y] = 1;
    }

    dijkstra_path(m, from, to);
  }

  return 0;
}

static int gaussian[5][5] = {
  {  1,  4,  7,  4,  1 },
  {  4, 16, 26, 16,  4 },
  {  7, 26, 41, 26,  7 },
  {  4, 16, 26, 16,  4 },
  {  1,  4,  7,  4,  1 }
};

static int smooth_height(map_t *m)
{
  int32_t i, x, y;
  int32_t s, t, p, q;
  queue_node_t *head, *tail, *tmp;
  /*  FILE *out;*/
  uint8_t height[MAP_Y][MAP_X];

  memset(&height, 0, sizeof (height));

  /* Seed with some values */
  for (i = 1; i < 255; i += 20) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (height[y][x]);
    height[y][x] = i;
    if (i == 1) {
      head = tail = (queue_node_t *)malloc(sizeof (*tail));
    } else {
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);
  */
  
  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = height[y][x];

    if (x - 1 >= 0 && y - 1 >= 0 && !height[y - 1][x - 1]) {
      height[y - 1][x - 1] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y - 1;
    }
    if (x - 1 >= 0 && !height[y][x - 1]) {
      height[y][x - 1] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y;
    }
    if (x - 1 >= 0 && y + 1 < MAP_Y && !height[y + 1][x - 1]) {
      height[y + 1][x - 1] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x - 1;
      tail->y = y + 1;
    }
    if (y - 1 >= 0 && !height[y - 1][x]) {
      height[y - 1][x] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y - 1;
    }
    if (y + 1 < MAP_Y && !height[y + 1][x]) {
      height[y + 1][x] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x;
      tail->y = y + 1;
    }
    if (x + 1 < MAP_X && y - 1 >= 0 && !height[y - 1][x + 1]) {
      height[y - 1][x + 1] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y - 1;
    }
    if (x + 1 < MAP_X && !height[y][x + 1]) {
      height[y][x + 1] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y;
    }
    if (x + 1 < MAP_X && y + 1 < MAP_Y && !height[y + 1][x + 1]) {
      height[y + 1][x + 1] = i;
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
      tail->next = NULL;
      tail->x = x + 1;
      tail->y = y + 1;
    }

    tmp = head;
    head = (queue_node_t *)head->next;
    free(tmp);
  }

  /* And smooth it a bit with a gaussian convolution */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }
  /* Let's do it again, until it's smooth like Kenny G. */
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      for (s = t = p = 0; p < 5; p++) {
        for (q = 0; q < 5; q++) {
          if (y + (p - 2) >= 0 && y + (p - 2) < MAP_Y &&
              x + (q - 2) >= 0 && x + (q - 2) < MAP_X) {
            s += gaussian[p][q];
            t += height[y + (p - 2)][x + (q - 2)] * gaussian[p][q];
          }
        }
      }
      m->height[y][x] = t / s;
    }
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&height, sizeof (height), 1, out);
  fclose(out);

  out = fopen("smoothed.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->height, sizeof (m->height), 1, out);
  fclose(out);
  */

  return 0;
}

static void find_building_location(map_t *m, pair_t p)
{
  do {
    p[dim_x] = rand() % (MAP_X - 3) + 1;
    p[dim_y] = rand() % (MAP_Y - 3) + 1;

    if ((((mapxy(p[dim_x] - 1, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] - 1, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x] + 2, p[dim_y]    ) == ter_path)     &&
          (mapxy(p[dim_x] + 2, p[dim_y] + 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] - 1) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] - 1) == ter_path))    ||
         ((mapxy(p[dim_x]    , p[dim_y] + 2) == ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 2) == ter_path)))   &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_center)   &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_center)   &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_mart)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_center))) &&
        (((mapxy(p[dim_x]    , p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y]    ) != ter_path)     &&
          (mapxy(p[dim_x]    , p[dim_y] + 1) != ter_path)     &&
          (mapxy(p[dim_x] + 1, p[dim_y] + 1) != ter_path)))) {
          break;
    }
  } while (1);
}

static int place_pokemart(map_t *m)
{
  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_mart;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_mart;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_mart;

  return 0;
}

static int place_center(map_t *m)
{  pair_t p;

  find_building_location(m, p);

  mapxy(p[dim_x]    , p[dim_y]    ) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y]    ) = ter_center;
  mapxy(p[dim_x]    , p[dim_y] + 1) = ter_center;
  mapxy(p[dim_x] + 1, p[dim_y] + 1) = ter_center;

  return 0;
}

/* Chooses tree or boulder for border cell.  Choice is biased by dominance *
 * of neighboring cells.                                                   */
static terrain_type_t border_type(map_t *m, int32_t x, int32_t y)
{
  int32_t p, q;
  int32_t r, t;
  int32_t miny, minx, maxy, maxx;
  
  r = t = 0;
  
  miny = y - 1 >= 0 ? y - 1 : 0;
  maxy = y + 1 <= MAP_Y ? y + 1: MAP_Y;
  minx = x - 1 >= 0 ? x - 1 : 0;
  maxx = x + 1 <= MAP_X ? x + 1: MAP_X;

  for (q = miny; q < maxy; q++) {
    for (p = minx; p < maxx; p++) {
      if (q != y || p != x) {
        if (m->map[q][p] == ter_mountain ||
            m->map[q][p] == ter_boulder) {
          r++;
        } else if (m->map[q][p] == ter_forest ||
                   m->map[q][p] == ter_tree) {
          t++;
        }
      }
    }
  }
  
  if (t == r) {
    return rand() & 1 ? ter_boulder : ter_tree;
  } else if (t > r) {
    if (rand() % 10) {
      return ter_tree;
    } else {
      return ter_boulder;
    }
  } else {
    if (rand() % 10) {
      return ter_boulder;
    } else {
      return ter_tree;
    }
  }
}

static int map_terrain(map_t *m, int8_t n, int8_t s, int8_t e, int8_t w)
{
  int32_t i, x, y;
  queue_node_t *head, *tail, *tmp;
  //  FILE *out;
  int num_grass, num_clearing, num_mountain, num_forest, num_water, num_total;
  terrain_type_t type;
  int added_current = 0;
  
  num_grass = rand() % 4 + 2;
  num_clearing = rand() % 4 + 2;
  num_mountain = rand() % 2 + 1;
  num_forest = rand() % 2 + 1;
  num_water = rand() % 2 + 1;
  num_total = num_grass + num_clearing + num_mountain + num_forest + num_water;

  memset(&m->map, 0, sizeof (m->map));

  /* Seed with some values */
  for (i = 0; i < num_total; i++) {
    do {
      x = rand() % MAP_X;
      y = rand() % MAP_Y;
    } while (m->map[y][x]);
    if (i == 0) {
      type = ter_grass;
    } else if (i == num_grass) {
      type = ter_clearing;
    } else if (i == num_grass + num_clearing) {
      type = ter_mountain;
    } else if (i == num_grass + num_clearing + num_mountain) {
      type = ter_forest;
    } else if (i == num_grass + num_clearing + num_mountain + num_forest) {
      type = ter_water;
    }
    m->map[y][x] = type;
    if (i == 0) {
      head = tail = (queue_node_t *)malloc(sizeof (*tail));
    } else {
      tail->next = (queue_node *)malloc(sizeof (*tail));
      tail = (queue_node_t *)tail->next;
    }
    tail->next = NULL;
    tail->x = x;
    tail->y = y;
  }

  /*
  out = fopen("seeded.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */

  /* Diffuse the vaules to fill the space */
  while (head) {
    x = head->x;
    y = head->y;
    i = m->map[y][x];
    
    if (x - 1 >= 0 && !m->map[y][x - 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x - 1] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x - 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y - 1 >= 0 && !m->map[y - 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y - 1][x] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y - 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (y + 1 < MAP_Y && !m->map[y + 1][x]) {
      if ((rand() % 100) < 20) {
        m->map[y + 1][x] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y + 1;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    if (x + 1 < MAP_X && !m->map[y][x + 1]) {
      if ((rand() % 100) < 80) {
        m->map[y][x + 1] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x + 1;
        tail->y = y;
      } else if (!added_current) {
        added_current = 1;
        m->map[y][x] = (terrain_type_t) i;
        tail->next = (queue_node *)malloc(sizeof (*tail));
        tail = (queue_node_t *)tail->next;
        tail->next = NULL;
        tail->x = x;
        tail->y = y;
      }
    }

    added_current = 0;
    tmp = head;
    head = (queue_node_t *)head->next;
    free(tmp);
  }

  /*
  out = fopen("diffused.pgm", "w");
  fprintf(out, "P5\n%u %u\n255\n", MAP_X, MAP_Y);
  fwrite(&m->map, sizeof (m->map), 1, out);
  fclose(out);
  */
  
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (y == 0 || y == MAP_Y - 1 ||
          x == 0 || x == MAP_X - 1) {
        mapxy(x, y) = border_type(m, x, y);
      }
    }
  }

  m->n = n;
  m->s = s;
  m->e = e;
  m->w = w;

  if (n != -1) {
    mapxy(n,         0        ) = ter_gate;
    mapxy(n,         1        ) = ter_gate;
  }
  if (s != -1) {
    mapxy(s,         MAP_Y - 1) = ter_gate;
    mapxy(s,         MAP_Y - 2) = ter_gate;
  }
  if (w != -1) {
    mapxy(0,         w        ) = ter_gate;
    mapxy(1,         w        ) = ter_gate;
  }
  if (e != -1) {
    mapxy(MAP_X - 1, e        ) = ter_gate;
    mapxy(MAP_X - 2, e        ) = ter_gate;
  }

  return 0;
}

static int place_boulders(map_t *m)
{
  int i;
  int x, y;

  for (i = 0; i < MIN_BOULDERS || rand() % 100 < BOULDER_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_forest &&
        m->map[y][x] != ter_path   &&
        m->map[y][x] != ter_gate) {
      m->map[y][x] = ter_boulder;
    }
  }

  return 0;
}

static int place_trees(map_t *m)
{
  int i;
  int x, y;
  
  for (i = 0; i < MIN_TREES || rand() % 100 < TREE_PROB; i++) {
    y = rand() % (MAP_Y - 2) + 1;
    x = rand() % (MAP_X - 2) + 1;
    if (m->map[y][x] != ter_mountain &&
        m->map[y][x] != ter_path     &&
        m->map[y][x] != ter_water    &&
        m->map[y][x] != ter_gate) {
      m->map[y][x] = ter_tree;
    }
  }

  return 0;
}

static void print_map()
{
  int x, y;
  int default_reached = 0;

  clear();
  //printf("\n\n\n");

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.pc.pos[dim_y] == y &&
          world.pc.pos[dim_x] == x) {
        mvaddch(y + 1, x, PC_SYMBOL);
      } else {
        if (world.cur_map->trainer_map[y][x] != NULL) {
          switch(world.cur_map->trainer_map[y][x]->type) {
          case char_hiker:
            mvaddch(y + 1, x, HIKER_SYMBOL);
            break;
          case char_rival:
            mvaddch(y + 1, x, RIVAL_SYMBOL);
            break;
          case char_pacer:
            mvaddch(y + 1, x, PACER_SYMBOL);
            break;
          case char_sentry:
            mvaddch(y + 1, x, SENTRY_SYMBOL);
            break;
          case char_wanderer:
            mvaddch(y + 1, x, WANDERER_SYMBOL);
            break;
          case char_explorer:
            mvaddch(y + 1, x, EXPLORER_SYMBOL);
            break;
          case char_pc:
            break;
          default:
            mvaddch(y, x, ERROR_SYMBOL);
            default_reached = 1;
            break;
          }
        }
        else {
          switch (world.cur_map->map[y][x]) {
          case ter_boulder:
            attron(COLOR_PAIR(COLOR_RED));
            mvaddch(y + 1, x, BOULDER_SYMBOL);
            attroff(COLOR_PAIR(COLOR_RED));
            break;
          case ter_mountain:
            attron(COLOR_PAIR(COLOR_RED));
            mvaddch(y + 1, x, MOUNTAIN_SYMBOL);
            attroff(COLOR_PAIR(COLOR_RED));
            break;
          case ter_tree:
            attron(COLOR_PAIR(COLOR_CYAN));
            mvaddch(y + 1, x, TREE_SYMBOL);
            attroff(COLOR_PAIR(COLOR_CYAN));
            break;
          case ter_forest:
            attron(COLOR_PAIR(COLOR_CYAN));
            mvaddch(y + 1, x, FOREST_SYMBOL);
            attroff(COLOR_PAIR(COLOR_CYAN));
            break;
          case ter_path:
            attron(COLOR_PAIR(COLOR_YELLOW));
            mvaddch(y + 1, x, PATH_SYMBOL);
            attroff(COLOR_PAIR(COLOR_YELLOW));
            break;
          case ter_gate:
            attron(COLOR_PAIR(COLOR_YELLOW));
            mvaddch(y + 1, x, GATE_SYMBOL);
            attroff(COLOR_PAIR(COLOR_YELLOW));
            break;
          case ter_mart:
            attron(COLOR_PAIR(COLOR_MAGENTA));
            mvaddch(y + 1, x, POKEMART_SYMBOL);
            attroff(COLOR_PAIR(COLOR_MAGENTA));
            break;
          case ter_center:
            attron(COLOR_PAIR(COLOR_MAGENTA));
            mvaddch(y + 1, x, POKEMON_CENTER_SYMBOL);
            attroff(COLOR_PAIR(COLOR_MAGENTA));
            break;
          case ter_grass:
            attron(COLOR_PAIR(COLOR_GREEN));
            mvaddch(y + 1, x, TALL_GRASS_SYMBOL);
            attroff(COLOR_PAIR(COLOR_GREEN));
            break;
          case ter_clearing:
            attron(COLOR_PAIR(COLOR_GREEN));
            mvaddch(y + 1, x, SHORT_GRASS_SYMBOL);
            attroff(COLOR_PAIR(COLOR_GREEN));
            break;
          case ter_water:
            attron(COLOR_PAIR(COLOR_BLUE));
            mvaddch(y + 1, x, WATER_SYMBOL);
            attroff(COLOR_PAIR(COLOR_BLUE));
            break;
          default:
            mvaddch(y + 1, x, ERROR_SYMBOL);
            default_reached = 1;
            break;
          }
        }
      }
    }
  }

  if (default_reached) {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }

  refresh();
}

#define ter_cost(x, y, c) move_cost[c][m->map[y][x]]

static int32_t hiker_cmp(const void *key, const void *with) {
  return (world.hiker_dist[((path_t *) key)->pos[dim_y]]
                          [((path_t *) key)->pos[dim_x]] -
          world.hiker_dist[((path_t *) with)->pos[dim_y]]
                          [((path_t *) with)->pos[dim_x]]);
}

static int32_t rival_cmp(const void *key, const void *with) {
  return (world.rival_dist[((path_t *) key)->pos[dim_y]]
                          [((path_t *) key)->pos[dim_x]] -
          world.rival_dist[((path_t *) with)->pos[dim_y]]
                          [((path_t *) with)->pos[dim_x]]);
}

static int32_t npc_cmp(const void *key, const void *with) {
  return ((npc_t *)key)->next_move - ((npc_t *)with)->next_move;
}

void pathfind(map_t *m)
{
  heap_t h;
  uint32_t x, y;
  static path_t p[MAP_Y][MAP_X], *c;
  static uint32_t initialized = 0;

  if (!initialized) {
    initialized = 1;
    for (y = 0; y < MAP_Y; y++) {
      for (x = 0; x < MAP_X; x++) {
        p[y][x].pos[dim_y] = y;
        p[y][x].pos[dim_x] = x;
      }
    }
  }

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      world.hiker_dist[y][x] = world.rival_dist[y][x] = DIJKSTRA_PATH_MAX;
    }
  }
  world.hiker_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = 
    world.rival_dist[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = 0;

  heap_init(&h, hiker_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (ter_cost(x, y, char_hiker) != DIJKSTRA_PATH_MAX) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      } else {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = (path_t *)heap_remove_min(&h))) {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker))) {
      world.hiker_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        world.hiker_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_hiker);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);

  heap_init(&h, rival_cmp, NULL);

  for (y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (ter_cost(x, y, char_rival) != DIJKSTRA_PATH_MAX) {
        p[y][x].hn = heap_insert(&h, &p[y][x]);
      } else {
        p[y][x].hn = NULL;
      }
    }
  }

  while ((c = (path_t *)heap_remove_min(&h))) {
    c->hn = NULL;
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x]    ] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] - 1][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] - 1][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y]    ][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y]    ][c->pos[dim_x] + 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] - 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] - 1].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x]    ] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x]    ].hn);
    }
    if ((p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn) &&
        (world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] >
         world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
         ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival))) {
      world.rival_dist[c->pos[dim_y] + 1][c->pos[dim_x] + 1] =
        world.rival_dist[c->pos[dim_y]][c->pos[dim_x]] +
        ter_cost(c->pos[dim_x], c->pos[dim_y], char_rival);
      heap_decrease_key_no_replace(&h,
                                   p[c->pos[dim_y] + 1][c->pos[dim_x] + 1].hn);
    }
  }
  heap_delete(&h);
}

void init_pc()
{
  int x, y;

  do {
    x = rand() % (MAP_X - 2) + 1;
    y = rand() % (MAP_Y - 2) + 1;
  } while (world.cur_map->map[y][x] != ter_path);

  world.pc.pos[dim_x] = x;
  world.pc.pos[dim_y] = y;
}

void init_npc(int numtrainers)
{
  int x, y;
  npc_t *npc;
  heap_init(&world.cur_map->trainer_heap, npc_cmp, NULL);
  

  while(numtrainers > 0) {
    npc = (npc_t *)malloc(sizeof(npc_t));
    switch(numtrainers) { // adding in at least a hiker/rival, then randomly picking the others if there's more
      case 1:
        npc->type = char_hiker;
        npc->sequence_num = numtrainers;
        break;
      case 2:
        npc->type = char_rival;
        npc->sequence_num = numtrainers;
        break;
      default: // the rest will be random
        int random = rand() % 6 + 1;
        int xd_rand = rand() % 3 - 1; // random direction -1 to 1
        int yd_rand = rand() % 3 - 1;
        if (random == 1) {
          npc->type = char_hiker;
          npc->sequence_num = numtrainers;
        } else if (random == 2) {
          npc->type = char_rival;
          npc->sequence_num = numtrainers;
        } else if (random == 3) {
          npc->type = char_pacer;
          npc->sequence_num = numtrainers;
          npc->dir[dim_x] = xd_rand;
          npc->dir[dim_y] = yd_rand;
        } else if (random == 4) {
          npc->type = char_wanderer;
          npc->sequence_num = numtrainers;
          npc->dir[dim_x] = xd_rand;
          npc->dir[dim_y] = yd_rand;
        } else if (random == 5) {
          npc->type = char_sentry;
          npc->sequence_num = numtrainers;
        } else { // aka 6
          npc->type = char_explorer;
          npc->sequence_num = numtrainers;
          npc->dir[dim_x] = xd_rand;
          npc->dir[dim_y] = yd_rand;
        }
    }

    do {
      x = rand() % (MAP_X - 2) + 1;
      y = rand() % (MAP_Y - 2) + 1;
    } while ((move_cost[npc->type][world.cur_map->map[y][x]]) == DIJKSTRA_PATH_MAX || world.cur_map->trainer_map[y][x] != NULL);

    npc->pos[dim_x] = x;
    npc->pos[dim_y] = y;
    world.cur_map->trainer_map[y][x] = (npc_t *)malloc(sizeof(npc_t));
    world.cur_map->trainer_map[y][x] = npc;

    npc->defeated = 0; // not defeated
    npc->next_move = 0; // the first move is 0
    heap_insert(&world.cur_map->trainer_heap, npc);
    numtrainers--;
  }
}

// New map expects cur_idx to refer to the index to be generated.  If that
// map has already been generated then the only thing this does is set
// cur_map.
static int new_map()
{
  int d, p;
  int e, w, n, s;

  if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]]) {
    world.cur_map = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]];
    return 0;
  }

  world.cur_map                                             =
    world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x]] =
    (map_t *)malloc(sizeof (*world.cur_map));

  smooth_height(world.cur_map);
  
  if (!world.cur_idx[dim_y]) {
    n = -1;
  } else if (world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]) {
    n = world.world[world.cur_idx[dim_y] - 1][world.cur_idx[dim_x]]->s;
  } else {
    n = 1 + rand() % (MAP_X - 2);
  }
  if (world.cur_idx[dim_y] == WORLD_SIZE - 1) {
    s = -1;
  } else if (world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]) {
    s = world.world[world.cur_idx[dim_y] + 1][world.cur_idx[dim_x]]->n;
  } else  {
    s = 1 + rand() % (MAP_X - 2);
  }
  if (!world.cur_idx[dim_x]) {
    w = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]) {
    w = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] - 1]->e;
  } else {
    w = 1 + rand() % (MAP_Y - 2);
  }
  if (world.cur_idx[dim_x] == WORLD_SIZE - 1) {
    e = -1;
  } else if (world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]) {
    e = world.world[world.cur_idx[dim_y]][world.cur_idx[dim_x] + 1]->w;
  } else {
    e = 1 + rand() % (MAP_Y - 2);
  }
  
  map_terrain(world.cur_map, n, s, e, w);
     
  place_boulders(world.cur_map);
  place_trees(world.cur_map);
  build_paths(world.cur_map);
  d = (abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)) +
       abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)));
  p = d > 200 ? 5 : (50 - ((45 * d) / 200));
  //  printf("d=%d, p=%d\n", d, p);
  if ((rand() % 100) < p || !d) {
    place_pokemart(world.cur_map);
  }
  if ((rand() % 100) < p || !d) {
    place_center(world.cur_map);
  }

  for (int y = 0; y < MAP_Y; y++)
  {
    for (int x = 0; x < MAP_X; x++)
    {
      world.cur_map->trainer_map[y][x] = NULL;
    }
  }

  init_npc(numtrainers);
  pathfind(world.cur_map);

  return 0;
}

// The world is global because of its size, so init_world is parameterless
void init_world()
{
  world.cur_idx[dim_x] = world.cur_idx[dim_y] = WORLD_SIZE / 2;
  new_map();
}

void delete_world()
{
  int x, y;

  for (y = 0; y < WORLD_SIZE; y++) {
    for (x = 0; x < WORLD_SIZE; x++) {
      if (world.world[y][x]) {
        free(world.world[y][x]);
        world.world[y][x] = NULL;
      }
    }
  }
}

void battle(npc_t *npc) {
  WINDOW *wbattle;
  wbattle = newwin(21, 80, 1, 0);
  box(wbattle, 0, 0);
  mvwprintw(wbattle, 1, 1, "You Won! Press ESC to exit.");

  int esc = 0;
  do {
    if (wgetch(wbattle) == 27) {
      esc = 1;
    }
  } while (!esc);
  delwin(wbattle);
  npc->defeated = 1;
}

// moves pc
void move_pc(int y, int x, int *m) {
  int nx, ny; // next x/y

  nx = world.pc.pos[dim_x] + x;
  ny = world.pc.pos[dim_y] + y;

  // if the pc can't move there print message and return
  if (move_cost[0][world.cur_map->map[ny][nx]] == DIJKSTRA_PATH_MAX) {
    *m = 1;
    refresh();
    return;
  }
  // if there's an npc on that spot start a battle
  if (world.cur_map->trainer_map[ny][nx] != NULL && world.cur_map->trainer_map[ny][nx]->defeated == 0) {
    battle(world.cur_map->trainer_map[ny][nx]);
  }
  // if they're on a gate switch maps
  if (world.cur_map->map[ny][nx] == ter_gate) {
    if (ny == 20) { // south gate
      world.cur_idx[dim_y]++;
      new_map();
      print_map();
      ny = 1;
    }
    else if (ny == 0) { // north gate
      world.cur_idx[dim_y]--;
      new_map();
      print_map();
      ny = 19;
    }
    else if (nx == 79) { // east gate
      world.cur_idx[dim_x]++;
      new_map();
      print_map();
      nx = 1;
    }
    else if (nx == 0) { // east gate
      world.cur_idx[dim_x]--;
      new_map();
      print_map();
      nx = 78;
    }
  }

  world.pc.pos[dim_x] = nx;
  world.pc.pos[dim_y] = ny;

  refresh();
  pathfind(world.cur_map);
}

// moves the npc
void move_npc(npc_t *npc) {
  int nx, ny; // next x/y
  int default_reached = 0;

  switch(npc->type) {
    case char_hiker: // path to the PC by following a maximum gradient on the hiker map
      if (npc->defeated == 1) {
        break;
      }
      world.cur_map->trainer_map[npc->pos[dim_y]][npc->pos[dim_x]] = NULL;

      // checking all directions
      for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
          nx = npc->pos[dim_x] + x;
          ny = npc->pos[dim_y] + y;
          // find a smaller spot on the distance map
          if (world.hiker_dist[ny][nx] < world.hiker_dist[npc->pos[dim_y]][npc->pos[dim_x]]) {
            npc->pos[dim_x] = nx;
            npc->pos[dim_y] = ny;
          }
        }
      }
      if (world.pc.pos[dim_y] == npc->pos[dim_y] && world.pc.pos[dim_x] == npc->pos[dim_x]) {
        battle(npc);
      }
      world.cur_map->trainer_map[npc->pos[dim_y]][npc->pos[dim_x]] = npc;
      break;
    case char_rival: // path to the PC by following a maximum gradient on the rival map
      if (npc->defeated == 1) {
        break;
      }
      world.cur_map->trainer_map[npc->pos[dim_y]][npc->pos[dim_x]] = NULL;
      
      // checking all directions
      for (int y = -1; y <= 1; y++) {
        for (int x = -1; x <= 1; x++) {
          nx = npc->pos[dim_x] + x;
          ny = npc->pos[dim_y] + y;
          // find a smaller spot on the distance map
          if (world.rival_dist[ny][nx] < world.rival_dist[npc->pos[dim_y]][npc->pos[dim_x]]) {
            npc->pos[dim_x] = nx;
            npc->pos[dim_y] = ny;
          }
        }
      }

      if (world.pc.pos[dim_y] == npc->pos[dim_y] && world.pc.pos[dim_x] == npc->pos[dim_x]) {
        battle(npc);
      }
      world.cur_map->trainer_map[npc->pos[dim_y]][npc->pos[dim_x]] = npc;
      break;
    case char_pacer: // start with a direction and walk until they hit some terrain they cannot traverse, then they turn around and repeat, pacing back and forth
      world.cur_map->trainer_map[npc->pos[dim_y]][npc->pos[dim_x]] = NULL;
      nx = npc->pos[dim_x] + npc->dir[dim_x];
      ny = npc->pos[dim_y] + npc->dir[dim_y];
      
      // loops until terrain is valid or npc isn't in that spot
      while (move_cost[npc->type][world.cur_map->map[ny][nx]] == DIJKSTRA_PATH_MAX || world.cur_map->trainer_map[ny][nx] != NULL) {
        npc->dir[dim_x] = -npc->dir[dim_x];
        npc->dir[dim_y] = -npc->dir[dim_y];
        nx = npc->pos[dim_x] + npc->dir[dim_x];
        ny = npc->pos[dim_y] + npc->dir[dim_y];
      }
      
      npc->pos[dim_x] = nx;
      npc->pos[dim_y] = ny;

      if (world.pc.pos[dim_y] == npc->pos[dim_y] && world.pc.pos[dim_x] == npc->pos[dim_x]) {
        battle(npc);
      }
      world.cur_map->trainer_map[ny][nx] = npc;
      break;
    case char_wanderer: // have a direction and walk strait ahead to the edge of the terrain, whereupon they turn in a random direction and repeat
      world.cur_map->trainer_map[npc->pos[dim_y]][npc->pos[dim_x]] = NULL;
      nx = npc->pos[dim_x] + npc->dir[dim_x];
      ny = npc->pos[dim_y] + npc->dir[dim_y];
      
      
      while (world.cur_map->map[ny][nx] != world.cur_map->map[npc->pos[dim_y]][npc->pos[dim_x]] 
      || world.cur_map->trainer_map[ny][nx] != NULL) { // if terrain is different or taken by npc switch direction till it isn't
        int xd_rand = rand() % 3 - 1; // random direction -1 to 1
        int yd_rand = rand() % 3 - 1;
        npc->dir[dim_x] = xd_rand;
        npc->dir[dim_y] = yd_rand;
        nx = npc->pos[dim_x] + npc->dir[dim_x];
        ny = npc->pos[dim_y] + npc->dir[dim_y];
      }
      
      npc->pos[dim_x] = nx;
      npc->pos[dim_y] = ny;

      if (world.pc.pos[dim_y] == npc->pos[dim_y] && world.pc.pos[dim_x] == npc->pos[dim_x]) {
        battle(npc);
      }
      world.cur_map->trainer_map[ny][nx] = npc;
      break;
    case char_explorer: // move like wanderers, but they cross terrain type boundaries, only changing to a new, random direction when they reach an impassable terrain element
      world.cur_map->trainer_map[npc->pos[dim_y]][npc->pos[dim_x]] = NULL;
      nx = npc->pos[dim_x] + npc->dir[dim_x];
      ny = npc->pos[dim_y] + npc->dir[dim_y];
      
      while (move_cost[npc->type][world.cur_map->map[ny][nx]] == DIJKSTRA_PATH_MAX || world.cur_map->trainer_map[ny][nx] != NULL) {
        int xd_rand = rand() % 3 - 1; // random direction -1 to 1
        int yd_rand = rand() % 3 - 1;
        npc->dir[dim_x] = xd_rand;
        npc->dir[dim_y] = yd_rand;
        nx = npc->pos[dim_x] + npc->dir[dim_x];
        ny = npc->pos[dim_y] + npc->dir[dim_y];
      }
      
      npc->pos[dim_x] = nx;
      npc->pos[dim_y] = ny;

      if (world.pc.pos[dim_y] == npc->pos[dim_y] && world.pc.pos[dim_x] == npc->pos[dim_x]) {
        battle(npc);
      }
      world.cur_map->trainer_map[ny][nx] = npc;
      break;
    case char_sentry: // don't move
      break;
    case char_pc: // ignored since it's move npc not pc
      break;
    default:
      default_reached = 1;
      break;
  }
  
  
  if (default_reached) {
    fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
  }
    
}

void next() {
    npc_t *npc = (npc_t *)heap_remove_min(&world.cur_map->trainer_heap);
    move_npc(npc);
    
    npc->next_move += move_cost[npc->type][world.cur_map->map[npc->pos[dim_y]][npc->pos[dim_x]]];
    heap_insert(&world.cur_map->trainer_heap, npc);
}

void print_hiker_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.hiker_dist[y][x] == DIJKSTRA_PATH_MAX) {
        printf("   ");
      } else {
        printf(" %02d", world.hiker_dist[y][x] % 100);
      }
    }
    printf("\n");
  }
}

void print_rival_dist()
{
  int x, y;

  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.rival_dist[y][x] == DIJKSTRA_PATH_MAX ||
          world.rival_dist[y][x] < 0) {
        printf("   ");
      } else {
        printf(" %02d", world.rival_dist[y][x] % 100);
      }
    }
    printf("\n");
  }
}

void enter_building() {
  WINDOW *wbuilding;
  wbuilding = newwin(21, 80, 1, 0);
  box(wbuilding, 0, 0);
  if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_mart) {
    mvwprintw(wbuilding, 1, 1, "You are in a Pokemart. Press < to exit.");
  } else {
    mvwprintw(wbuilding, 1, 1, "You are in a Pokecenter. Press < to exit.");
  }

  int esc = 0;
  do {
    if (wgetch(wbuilding) == '<') {
      esc = 1;
    }
  } while (!esc);
  delwin(wbuilding);
}

void trainer_list() {
  WINDOW *wtrainer;
  int height = 21; int width = 80;
  int npc_y; int npc_x;
  int scroll_pos = 0; int esc = 0;
  wtrainer = newwin(height, width, 1, 0);
  keypad(wtrainer, TRUE);
  set_escdelay(25); // this is so you only need to press esc once

  do {
    wclear(wtrainer);
    box(wtrainer, 0, 0);
    int default_reached = 0;
    int trainers = 4;
    //int total_trainers = 0;
    mvwprintw(wtrainer, 1, (width - 48) / 2, "Welcome to the trainers list. Press ESC to exit.");
    mvwprintw(wtrainer, 2, (width - 20) / 2, "Character | Distance");
    mvwprintw(wtrainer, 3, (width - 20) / 2, "--------------------");
    
    // trainer information
    for (int i = scroll_pos; i <= numtrainers && trainers < height - 1; i++) {
      for (int y = 0; y < MAP_Y; y++) {
        for (int x = 0; x < MAP_X; x++) {
          // trainer symbols & distance
          if (world.cur_map->trainer_map[y][x] != NULL && i == world.cur_map->trainer_map[y][x]->sequence_num) {
            switch(world.cur_map->trainer_map[y][x]->type) {
              case char_hiker:
                mvwaddch(wtrainer, trainers, 71 / 2, HIKER_SYMBOL);
                break;
              case char_rival:
                mvwaddch(wtrainer, trainers, 71 / 2, RIVAL_SYMBOL);
                break;
              case char_pacer:
                mvwaddch(wtrainer, trainers, 71 / 2, PACER_SYMBOL);
                break;
              case char_sentry:
                mvwaddch(wtrainer, trainers, 71 / 2, SENTRY_SYMBOL);
                break;
              case char_wanderer:
                mvwaddch(wtrainer, trainers, 71 / 2, WANDERER_SYMBOL);
                break;
              case char_explorer:
                mvwaddch(wtrainer, trainers, 71 / 2, EXPLORER_SYMBOL);
                break;
              case char_pc:
                break;
              default:
                mvwaddch(wtrainer, trainers, 71 / 2, ERROR_SYMBOL);
                default_reached = 1;
                break;
            }
            // finding distance from pc
            npc_y = world.pc.pos[dim_y] - world.cur_map->trainer_map[y][x]->pos[dim_y];
            npc_x = world.pc.pos[dim_x] - world.cur_map->trainer_map[y][x]->pos[dim_x];
            if (npc_y > 0) { // north
              mvwprintw(wtrainer, trainers, 82 / 2, "%d north", npc_y);
              if (npc_x > 0) {
                mvwprintw(wtrainer, trainers, 100 / 2, "%d east", npc_x);
              }
              else if (npc_x < 0) {
                mvwprintw(wtrainer, trainers, 100 / 2, "%d west", abs(npc_x));
              }
            }
            else if (npc_y < 0) { // south
              mvwprintw(wtrainer, trainers, 82 / 2, "%d south", abs(npc_y));
              if (npc_x > 0) {
                mvwprintw(wtrainer, trainers, 100 / 2, "%d east", npc_x);
              }
              else if (npc_x < 0) {
                mvwprintw(wtrainer, trainers, 100 / 2, "%d west", abs(npc_x));
              }
            }
            else if (npc_x > 0) { // east
              mvwprintw(wtrainer, trainers, 82 / 2, "%d east", npc_x);
            }
            else if (npc_x < 0) { // west
              mvwprintw(wtrainer, trainers, 82 / 2, "%d west", abs(npc_x));
            }

            trainers++;
          }
        }
      }
    }

    wrefresh(wtrainer);
    
    if (default_reached) {
      fprintf(stderr, "Default reached in %s\n", __FUNCTION__);
    }

    int ch = wgetch(wtrainer);
    // checks for scrolling
    switch (ch) {
      case KEY_DOWN:
        if (scroll_pos < numtrainers - (height - 6)) {
          scroll_pos++; // Scroll down
        }
        break;
      case KEY_UP:
        if (scroll_pos > 0) {
          scroll_pos--; // Scroll up
        }
        break;
      case 27:
        esc = 1;
        break;
    }

  } while (!esc);
  delwin(wtrainer);
}

void fly() {
  int x, y;
  int nx, ny;
  // get x coord (between -200 and 200)
  do {
  mvprintw(0, 0, "Please enter the x coordinate (-200 to 200)");
  refresh();
  echo();
  curs_set(1);
  mvscanw(0, 44, "%d", &x);
  } while (x < -200 || x > 200);

  // get y coord (between -200 and 200)
  do {
  mvprintw(0, 0, "Please enter the y coordinate (-200 to 200)");
  refresh();
  echo();
  curs_set(1);
  mvscanw(0, 44, "%d", &y);
  } while (y < -200 || y > 200);

  x += 200;
  y += 200;
  refresh();
  noecho();
  curs_set(0);

  world.cur_idx[dim_x] = x;
  world.cur_idx[dim_y] = y;
  new_map();
  // finds a random path position for the PC to spawn on
  do {
    nx = rand() % (MAP_X - 2) + 1;
    ny = rand() % (MAP_Y - 2) + 1;
  } while (world.cur_map->map[ny][nx] != ter_path);
  world.pc.pos[dim_x] = nx;
  world.pc.pos[dim_y] = ny;

  print_map();
  refresh();
}

int verify_file(const char *file) {
  int check = 0; // acting as a bool to see if name is valid
  const char* file_names[10] = {"pokemon", "moves", "pokemon_moves", "pokemon_species", 
  "experience", "type_names", "pokemon_stats", "stats", "pokemon_types"};
  int i = 0;

  for (i = 0; i < 9; i++) {
    if (strcmp(file, file_names[i]) == 0) {
      check = 1;
      poke_file = i; // initialized as a global parameter
    }
  }

  return check;
}

void print_file(std::vector<std::string> line_v, int vars) {
  for (size_t i = 0; i < line_v.size(); ++i) {
      if (line_v[i] == "INT_MAX") {
        std::cout << " " << " ";
      } else {
        std::cout << line_v[i] << " ";
      }
      if ((i + 1) % vars == 0) { // end of each section end the printing
        std::cout << std::endl;
      }
  }
}

void read_file(std::string path) {
  std::ifstream file(path);
  std::string line;
  const char* file_names[10] = {"pokemon", "moves", "pokemon_moves", "pokemon_species", 
  "experience", "type_names", "pokemon_stats", "stats", "pokemon_types"};
  std::vector<std::string> line_v;

  // checking pokemon.csv
  if (strcmp(file_names[poke_file], "pokemon") == 0) {
    std::vector<pokemon> pokedex1(1093);
    path += "pokemon.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex1[j].id = std::stoi(var); break;
              case 2: strncpy(pokedex1[j].identifier, var.c_str(), sizeof(pokedex1[j].identifier) - 1);
                      pokedex1[j].identifier[sizeof(pokedex1[j].identifier) - 1] = '\0'; break;
              case 3: pokedex1[j].species_id = std::stoi(var); break;
              case 4: pokedex1[j].height = std::stoi(var); break;
              case 5: pokedex1[j].weight = std::stoi(var); break;
              case 6: pokedex1[j].base_expierence = std::stoi(var); break;
              case 7: pokedex1[j].order = std::stoi(var); break;
              case 8: pokedex1[j].is_default = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 8) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex1 entry
      if(j >= (int)(pokedex1.size())) break;
    }
    file.close();
    print_file(line_v, 8);
  }

    // checking moves.csv
  if (strcmp(file_names[poke_file], "moves") == 0) {
    std::vector<moves> pokedex2(845);
    path += "moves.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex2[j].id = std::stoi(var); break;
              case 2: strncpy(pokedex2[j].identifier, var.c_str(), sizeof(pokedex2[j].identifier) - 1);
                      pokedex2[j].identifier[sizeof(pokedex2[j].identifier) - 1] = '\0'; break;
              case 3: pokedex2[j].generation_id = std::stoi(var); break;
              case 4: pokedex2[j].type_id = std::stoi(var); break;
              case 5: pokedex2[j].power = std::stoi(var); break;
              case 6: pokedex2[j].pp = std::stoi(var); break;
              case 7: pokedex2[j].accuracy = std::stoi(var); break;
              case 8: pokedex2[j].priority = std::stoi(var); break;
              case 9: pokedex2[j].target_id = std::stoi(var); break;
              case 10: pokedex2[j].damage_class_id = std::stoi(var); break;
              case 11: pokedex2[j].effect_id = std::stoi(var); break;
              case 12: pokedex2[j].effect_chance = std::stoi(var); break;
              case 13: pokedex2[j].contest_type_id = std::stoi(var); break;
              case 14: pokedex2[j].contest_effect_id = std::stoi(var); break;
              case 15: pokedex2[j].super_contest_effect_id = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 15) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex2 entry
      if(j >= (int)(pokedex2.size())) break;
    }
    file.close();
    print_file(line_v, 15);
  }

  // checking pokemon_moves.csv
  if (strcmp(file_names[poke_file], "pokemon_moves") == 0) {
    std::vector<pokemon_moves> pokedex3(528239);
    path += "pokemon_moves.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex3[j].pokemon_id = std::stoi(var); break;
              case 2: pokedex3[j].version_group_id = std::stoi(var); break;
              case 3: pokedex3[j].move_id = std::stoi(var); break;
              case 4: pokedex3[j].pokemon_move_method_id = std::stoi(var); break;
              case 5: pokedex3[j].level = std::stoi(var); break;
              case 6: pokedex3[j].order = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 6) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex3 entry
      if(j >= (int)(pokedex3.size())) break;
    }
    file.close();
    print_file(line_v, 6);
  }

  // checking pokemon_species.csv
  if (strcmp(file_names[poke_file], "pokemon_species") == 0) {
    std::vector<pokemon_species> pokedex4(899);
    path += "pokemon_species.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex4[j].id = std::stoi(var); break;
              case 2: strncpy(pokedex4[j].identifier, var.c_str(), sizeof(pokedex4[j].identifier) - 1);
                      pokedex4[j].identifier[sizeof(pokedex4[j].identifier) - 1] = '\0'; break;
              case 3: pokedex4[j].generation_id = std::stoi(var); break;
              case 4: pokedex4[j].evolves_from_species_id = std::stoi(var); break;
              case 5: pokedex4[j].evolution_chain_id = std::stoi(var); break;
              case 6: pokedex4[j].color_id = std::stoi(var); break;
              case 7: pokedex4[j].shape_id = std::stoi(var); break;
              case 8: pokedex4[j].habitat_id = std::stoi(var); break;
              case 9: pokedex4[j].gender_rate = std::stoi(var); break;
              case 10: pokedex4[j].capture_rate = std::stoi(var); break;
              case 11: pokedex4[j].base_happiness = std::stoi(var); break;
              case 12: pokedex4[j].is_baby = std::stoi(var); break;
              case 13: pokedex4[j].hatch_counter = std::stoi(var); break;
              case 14: pokedex4[j].has_gender_differences = std::stoi(var); break;
              case 15: pokedex4[j].growth_rate_id = std::stoi(var); break;
              case 16: pokedex4[j].forms_switchable = std::stoi(var); break;
              case 17: pokedex4[j].is_legendary = std::stoi(var); break;
              case 18: pokedex4[j].is_mythical = std::stoi(var); break;
              case 19: pokedex4[j].order = std::stoi(var); break;
              case 20: pokedex4[j].conquest_order = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 20) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex4 entry
      if(j >= (int)(pokedex4.size())) break;
    }
    file.close();
    print_file(line_v, 20);
  }

  // checking experience.csv
  if (strcmp(file_names[poke_file], "experience") == 0) {
    std::vector<experience> pokedex5(601);
    path += "experience.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex5[j].growth_rate_id = std::stoi(var); break;
              case 2: pokedex5[j].level = std::stoi(var); break;
              case 3: pokedex5[j].experience = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 3) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex5 entry
      if(j >= (int)(pokedex5.size())) break;
    }
    file.close();
    print_file(line_v, 3);
  }

  // checking type_names.csv
  if (strcmp(file_names[poke_file], "type_names") == 0) {
    std::vector<type_names> pokedex6(194);
    path += "type_names.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex6[j].type_id = std::stoi(var); break;
              case 2: pokedex6[j].local_language_id = std::stoi(var); break;
              case 3: strncpy(pokedex6[j].name, var.c_str(), sizeof(pokedex6[j].name) - 1);
                      pokedex6[j].name[sizeof(pokedex6[j].name) - 1] = '\0'; break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 3) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex6 entry
      if(j >= (int)(pokedex6.size())) break;
    }
    file.close();
    print_file(line_v, 3);
  }

  // checking pokemon_stats.csv
  if (strcmp(file_names[poke_file], "pokemon_stats") == 0) {
    std::vector<pokemon_stats> pokedex7(6553);
    path += "pokemon_stats.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex7[j].pokemon_id = std::stoi(var); break;
              case 2: pokedex7[j].stat_id = std::stoi(var); break;
              case 3: pokedex7[j].base_stat = std::stoi(var); break;
              case 4: pokedex7[j].effort = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 4) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex7 entry
      if(j >= (int)(pokedex7.size())) break;
    }
    file.close();
    print_file(line_v, 4);
  }

  // checking stats.csv
  if (strcmp(file_names[poke_file], "stats") == 0) {
    std::vector<stats> pokedex8(9);
    path += "stats.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;

      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex8[j].id = std::stoi(var); break;
              case 2: pokedex8[j].damage_class_id = std::stoi(var); break;
              case 3: strncpy(pokedex8[j].identifier, var.c_str(), sizeof(pokedex8[j].identifier) - 1);
                      pokedex8[j].identifier[sizeof(pokedex8[j].identifier) - 1] = '\0'; break;
              case 4: pokedex8[j].is_battle_only = std::stoi(var); break;
              case 5: pokedex8[j].game_index = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            //std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 5) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex8 entry
      if(j >= (int)(pokedex8.size())) break;
    }
    file.close();
    print_file(line_v, 5);
  }

  // checking pokemon_types.csv
  if (strcmp(file_names[poke_file], "pokemon_types") == 0) {
    std::vector<pokemon_types> pokedex9(1677);
    path += "pokemon_types.csv";
    std::ifstream file(path);
    std::string line;
    getline(file, line);
    int i = 1;
    int j = 0;

    // while there's a next line
    while(getline(file, line)) {
      std::stringstream stream_line(line);
      std::string var;
      // when there's a comma check if var is empty or not
      while (getline(stream_line, var, ',')) {
        try {
          // Convert and assign data based on the column
          switch (i) {
              case 1: pokedex9[j].pokemon_id = std::stoi(var); break;
              case 2: pokedex9[j].type_id = std::stoi(var); break;
              case 3: pokedex9[j].slot = std::stoi(var); break;
          }
        } catch (const std::exception& e) {
            // Handle conversion errors or any other exceptions
            std::cerr << "Error processing column " << i << " with value '" << var << "': " << e.what() << '\n';
        }

        if (!var.empty()) {
          line_v.push_back(var);
        } else {
          line_v.push_back("INT_MAX");
        }
        i++;
      }
      if (i == 3) {
        line_v.push_back("INT_MAX");
      }
      i = 1;

      j++; // Move to the next pokedex9 entry
      if(j >= (int)(pokedex9.size())) break;
      
    }
    file.close();
    print_file(line_v, 3);
  }
}

void parse(const char *file) {
  std::string f = file;

  std::string default_p = "/share/cs327/pokedex/pokedex/data/csv/";
  std::string home_p = getenv("HOME");
  home_p += "/.poke327/pokedex/pokedex/data/csv/";

  std::string path = default_p;
  struct stat buffer;

  if (stat(path.c_str(), &buffer) == 0) {
    read_file(path);
    return;
  }

  path = home_p;
  if (stat(path.c_str(), &buffer) == 0) {
    read_file(path);
    return;
  }

  // if neither
  printf("Database not found.\n");
}

int main(int argc, char *argv[])
{
  struct timeval tv;
  uint32_t seed;
  int quit_game = 0;
  int message = 0;
  int default_reached = 0;
  int building = 0;

  // if (argc > 2) { // checking if they put in a --numtrainers command
  //   if (strcmp(argv[1], "--numtrainers") == 0) {
  //     numtrainers = atoi(argv[2]);
  //   }
  // }

  if (argv[1]) {
    if (verify_file(argv[1])) {
      parse(argv[1]);
    } else {
      printf("Not a valid file name.\n");
    }
  }

  return 0;

  if (argc == 2) {
    seed = atoi(argv[1]);
  } else {
    gettimeofday(&tv, NULL);
    seed = (tv.tv_usec ^ (tv.tv_sec << 20)) & 0xffffffff;
  }

  init_term();
  mvprintw(0, 1, "Using seed: %u\n", seed);
  srand(seed);

  init_world();
  init_pc();

  print_map();
  // print_hiker_dist();
  // print_rival_dist();

  do {
    // print statement below used for debugging, can be used to track PC position and map position
    // mvprintw(22, 0, "PC X: %d Y: %d MAP X: %d Y: %d", world.pc.pos[dim_x], world.pc.pos[dim_y], world.cur_idx[dim_x] - 200, world.cur_idx[dim_y] - 200);
    switch(getch()) {
      case 'q':
        quit_game = 1;
        break;
      case '7':
      case 'y':
        move_pc(-1, -1, &message);
        break;
      case '8':
      case 'k':
        move_pc(-1, 0, &message);
        break;
      case '9':
      case 'u':
        move_pc(-1, 1, &message);
        break;
      case '6':
      case 'l':
        move_pc(0, 1, &message);
        break;
      case '3':
      case 'n':
        move_pc(1, 1, &message);
        break;
      case '2':
      case 'j':
        move_pc(1, 0, &message);
        break;
      case '1':
      case 'b':
        move_pc(1, -1, &message);
        break;
      case '4':
      case 'h':
        move_pc(0, -1, &message);
        break;
      case '5':
      case ' ':
      case '.':
        break;
      case '>':
        if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_mart 
        || world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_center) {
          enter_building();
        }
        else {
          building = 1;
        }
        break;
      case 't':
        trainer_list();
        break;
      case 'f':
        fly();
        break;
      default:
        default_reached = 1;
        break;
    }
    next();
    print_map();
    if (message) {
      mvprintw(0, 0, "You can't go there!");
    }
    if (default_reached) {
      mvprintw(0, 0, "Not a valid key.");
    }
    if (building) {
      mvprintw(0, 0, "You aren't on a Pokemart or Pokecenter.");
    }
    building = 0;
    default_reached = 0;
    message = 0;
  } while(!quit_game);
  
  endwin();
  delete_world();

  return 0;
}
