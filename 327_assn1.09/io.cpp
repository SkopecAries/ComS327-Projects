#include <unistd.h>
#include <ncurses.h>
#include <ctype.h>
#include <stdlib.h>
#include <limits.h>

#include "io.h"
#include "character.h"
#include "Assignment1_09.h"
#include "pokemon.h"
#include "db_parse.h"

#define TRAINER_LIST_FIELD_WIDTH 46

typedef struct io_message {
  /* Will print " --more-- " at end of line when another message follows. *
   * Leave 10 extra spaces for that.                                      */
  char msg[71];
  struct io_message *next;
} io_message_t;

static io_message_t *io_head, *io_tail;

void io_init_terminal(void)
{
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

void io_reset_terminal(void)
{
  endwin();

  while (io_head) {
    io_tail = io_head;
    io_head = io_head->next;
    free(io_tail);
  }
  io_tail = NULL;
}

void io_queue_message(const char *format, ...)
{
  io_message_t *tmp;
  va_list ap;

  if (!(tmp = (io_message_t *) malloc(sizeof (*tmp)))) {
    perror("malloc");
    exit(1);
  }

  tmp->next = NULL;

  va_start(ap, format);

  vsnprintf(tmp->msg, sizeof (tmp->msg), format, ap);

  va_end(ap);

  if (!io_head) {
    io_head = io_tail = tmp;
  } else {
    io_tail->next = tmp;
    io_tail = tmp;
  }
}

static void io_print_message_queue(uint32_t y, uint32_t x)
{
  while (io_head) {
    io_tail = io_head;
    attron(COLOR_PAIR(COLOR_CYAN));
    mvprintw(y, x, "%-80s", io_head->msg);
    attroff(COLOR_PAIR(COLOR_CYAN));
    io_head = io_head->next;
    if (io_head) {
      attron(COLOR_PAIR(COLOR_CYAN));
      mvprintw(y, x + 70, "%10s", " --more-- ");
      attroff(COLOR_PAIR(COLOR_CYAN));
      refresh();
      getch();
    }
    free(io_tail);
  }
  io_tail = NULL;
}

/**************************************************************************
 * Compares trainer distances from the PC according to the rival distance *
 * map.  This gives the approximate distance that the PC must travel to   *
 * get to the trainer (doesn't account for crossing buildings).  This is  *
 * not the distance from the NPC to the PC unless the NPC is a rival.     *
 *                                                                        *
 * Not a bug.                                                             *
 **************************************************************************/
static int compare_trainer_distance(const void *v1, const void *v2)
{
  const character *const *c1 = (const character * const *) v1;
  const character *const *c2 = (const character * const *) v2;

  return (world.rival_dist[(*c1)->pos[dim_y]][(*c1)->pos[dim_x]] -
          world.rival_dist[(*c2)->pos[dim_y]][(*c2)->pos[dim_x]]);
}

static character *io_nearest_visible_trainer()
{
  character **c, *n;
  uint32_t x, y, count;

  c = (character **) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = world.cur_map->cmap[y][x];
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  n = c[0];

  free(c);

  return n;
}

void io_display()
{
  uint32_t y, x;
  character *c;

  clear();
  for (y = 0; y < MAP_Y; y++) {
    for (x = 0; x < MAP_X; x++) {
      if (world.cur_map->cmap[y][x]) {
        mvaddch(y + 1, x, world.cur_map->cmap[y][x]->symbol);
      } else {
        switch (world.cur_map->map[y][x]) {
        case ter_boulder:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, BOULDER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_mountain:
          attron(COLOR_PAIR(COLOR_MAGENTA));
          mvaddch(y + 1, x, MOUNTAIN_SYMBOL);
          attroff(COLOR_PAIR(COLOR_MAGENTA));
          break;
        case ter_tree:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, TREE_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_forest:
          attron(COLOR_PAIR(COLOR_GREEN));
          mvaddch(y + 1, x, FOREST_SYMBOL);
          attroff(COLOR_PAIR(COLOR_GREEN));
          break;
        case ter_path:
        case ter_bailey:
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
          attron(COLOR_PAIR(COLOR_BLUE));
          mvaddch(y + 1, x, POKEMART_SYMBOL);
          attroff(COLOR_PAIR(COLOR_BLUE));
          break;
        case ter_center:
          attron(COLOR_PAIR(COLOR_RED));
          mvaddch(y + 1, x, POKEMON_CENTER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_RED));
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
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, WATER_SYMBOL);
          attroff(COLOR_PAIR(COLOR_CYAN));
          break;
        default:
 /* Use zero as an error symbol, since it stands out somewhat, and it's *
  * not otherwise used.                                                 */
          attron(COLOR_PAIR(COLOR_CYAN));
          mvaddch(y + 1, x, ERROR_SYMBOL);
          attroff(COLOR_PAIR(COLOR_CYAN)); 
       }
      }
    }
  }

  mvprintw(23, 1, "PC position is (%2d,%2d) on map %d%cx%d%c.",
           world.pc.pos[dim_x],
           world.pc.pos[dim_y],
           abs(world.cur_idx[dim_x] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_x] - (WORLD_SIZE / 2) >= 0 ? 'E' : 'W',
           abs(world.cur_idx[dim_y] - (WORLD_SIZE / 2)),
           world.cur_idx[dim_y] - (WORLD_SIZE / 2) <= 0 ? 'N' : 'S');
  mvprintw(22, 1, "%d known %s.", world.cur_map->num_trainers,
           world.cur_map->num_trainers > 1 ? "trainers" : "trainer");
  mvprintw(22, 30, "Nearest visible trainer: ");
  if ((c = io_nearest_visible_trainer())) {
    attron(COLOR_PAIR(COLOR_RED));
    mvprintw(22, 55, "%c at vector %d%cx%d%c.",
             c->symbol,
             abs(c->pos[dim_y] - world.pc.pos[dim_y]),
             ((c->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              'N' : 'S'),
             abs(c->pos[dim_x] - world.pc.pos[dim_x]),
             ((c->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              'W' : 'E'));
    attroff(COLOR_PAIR(COLOR_RED));
  } else {
    attron(COLOR_PAIR(COLOR_BLUE));
    mvprintw(22, 55, "NONE.");
    attroff(COLOR_PAIR(COLOR_BLUE));
  }

  io_print_message_queue(0, 0);

  refresh();
}

uint32_t io_teleport_pc(pair_t dest)
{
  /* Just for fun. And debugging.  Mostly debugging. */

  do {
    dest[dim_x] = rand_range(1, MAP_X - 2);
    dest[dim_y] = rand_range(1, MAP_Y - 2);
  } while (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]                  ||
           move_cost[char_pc][world.cur_map->map[dest[dim_y]]
                                                [dest[dim_x]]] ==
             DIJKSTRA_PATH_MAX                                            ||
           world.rival_dist[dest[dim_y]][dest[dim_x]] < 0);

  return 0;
}

static void io_scroll_trainer_list(char (*s)[TRAINER_LIST_FIELD_WIDTH],
                                   uint32_t count)
{
  uint32_t offset;
  uint32_t i;

  offset = 0;

  while (1) {
    for (i = 0; i < 13; i++) {
      mvprintw(i + 6, 19, " %-40s ", s[i + offset]);
    }
    switch (getch()) {
    case KEY_UP:
      if (offset) {
        offset--;
      }
      break;
    case KEY_DOWN:
      if (offset < (count - 13)) {
        offset++;
      }
      break;
    case 27:
      return;
    }

  }
}

static void io_list_trainers_display(npc **c, uint32_t count)
{
  uint32_t i;
  char (*s)[TRAINER_LIST_FIELD_WIDTH]; /* pointer to array of 40 char */

  s = (char (*)[TRAINER_LIST_FIELD_WIDTH]) malloc(count * sizeof (*s));

  mvprintw(3, 19, " %-40s ", "");
  /* Borrow the first element of our array for this string: */
  snprintf(s[0], TRAINER_LIST_FIELD_WIDTH, "You know of %d trainers:", count);
  mvprintw(4, 19, " %-40s ", *s);
  mvprintw(5, 19, " %-40s ", "");

  for (i = 0; i < count; i++) {
    snprintf(s[i], TRAINER_LIST_FIELD_WIDTH, "%16s %c: %2d %s by %2d %s",
             char_type_name[c[i]->ctype],
             c[i]->symbol,
             abs(c[i]->pos[dim_y] - world.pc.pos[dim_y]),
             ((c[i]->pos[dim_y] - world.pc.pos[dim_y]) <= 0 ?
              "North" : "South"),
             abs(c[i]->pos[dim_x] - world.pc.pos[dim_x]),
             ((c[i]->pos[dim_x] - world.pc.pos[dim_x]) <= 0 ?
              "West" : "East"));
    if (count <= 13) {
      /* Handle the non-scrolling case right here. *
       * Scrolling in another function.            */
      mvprintw(i + 6, 19, " %-40s ", s[i]);
    }
  }

  if (count <= 13) {
    mvprintw(count + 6, 19, " %-40s ", "");
    mvprintw(count + 7, 19, " %-40s ", "Hit escape to continue.");
    while (getch() != 27 /* escape */)
      ;
  } else {
    mvprintw(19, 19, " %-40s ", "");
    mvprintw(20, 19, " %-40s ",
             "Arrows to scroll, escape to continue.");
    io_scroll_trainer_list(s, count);
  }

  free(s);
}

static void io_list_trainers()
{
  npc **c;
  uint32_t x, y, count;

  c = (npc **) malloc(world.cur_map->num_trainers * sizeof (*c));

  /* Get a linear list of trainers */
  for (count = 0, y = 1; y < MAP_Y - 1; y++) {
    for (x = 1; x < MAP_X - 1; x++) {
      if (world.cur_map->cmap[y][x] && world.cur_map->cmap[y][x] !=
          &world.pc) {
        c[count++] = dynamic_cast <npc *> (world.cur_map->cmap[y][x]);
      }
    }
  }

  /* Sort it by distance from PC */
  qsort(c, count, sizeof (*c), compare_trainer_distance);

  /* Display it */
  io_list_trainers_display(c, count);
  free(c);

  /* And redraw the map */
  io_display();
}

void io_pokemart()
{
  mvprintw(0, 0, "Welcome to the Pokemart.  Could I interest you in some Pokeballs? *All supplies filled*");
  world.pc.revive = 5;
  world.pc.potion = 10;
  world.pc.pokeball = 10;
  refresh();
  getch();
}

void io_pokemon_center()
{
  for(int i = 0; i < (int)world.pc.char_poke.size(); i++){
    world.pc.char_poke[i].hp = world.pc.char_poke[i].max_hp;
  }
  mvprintw(0, 0, "Welcome to the Pokemon Center.  How can Nurse Joy assist you? *All Pokemon fully revived*");
  refresh();
  getch();
}

void io_list_pokemon(){
  clear();
  mvprintw(1,0,"Pokemon:");

  for(int i = 0; i < (int)world.pc.char_poke.size(); i++){
      if(world.pc.char_poke[i].hp > 0){
        mvprintw(i + 2,0,"(%d) %s: HP: %d/%d",i + 1, world.pc.char_poke[i].name.c_str(), world.pc.char_poke[i].hp, world.pc.char_poke[i].max_hp);
      }else{
        mvprintw(i + 2,0,"(%d) %s: fainted!",i + 1, world.pc.char_poke[i].name.c_str());
      }
  }
}

int io_moves(int move_num, poke *pc_poke, poke *rival_poke) {
  clear();
  mvprintw(0, 0, "%s's HP is: %d/%d & Level: %d", rival_poke->name.c_str(), rival_poke->hp, rival_poke->max_hp, rival_poke->level);
  mvprintw(1, 0, "-----------");
  mvprintw(2, 0, "%s's HP is: %d/%d & Level: %d", pc_poke->name.c_str(), pc_poke->hp, pc_poke->max_hp, pc_poke->level);
  mvprintw(3, 0, "-----------");
  refresh();

  int enemy_move = rand_range(0, rival_poke->move_num - 1);
  bool pc_turn = false;
  bool pc_hit = false;
  bool enemy_hit = false;
  int pc_move_id = pc_poke->poke_moves[move_num].move_id;
  int enemy_move_id = rival_poke->poke_moves[enemy_move].move_id;
  int pc_priority = moves[pc_move_id].priority;
  int enemy_priority = moves[enemy_move_id].priority;

  // determining who goes first
  if (pc_priority == enemy_priority) {
    int pc_speed = pc_poke->speed;
    int enemy_speed = rival_poke->speed;
    if (pc_speed == enemy_speed) {
      if (rand() % 2 == 0) {
        pc_turn = true;
      }
      else {
        pc_turn = false;
      }
    } else if (pc_speed > enemy_speed) {
      pc_turn = true;
    } else {
      pc_turn = false;
    }
  } else if (pc_priority > enemy_priority) {
    pc_turn = true;
  } else {
    pc_turn = false;
  }

  if (pc_turn) {
    mvprintw(4, 0, "%s used %s!", pc_poke->name.c_str(), pc_poke->move_name[move_num].c_str());
    refresh();
    if (rand() % 100 <= moves[pc_move_id].accuracy) {
      pc_hit = true;
    } else {
      pc_hit = false;
    }

    if (pc_hit) {
      double stab = 1.0;
      int type;
      for (int i = 1; i < 1676; i++) {
        if (pokemon_types[i].pokemon_id == pc_poke->Pokemon.id) {
          type = pokemon_types[i].type_id;
        }
      }
      if (moves[pc_move_id].type_id == type) {
        stab = 1.5;
      }
      int power = moves[pc_move_id].power;
      double critical = 1;
      if (rand_range(0, 255) < (pc_poke->speed / 2)) {
        critical = 1.5;
      }
      double random = rand_range(85, 100) / 100.0;
      int damage = (((((((2 * pc_poke->level) / 5) + 2) * power * (pc_poke->attack / pc_poke->defense)) / 50) + 2) * critical * random * stab * 1.0);
      mvprintw(5, 0, "%s hit %s for %d!", pc_poke->name.c_str(), rival_poke->name.c_str(), damage);
      refresh();

      rival_poke->hp -= damage;
      if (rival_poke->hp <= 0) {
        usleep(2000000);
        rival_poke->hp = 0;
        clear();
        mvprintw(0, 0, "%s fainted!", rival_poke->name.c_str());
        refresh();
        usleep(2000000);
        return 2;
      }
    } 
    else {
      mvprintw(5, 0, "%s missed!", pc_poke->name.c_str());
      refresh();
    }
    usleep(2000000);

    mvprintw(6, 0, "%s used %s!", rival_poke->name.c_str(), rival_poke->move_name[enemy_move].c_str());
    refresh();
    if (rand() % 100 <= moves[enemy_move_id].accuracy) {
      enemy_hit = true;
    } else {
      enemy_hit = false;
    }

    if (enemy_hit) {
      double stab = 1.0;
      int type;
      for (int i = 1; i < 1676; i++) {
        if (pokemon_types[i].pokemon_id == rival_poke->Pokemon.id) {
          type = pokemon_types[i].type_id;
        }
      }
      if (moves[enemy_move_id].type_id == type) {
        stab = 1.5;
      }
      int power = moves[enemy_move_id].power;
      double critical = 1;
      if (rand_range(0, 255) < (rival_poke->speed / 2)) {
        critical = 1.5;
      }
      double random = rand_range(85, 100) / 100.0;
      int damage = (((((((2 * rival_poke->level) / 5) + 2) * power * (rival_poke->attack / rival_poke->defense)) / 50) + 2) * critical * random * stab * 1.0);
      mvprintw(7, 0, "%s hit %s for %d!", rival_poke->name.c_str(), pc_poke->name.c_str(), damage);
      refresh();

      pc_poke->hp -= damage;
      if (pc_poke->hp <= 0) {
        usleep(2000000);
        pc_poke->hp = 0;
        clear();
        mvprintw(0, 0, "%s fainted!", pc_poke->name.c_str());
        refresh();
        usleep(2000000);
        return 1;
      }
    }
    else {
      mvprintw(7, 0, "%s missed!", rival_poke->name.c_str());
      refresh();
    }
    usleep(2000000);
  }
  else { // enemy goes first
    mvprintw(4, 0, "%s used %s!", rival_poke->name.c_str(), rival_poke->move_name[enemy_move].c_str());
    refresh();
    if (rand() % 100 <= moves[enemy_move_id].accuracy) {
      enemy_hit = true;
    } else {
      enemy_hit = false;
    }

    if (enemy_hit) {
      double stab = 1.0;
      int type;
      for (int i = 1; i < 1676; i++) {
        if (pokemon_types[i].pokemon_id == rival_poke->Pokemon.id) {
          type = pokemon_types[i].type_id;
        }
      }
      if (moves[enemy_move_id].type_id == type) {
        stab = 1.5;
      }
      int power = moves[enemy_move_id].power;
      double critical = 1;
      if (rand_range(0, 255) < (rival_poke->speed / 2)) {
        critical = 1.5;
      }
      double random = rand_range(85, 100) / 100.0;
      int damage = (((((((2 * rival_poke->level) / 5) + 2) * power * (rival_poke->attack / rival_poke->defense)) / 50) + 2) * critical * random * stab * 1.0);
      mvprintw(5, 0, "%s hit %s for %d!", rival_poke->name.c_str(), pc_poke->name.c_str(), damage);
      refresh();

      pc_poke->hp -= damage;
      if (pc_poke->hp <= 0) {
        usleep(2000000);
        pc_poke->hp = 0;
        clear();
        mvprintw(0, 0, "%s fainted!", pc_poke->name.c_str());
        refresh();
        usleep(2000000);
        return 1;
      }
    }
    else {
      mvprintw(5, 0, "%s missed!", rival_poke->name.c_str());
      refresh();
    }
    usleep(2000000);

    mvprintw(6, 0, "%s used %s!", pc_poke->name.c_str(), pc_poke->move_name[move_num].c_str());
    refresh();
    if (rand() % 100 <= moves[pc_move_id].accuracy) {
      pc_hit = true;
    } else {
      pc_hit = false;
    }

    if (pc_hit) {
      double stab = 1.0;
      int type;
      for (int i = 1; i < 1676; i++) {
        if (pokemon_types[i].pokemon_id == pc_poke->Pokemon.id) {
          type = pokemon_types[i].type_id;
        }
      }
      if (moves[pc_move_id].type_id == type) {
        stab = 1.5;
      }
      int power = moves[pc_move_id].power;
      double critical = 1;
      if (rand_range(0, 255) < (pc_poke->speed / 2)) {
        critical = 1.5;
      }
      double random = rand_range(85, 100) / 100.0;
      int damage = (((((((2 * pc_poke->level) / 5) + 2) * power * (pc_poke->attack / pc_poke->defense)) / 50) + 2) * critical * random * stab * 1.0);
      mvprintw(7, 0, "%s hit %s for %d!", pc_poke->name.c_str(), rival_poke->name.c_str(), damage);
      refresh();

      rival_poke->hp -= damage;
      if (rival_poke->hp <= 0) {
        usleep(2000000);
        rival_poke->hp = 0;
        clear();
        mvprintw(0, 0, "%s fainted!", rival_poke->name.c_str());
        refresh();
        usleep(2000000);
        return 2;
      }
    } 
    else {
      mvprintw(7, 0, "%s missed!", pc_poke->name.c_str());
      refresh();
    }
    usleep(2000000);
  }
  return 0;
}

int io_fight(poke *pc_poke, poke *rival_poke) {
  int esc = 0;
  // const int ESCAPE = 27;
  clear();
  do {
    int move = 0;
    clear();
    mvprintw(0, 0, "Select a move:\n(1) %s lvl: %d", pc_poke->move_name[0].c_str(), pc_poke->poke_moves[0].level);
    if (pc_poke->move_num == 2) {
        mvprintw(2,0, "(2) %s lvl: %d", pc_poke->move_name[1].c_str(), pc_poke->poke_moves[1].level);
    }
    mvprintw(3, 0, "-----------");
    mvprintw(4, 0, "%s's HP is: %d/%d & Level: %d", rival_poke->name.c_str(), rival_poke->hp, rival_poke->max_hp, rival_poke->level);
    mvprintw(5, 0, "-----------");
    mvprintw(6, 0, "%s's HP is: %d/%d & Level: %d", pc_poke->name.c_str(), pc_poke->hp, pc_poke->max_hp, pc_poke->level);
    refresh();
    switch(getch()) {
      // case ESCAPE:
      //   esc = 1;
      //   break;
      case '1':
        move = io_moves(0, pc_poke, rival_poke);
        if (move == 1) {
          int i;
          for(i = 0; i < (int)world.pc.char_poke.size(); i++){
            if(world.pc.char_poke[i].hp > 0){
              break;
            }
          }
          
          clear();
          if (i == (int)world.pc.char_poke.size()) {
            mvprintw(0, 0, "All of you pokemon have fainted! Better luck next time..");
            refresh();
            usleep(2000000);
            return 6;
            esc = 1;
          } else {
            mvprintw(1, 0, "Your pokemon has fainted! Time to pick a new one..");
            refresh();
            usleep(2000000);
            return i;
            esc = 1;
          }
        } else if (move == 2) {
          return 7;
          esc = 1;
        }
        break;
      case '2':
        move = io_moves(1, pc_poke, rival_poke);
        if (move == 1) {
          int i;
          for(i = 0; i < (int)world.pc.char_poke.size(); i++){
            if(world.pc.char_poke[i].hp > 0){
              break;
            }
          }
          
          clear();
          if (i == (int)world.pc.char_poke.size()) {
            mvprintw(0, 0, "All of you pokemon have fainted! Better luck next time..");
            refresh();
            usleep(2000000);
            return 6;
            esc = 1;
          } else {
            mvprintw(0, 0, "Your pokemon has fainted! Time to pick a new one..");
            refresh();
            usleep(2000000);
            return i;
          }
        } else if (move == 2) {
          return 7;
          esc = 1;
        }
        break;
    }
  } while (!esc);
  refresh();
  return -1;

}

int io_bag(int wild) {
  clear();
  int esc = 0;
  const int ESCAPE = 27;
  do {
    int pick_revive = 0;
    int pick_potion = 0;
    clear();
    mvprintw(0, 0, "BAG\nHere are you items, select which to use or press ESC:");
    mvprintw(2, 0, "(1) Revives: %d\n(2) Potions: %d\n(3) Pokeballs: %d", world.pc.revive, world.pc.potion, world.pc.pokeball);
    refresh();
    switch(getch()) {
      case ESCAPE:
        esc = 1;
        break;
      case '1':
        if (world.pc.revive > 0) {
          pick_revive = 1;
          while (pick_revive) {
            clear();
            io_list_pokemon();
            mvprintw(0, 0, "Select the pokemon you'd like to revive.");
            refresh();
            switch(getch()) {
              case ESCAPE:
                pick_revive = 0;
                break;
              case '1':
                if (world.pc.char_poke[0].hp > 0) {
                  mvprintw(10, 0, "Pokemon must have zero HP to revive.");
                  refresh();
                  usleep(2000000);
                } else {
                  world.pc.char_poke[0].hp += world.pc.char_poke[0].max_hp / 2;
                  world.pc.revive--;
                }
                break;
              case '2':
                if ((int)world.pc.char_poke.size() >= 2) {
                  if (world.pc.char_poke[1].hp > 0) {
                  mvprintw(10, 0, "Pokemon must have zero HP to revive.");
                  refresh();
                  usleep(2000000);
                  } else {
                    world.pc.char_poke[1].hp += world.pc.char_poke[1].max_hp / 2;
                    world.pc.revive--;
                  }
                } else {
                  mvprintw(10, 0, "Invalid Pokemon.");
                  refresh();
                  usleep(2000000);
                }
              break;
            case '3':
              if ((int)world.pc.char_poke.size() >= 3) {
                if (world.pc.char_poke[2].hp > 0) {
                mvprintw(10, 0, "Pokemon must have zero HP to revive.");
                refresh();
                usleep(2000000);
                } else {
                  world.pc.char_poke[2].hp += world.pc.char_poke[2].max_hp / 2;
                  world.pc.revive--;
                }
              } else {
                mvprintw(10, 0, "Invalid Pokemon.");
                refresh();
                usleep(2000000);
              }
              break;
            case '4':
              if ((int)world.pc.char_poke.size() >= 4) {
                if (world.pc.char_poke[3].hp > 0) {
                mvprintw(10, 0, "Pokemon must have zero HP to revive.");
                refresh();
                usleep(2000000);
                } else {
                  world.pc.char_poke[3].hp += world.pc.char_poke[3].max_hp / 2;
                  world.pc.revive--;
                }
              } else {
                mvprintw(10, 0, "Invalid Pokemon.");
                refresh();
                usleep(2000000);
              }
              break;
            case '5':
              if ((int)world.pc.char_poke.size() >= 5) {
                if (world.pc.char_poke[4].hp > 0) {
                mvprintw(10, 0, "Pokemon must have zero HP to revive.");
                refresh();
                usleep(2000000);
                } else {
                  world.pc.char_poke[4].hp += world.pc.char_poke[4].max_hp / 2;
                  world.pc.revive--;
                }
              } else {
                mvprintw(10, 0, "Invalid Pokemon.");
                refresh();
                usleep(2000000);
              }
              break;
            case '6':
              if ((int)world.pc.char_poke.size() == 6) {
                if (world.pc.char_poke[5].hp > 0) {
                mvprintw(10, 0, "Pokemon must have zero HP to revive.");
                refresh();
                usleep(2000000);
                } else {
                  world.pc.char_poke[5].hp += world.pc.char_poke[5].max_hp / 2;
                  world.pc.revive--;
                }
              } else {
                mvprintw(10, 0, "Invalid Pokemon.");
                refresh();
                usleep(2000000);
              }
              break;
            default:
              mvprintw(10, 0, "Invalid Pokemon.");
              refresh();
              usleep(2000000);
              break;
            }
          }
        } else {
          clear();
          mvprintw(0, 0, "You don't have enough revives! Go to a PokeMart for more!");
          refresh();
          usleep(2000000);
        }
        break;
      case '2':
        if (world.pc.potion > 0) {
          pick_potion = 1;
          while (pick_potion) {
            clear();
            io_list_pokemon();
            mvprintw(0, 0, "Select the pokemon you'd like to heal.");
            refresh();
            switch(getch()) {
              case ESCAPE:
                pick_potion = 0;
                break;
              case '1':
                if (world.pc.char_poke[0].hp == world.pc.char_poke[0].max_hp) {
                  mvprintw(10, 0, "Your Pokemon has full HP! It doesn't need a potion!");
                  refresh();
                  usleep(2000000);
                } else {
                  if (world.pc.char_poke[0].max_hp - world.pc.char_poke[0].hp >= 20) {
                    world.pc.char_poke[0].hp += 20;
                  } else {
                    world.pc.char_poke[0].hp = world.pc.char_poke[0].max_hp;
                  }
                  world.pc.potion--;
                }
                break;
              case '2':
                if ((int)world.pc.char_poke.size() >= 2) {
                  if (world.pc.char_poke[1].hp == world.pc.char_poke[1].max_hp) {
                    mvprintw(10, 0, "Your Pokemon has full HP! It doesn't need a potion!");
                    refresh();
                    usleep(2000000);
                  } else {
                    if (world.pc.char_poke[1].max_hp - world.pc.char_poke[1].hp >= 20) {
                      world.pc.char_poke[1].hp += 20;
                    } else {
                      world.pc.char_poke[1].hp = world.pc.char_poke[1].max_hp;
                    }
                    world.pc.potion--;
                  }
                } else {
                  mvprintw(10, 0, "Invalid Pokemon.");
                  refresh();
                  usleep(2000000);
                }
              break;
            case '3':
              if ((int)world.pc.char_poke.size() >= 3) {
                if (world.pc.char_poke[2].hp == world.pc.char_poke[2].max_hp) {
                  mvprintw(10, 0, "Your Pokemon has full HP! It doesn't need a potion!");
                  refresh();
                  usleep(2000000);
                } else {
                  if (world.pc.char_poke[2].max_hp - world.pc.char_poke[2].hp >= 20) {
                    world.pc.char_poke[2].hp += 20;
                  } else {
                    world.pc.char_poke[2].hp = world.pc.char_poke[2].max_hp;
                  }
                  world.pc.potion--;
                }
              } else {
                  mvprintw(10, 0, "Invalid Pokemon.");
                  refresh();
                  usleep(2000000);
                }
              break;
            case '4':
              if ((int)world.pc.char_poke.size() >= 4) {
                if (world.pc.char_poke[3].hp == world.pc.char_poke[3].max_hp) {
                  mvprintw(10, 0, "Your Pokemon has full HP! It doesn't need a potion!");
                  refresh();
                  usleep(2000000);
                } else {
                  if (world.pc.char_poke[3].max_hp - world.pc.char_poke[3].hp >= 20) {
                    world.pc.char_poke[3].hp += 20;
                  } else {
                    world.pc.char_poke[3].hp = world.pc.char_poke[3].max_hp;
                  }
                  world.pc.potion--;
                }
              } else {
                  mvprintw(10, 0, "Invalid Pokemon.");
                  refresh();
                  usleep(2000000);
                }
              break;
            case '5':
              if ((int)world.pc.char_poke.size() >= 5) {
                if (world.pc.char_poke[4].hp == world.pc.char_poke[4].max_hp) {
                  mvprintw(10, 0, "Your Pokemon has full HP! It doesn't need a potion!");
                  refresh();
                  usleep(2000000);
                } else {
                  if (world.pc.char_poke[4].max_hp - world.pc.char_poke[4].hp >= 20) {
                    world.pc.char_poke[4].hp += 20;
                  } else {
                    world.pc.char_poke[4].hp = world.pc.char_poke[4].max_hp;
                  }
                  world.pc.potion--;
                }
              } else {
                  mvprintw(10, 0, "Invalid Pokemon.");
                  refresh();
                  usleep(2000000);
                }
              break;
            case '6':
              if ((int)world.pc.char_poke.size() == 6) {
                if (world.pc.char_poke[5].hp == world.pc.char_poke[5].max_hp) {
                  mvprintw(10, 0, "Your Pokemon has full HP! It doesn't need a potion!");
                  refresh();
                  usleep(2000000);
                } else {
                  if (world.pc.char_poke[5].max_hp - world.pc.char_poke[5].hp >= 20) {
                    world.pc.char_poke[5].hp += 20;
                  } else {
                    world.pc.char_poke[5].hp = world.pc.char_poke[5].max_hp;
                  }
                  world.pc.potion--;
                }
              } else {
                  mvprintw(10, 0, "Invalid Pokemon.");
                  refresh();
                  usleep(2000000);
                }
              break;
            default:
              mvprintw(10, 0, "Invalid Pokemon.");
              refresh();
              usleep(2000000);
              break;
            }
          }
        } else {
          clear();
          mvprintw(0, 0, "You don't have enough potions! Go to a PokeMart for more!");
          refresh();
          usleep(2000000);
        }
        break;
      case '3':
        if (world.pc.pokeball > 0) {
          if (wild) {
            world.pc.pokeball--;
            clear();
            mvprintw(0, 0, "Throwing pokeball..");
            refresh();
            usleep(2000000);
            return 1;
            esc = 1;
          } else {
            clear();
            mvprintw(0, 0, "You must be in a battle with a wild poke OR they must be ready to catch!");
            refresh();
            usleep(2000000);
          }
        } else {
          clear();
          mvprintw(0, 0, "You don't have enough pokeballs! Go to a PokeMart for more!");
          refresh();
          usleep(2000000);
          return 2;
        }
        break;
      default:
        mvprintw(5, 0, "Invalid key. Press 1 for revive, 2 for potion, 3 for pokeball, or ESC to exit.");
        refresh();
        usleep(2000000);
        break;
    }
  } while(!esc);
  if (wild) {
    return 2;
  }
  return 0;
}

int io_swap_pokemon() {
  int esc = 0;
  do{
    clear();
    io_list_pokemon();
    mvprintw(0, 0, "Which Pokemon should battle?");
    refresh();
    switch(getch()) {
      case '1':
        if (world.pc.char_poke[0].hp > 0) {
          return 0;
          esc = 1;
        } else {
          mvprintw(10, 0, "Pokemon has fainted! It can't battle!");
          refresh();
          usleep(2000000);
        }
        break;
      case '2':
        if ((int)world.pc.char_poke.size() >= 2) {
          if (world.pc.char_poke[1].hp > 0) {
            return 1;
            esc = 1;
          } else {
            mvprintw(10, 0, "Pokemon has fainted! It can't battle!");
            refresh();
            usleep(2000000);
          }
        } else {
          mvprintw(10, 0, "Invalid Pokemon.");
          refresh();
          usleep(2000000);
        }
        break;
      case '3':
        if ((int)world.pc.char_poke.size() >= 3) {
          if (world.pc.char_poke[2].hp > 0) {
            return 2;
            esc = 1;
          } else {
            mvprintw(10, 0, "Pokemon has fainted! It can't battle!");
            refresh();
            usleep(2000000);
          }
        } else {
          mvprintw(10, 0, "Invalid Pokemon.");
          refresh();
          usleep(2000000);
        }
        break;
      case '4':
        if ((int)world.pc.char_poke.size() >= 4) {
          if (world.pc.char_poke[3].hp > 0) {
            return 3;
            esc = 1;
          } else {
            mvprintw(10, 0, "Pokemon has fainted! It can't battle!");
            refresh();
            usleep(2000000);
          }
        } else {
          mvprintw(10, 0, "Invalid Pokemon.");
          refresh();
          usleep(2000000);
        }
        break;
      case '5':
        if ((int)world.pc.char_poke.size() >= 5) {
          if (world.pc.char_poke[4].hp > 0) {
            return 4;
            esc = 1;
          } else {
            mvprintw(10, 0, "Pokemon has fainted! It can't battle!");
            refresh();
            usleep(2000000);
          }
        } else {
          mvprintw(10, 0, "Invalid Pokemon.");
          refresh();
          usleep(2000000);
        }
        break;
      case '6':
        if ((int)world.pc.char_poke.size() == 6) {
          if (world.pc.char_poke[5].hp > 0) {
            return 5;
            esc = 1;
          } else {
            mvprintw(10, 0, "Pokemon has fainted! It can't battle!");
            refresh();
            usleep(2000000);
          }
        } else {
          mvprintw(10, 0, "Invalid Pokemon.");
          refresh();
          usleep(2000000);
        }
        break;
      default:
        mvprintw(10, 0, "Invalid Pokemon.");
        refresh();
        usleep(2000000);
        break;
    }
  } while (!esc);
  return 0;
}

void io_battle(character *aggressor, character *defender)
{
  npc *n = (npc *) ((aggressor == &world.pc) ? defender : aggressor);

  io_display();
  clear();
  int esc = 0;
  // const int ESCAPE = 27;
  int curr = -1;
  int n_curr = -1;
  int i = 0;
  for (i = 0; i < (int)world.pc.char_poke.size(); i++) {
    if (world.pc.char_poke[i].hp > 0) {
      curr = i;
      break;
    }
  }
  if (curr == -1) {
    return;
  }

  for (i = 0; i < (int)n->char_poke.size(); i++) {
    if (n->char_poke[i].hp > 0) {
      n_curr = i;
      break;
    }
  }
  if (n_curr == -1) {
    return;
  }

  
  do {
    int fight = 0;
    clear();
    mvprintw(0, 0, "You're in a battle with a %s!\nThey have %d Pokemon.\n%s sent out %s!",
    char_type_name[n->ctype], (int)n->char_poke.size(), char_type_name[n->ctype], n->char_poke[n_curr].name.c_str());
    mvprintw(3, 0, "%s's HP is: %d/%d & Level: %d", n->char_poke[n_curr].name.c_str(), n->char_poke[n_curr].hp, n->char_poke[n_curr].max_hp, n->char_poke[n_curr].level);
    mvprintw(4, 0, "-----------");
    mvprintw(5, 0, "Go! %s!", world.pc.char_poke[curr].name.c_str());
    mvprintw(6, 0, "%s's HP is: %d/%d & Level: %d", world.pc.char_poke[curr].name.c_str(), world.pc.char_poke[curr].hp, world.pc.char_poke[curr].max_hp, world.pc.char_poke[curr].level);
    mvprintw(7, 0, "-----------");
    mvprintw(8, 0, "(1) FIGHT\n(2) BAG\n(3) POKEMON");
    refresh();
    switch(getch()) {
      // case ESCAPE:
      //   esc = 1;
      //   break;
      case '1':
        fight = io_fight(&world.pc.char_poke[curr], &n->char_poke[n_curr]);
        if (fight >= 0 && fight < 6) {
          curr = fight;
        } else if (fight == 7) {
          n_curr = -1;
          for (i = 0; i < (int)n->char_poke.size(); i++) {
            if (n->char_poke[i].hp > 0) {
              n_curr = i;
              break;
            }
          }

          if (n_curr == -1) {
            return;
          } else if (i == 6) {
            clear();
            mvprintw(0, 0, "All enemy pokemon have been defeated.");
            refresh();
            usleep(2000000);
            n->defeated = 1;
            if (n->ctype == char_hiker || n->ctype == char_rival) {
              n->mtype = move_wander;
            }
            esc = 1;
          }
        } else {
          esc = 1;
        }
        break;
      case '2':
        io_bag(0);
        break;
      case '3':
        curr = io_swap_pokemon();
        break;
      default:
        mvprintw(11, 0, "Invalid key. Press 1 for fight, 2 for bag, or 3 to swap pokemon.");
        refresh();
        usleep(2000000);
        break;
    }
  } while (!esc);

  refresh();
}

void io_pokemon() {
  clear();
  poke Poke = poke();
  int esc = 0;
  // const int ESCAPE = 27;
  int curr = -1;
  int i = 0;
  for (i = 0; i < (int)world.pc.char_poke.size(); i++) {
    if (world.pc.char_poke[i].hp > 0) {
      curr = i;
      break;
    }
  }
  if (curr == -1) {
    return;
  }

  
  do {
    int fight = 0;
    int pokeball = 0;
    clear();
    mvprintw(0, 0, "A wild %s has appeared!", Poke.name.c_str());
    mvprintw(1, 0, "%s's HP is: %d/%d & Level: %d", Poke.name.c_str(), Poke.hp, Poke.max_hp, Poke.level);
    mvprintw(2, 0, "-----------");
    mvprintw(3, 0, "Go! %s!", world.pc.char_poke[curr].name.c_str());
    mvprintw(4, 0, "%s's HP is: %d/%d & Level: %d", world.pc.char_poke[curr].name.c_str(), world.pc.char_poke[curr].hp, world.pc.char_poke[curr].max_hp, world.pc.char_poke[curr].level);
    mvprintw(5, 0, "-----------");
    mvprintw(6, 0, "(1) FIGHT\n(2) BAG\n(3) RUN\n(4) POKEMON");
    refresh();
    switch(getch()) {
      // case ESCAPE:
      //   esc = 1;
      //   break;
      case '1':
        fight = io_fight(&world.pc.char_poke[curr], &Poke);
        if (fight >= 0 && fight < 6) {
          curr = fight;
        } else if (fight == 7) {
          int enter_bag = 1;
          while (enter_bag) {
            clear();
            mvprintw(0, 0, "Pokemon %s is ready to catch. Would you like to enter your bag to grab a pokeball? (1 for yes, 2 for no)", Poke.name.c_str());
            refresh();
            switch(getch()) {
              case '1':
                pokeball = io_bag(1);
                if (pokeball == 1) {
                  if ((int)world.pc.char_poke.size() < 6) {
                    world.pc.char_poke.push_back(Poke);
                  } else {
                    world.pc.pokeball++; // adding back the pokeball since we can't catch a pokemon
                  }

                  clear();
                  mvprintw(0, 0, "Pokemon %s will be added to your collection if there's space.", Poke.name.c_str());
                  refresh();
                  usleep(2000000);
                  enter_bag = 0;
                } else {
                  clear();
                  mvprintw(0, 0, "Exiting battle.");
                  refresh();
                  usleep(2000000);
                  enter_bag = 0;
                }
                break;
              case '2':
                mvprintw(1, 0, "Ok, exiting battle.");
                refresh();
                usleep(2000000);
                enter_bag = 0;
                break;
              default:
                mvprintw(1, 0, "Please press 1 to enter your bag, or 2 to exit the battle.");
                refresh();
                usleep(2000000);
                break;
            }
          }
          esc = 1;
        } else {
          esc = 1;
        }
        break;
      case '2':
        io_bag(0);
        break;
      case '3':
        if (rand() % 50 == 0) {
          clear();
          mvprintw(0, 0, "You have failed to run!");
          refresh();
          usleep(2000000);
        } else {
          clear();
          mvprintw(0, 0, "You have successfully ran away!");
          refresh();
          usleep(2000000);
          esc = 1;
        }
        break;
      case '4':
        curr = io_swap_pokemon();
        break;
      default:
        mvprintw(10, 0, "Invalid key. Press 1 for fight, 2 for bag, 3 to run, or 4 to swap pokemon.");
        refresh();
        usleep(2000000);
        break;
    }
  } while (!esc);
  refresh();
  
}

uint32_t move_pc_dir(uint32_t input, pair_t dest)
{
  dest[dim_y] = world.pc.pos[dim_y];
  dest[dim_x] = world.pc.pos[dim_x];

  switch (input) {
  case 1:
  case 2:
  case 3:
    dest[dim_y]++;
    break;
  case 4:
  case 5:
  case 6:
    break;
  case 7:
  case 8:
  case 9:
    dest[dim_y]--;
    break;
  }
  switch (input) {
  case 1:
  case 4:
  case 7:
    dest[dim_x]--;
    break;
  case 2:
  case 5:
  case 8:
    break;
  case 3:
  case 6:
  case 9:
    dest[dim_x]++;
    break;
  case '>':
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_mart) {
      io_pokemart();
    }
    if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] ==
        ter_center) {
      io_pokemon_center();
    }
    break;
  }

  if (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) {
    if (dynamic_cast<npc *> (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]) &&
        ((npc *) world.cur_map->cmap[dest[dim_y]][dest[dim_x]])->defeated) {
      // Some kind of greeting here would be nice
      return 1;
    } else if ((dynamic_cast<npc *>
                (world.cur_map->cmap[dest[dim_y]][dest[dim_x]]))) {
      io_battle(&world.pc, world.cur_map->cmap[dest[dim_y]][dest[dim_x]]);
      // Not actually moving, so set dest back to PC position
      dest[dim_x] = world.pc.pos[dim_x];
      dest[dim_y] = world.pc.pos[dim_y];
    }
  }
  
  if (move_cost[char_pc][world.cur_map->map[dest[dim_y]][dest[dim_x]]] ==
      DIJKSTRA_PATH_MAX) {
    return 1;
  }

  // player is in long grass
  if (world.cur_map->map[world.pc.pos[dim_y]][world.pc.pos[dim_x]] == ter_grass) {
    if (rand() % 10 == 0) {
      io_pokemon();
    }
  }

  return 0;
}

void io_teleport_world(pair_t dest)
{
  /* mvscanw documentation is unclear about return values.  I believe *
   * that the return value works the same way as scanf, but instead   *
   * of counting on that, we'll initialize x and y to out of bounds   *
   * values and accept their updates only if in range.                */
  int x = INT_MAX, y = INT_MAX;
  
  world.cur_map->cmap[world.pc.pos[dim_y]][world.pc.pos[dim_x]] = NULL;

  echo();
  curs_set(1);
  do {
    mvprintw(0, 0, "Enter x [-200, 200]:           ");
    refresh();
    mvscanw(0, 21, (char *) "%d", &x);
  } while (x < -200 || x > 200);
  do {
    mvprintw(0, 0, "Enter y [-200, 200]:          ");
    refresh();
    mvscanw(0, 21, (char *) "%d", &y);
  } while (y < -200 || y > 200);

  refresh();
  noecho();
  curs_set(0);

  x += 200;
  y += 200;

  world.cur_idx[dim_x] = x;
  world.cur_idx[dim_y] = y;

  new_map(1);
  io_teleport_pc(dest);
}

void io_handle_input(pair_t dest)
{
  uint32_t turn_not_consumed;
  int key;

  do {
    switch (key = getch()) {
    case '7':
    case 'y':
    case KEY_HOME:
      turn_not_consumed = move_pc_dir(7, dest);
      break;
    case '8':
    case 'k':
    case KEY_UP:
      turn_not_consumed = move_pc_dir(8, dest);
      break;
    case '9':
    case 'u':
    case KEY_PPAGE:
      turn_not_consumed = move_pc_dir(9, dest);
      break;
    case '6':
    case 'l':
    case KEY_RIGHT:
      turn_not_consumed = move_pc_dir(6, dest);
      break;
    case '3':
    case 'n':
    case KEY_NPAGE:
      turn_not_consumed = move_pc_dir(3, dest);
      break;
    case '2':
    case 'j':
    case KEY_DOWN:
      turn_not_consumed = move_pc_dir(2, dest);
      break;
    case '1':
    case 'b':
    case KEY_END:
      turn_not_consumed = move_pc_dir(1, dest);
      break;
    case '4':
    case 'h':
    case KEY_LEFT:
      turn_not_consumed = move_pc_dir(4, dest);
      break;
    case '5':
    case ' ':
    case '.':
    case KEY_B2:
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    case '>':
      turn_not_consumed = move_pc_dir('>', dest);
      break;
    case 'Q':
      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      world.quit = 1;
      turn_not_consumed = 0;
      break;
      break;
    case 'B':
      io_bag(0);
      clear();
      io_display();
      refresh();
      turn_not_consumed = 1;
      break;
    case 't':
      io_list_trainers();
      turn_not_consumed = 1;
      break;
    case 'p':
      /* Teleport the PC to a random place in the map.              */
      io_teleport_pc(dest);
      turn_not_consumed = 0;
      break;
   case 'f':
      /* Fly to any map in the world.                                */
      io_teleport_world(dest);
      turn_not_consumed = 0;
      break;    
    case 'q':
      /* Demonstrate use of the message queue.  You can use this for *
       * printf()-style debugging (though gdb is probably a better   *
       * option.  Not that it matters, but using this command will   *
       * waste a turn.  Set turn_not_consumed to 1 and you should be *
       * able to figure out why I did it that way.                   */
      io_queue_message("This is the first message.");
      io_queue_message("Since there are multiple messages, "
                       "you will see \"more\" prompts.");
      io_queue_message("You can use any key to advance through messages.");
      io_queue_message("Normal gameplay will not resume until the queue "
                       "is empty.");
      io_queue_message("Long lines will be truncated, not wrapped.");
      io_queue_message("io_queue_message() is variadic and handles "
                       "all printf() conversion specifiers.");
      io_queue_message("Did you see %s?", "what I did there");
      io_queue_message("When the last message is displayed, there will "
                       "be no \"more\" prompt.");
      io_queue_message("Have fun!  And happy printing!");
      io_queue_message("Oh!  And use 'Q' to quit!");

      dest[dim_y] = world.pc.pos[dim_y];
      dest[dim_x] = world.pc.pos[dim_x];
      turn_not_consumed = 0;
      break;
    default:
      /* Also not in the spec.  It's not always easy to figure out what *
       * key code corresponds with a given keystroke.  Print out any    *
       * unhandled key here.  Not only does it give a visual error      *
       * indicator, but it also gives an integer value that can be used *
       * for that key in this (or other) switch statements.  Printed in *
       * octal, with the leading zero, because ncurses.h lists codes in *
       * octal, thus allowing us to do reverse lookups.  If a key has a *
       * name defined in the header, you can use the name here, else    *
       * you can directly use the octal value.                          */
      mvprintw(0, 0, "Unbound key: %#o ", key);
      turn_not_consumed = 1;
    }
    refresh();
  } while (turn_not_consumed);
}
