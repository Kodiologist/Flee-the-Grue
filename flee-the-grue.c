/* -*- C -*- */

#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <termios.h>
#include <glib.h>

#include "memory.c"

#include "definitions.h"

// ------------------------------------------------------------
// Main
// ------------------------------------------------------------

int main(int argc, char **argv)
 {munge_terminal();
  buffer[0] = 0;
  buffer_size = 0;

  bool play_again;
  for (;;)
    {setup();
     play_again = play();
     if (play_again)
        {puts(M_RESTART);
         getchar();}
     else break;}

  unmunge_terminal();
  assert_mem();
  return 0;}

// ------------------------------------------------------------
// Setup
// ------------------------------------------------------------

void setup(void)
/* Sets up a new game. */
 {wumpus_alive = true;
  grue_stun = 0;
  arrows = STARTING_ARROWS;
  
  // We'll make a hash of rooms from which to select starting
  // positions for the various objects. The rooms are stored
  // as keys; we'll ignore the values.
  GHashTable *rooms = g_hash_table_new_full
     ((GHashFunc) point_hash, (GEqualFunc) point_eq,
      kfree, NULL);
  for (nat x = 0 ; x <= MAX_X ; ++x)
      for (nat y = 0 ; y <= MAX_Y ; ++y)
         {point *p = kmalloc( sizeof(point) );
          p->x = x; p->y = y;
          g_hash_table_insert(rooms, p, NULL);}

  //player.x = 0; player.y = 1;
  //wumpus.x = 1; wumpus.y = 3;
  //grue.x = 1; grue.y = 3;
  //the_exit.x = 5; the_exit.y = 5;
  add_to_map(rooms, &player);
  add_to_map(rooms, &wumpus);
  add_to_map(rooms, &grue);
  add_to_map(rooms, &the_exit);

  grue_before = r_connected(&grue);

  g_hash_table_destroy(rooms);}

guint point_hash(void *p)
/* Creates a hash value for the given point. */
 {return ((point *)p)->y * (MAX_X+1) + ((point *)p)->x + 1;}

gboolean point_eq(void *a, void *b)
/* Checks two points for equality. */
 {return ((point *)a)->x == ((point *)b)->x &&
         ((point *)a)->y == ((point *)b)->y
    ? TRUE : FALSE;}

void add_to_map(GHashTable *rooms, point *obj)
/* Sets obj to a random room, and removes from rooms that point
and every other one within three moves. */
 {*obj = *r_point(rooms);
  g_hash_table_remove(rooms, obj);

  GHashTableIter i;
  void *p;
  GHashTable *nearby = rooms_nearby(obj);
  g_hash_table_iter_init(&i, nearby);
  while ( g_hash_table_iter_next(&i, &p, NULL) )
      g_hash_table_remove(rooms, p);
  g_hash_table_destroy(nearby);}

// ------------------------------------------------------------
// Gameplay
// ------------------------------------------------------------

bool play(void)
/* The main loop of the game. */
 {cmd c;
  for (;;)
     {describe_surroundings();
      c = get_command();
      switch (c.type)
         {case WAIT: break;
          case MOVE: player = c.arg; break;
          case SHOOT: shoot(&player, &c.arg); break;
          case RESTART: return true;
          case QUIT:
              puts(M_QUIT);
              return false;}
      if ( !wumpus_alive && point_eq(&player, &the_exit) )
          return endgame(M_WON);
      if ( wumpus_alive && !arrows )
          return endgame(M_LOST_NOARROWS);
      if ( point_eq(&player, &wumpus) )
          return endgame(M_LOST_DEFENDING_WUMPUS);
      if ( point_eq(&player, &grue) )
          return endgame(M_LOST_DEFENDING_GRUE);
      if ( move_wumpus(c.type == SHOOT) )
          return endgame(M_LOST_ATTACKING_WUMPUS);
      if ( move_grue() )
          return endgame(M_LOST_ATTACKING_GRUE);}}

void shoot(point *origin, point *dest)
/* Shoots an arrow from origin in the direction of dest. */
 {--arrows;
  bool hit_wumpus = false;
  bool hit_grue = false;
  if ( point_eq(dest, &wumpus) ) hit_wumpus = true;
  if ( point_eq(dest, &grue) ) hit_grue = true;
  if (!hit_wumpus && !hit_grue)
     /* The arrow forks. Note that if both monsters happen to
     occupy a single one of these alternate rooms, only the
     wumpus gets hit, since the arrow's already split in two, and
     given the choice, one would generally prefer to hit the
     wumpus instead of the grue. On the other hand, if one monster
     is in each of these alternate rooms, you'll hit both of them. */
     {point a, b;
      bool two = other_rooms(dest, origin, &a, &b);
      if ( point_eq(&a, &wumpus) ) hit_wumpus = true;
      else if ( point_eq(&a, &grue) ) hit_grue = true;
      if (two)
         {if ( point_eq(&b, &wumpus) ) hit_wumpus = true;
          else if ( point_eq(&b, &grue) ) hit_grue = true;}}
  if (hit_wumpus)
     {wumpus_alive = false;
      wumpus.x = MAX_X + 1;
      wumpus.y = MAX_Y + 1;}
  if (hit_grue) grue_stun += GRUE_STUN_DURATION;
  msg(hit_wumpus && hit_grue
    ? M_HIT_BOTH
    : hit_wumpus
    ? M_HIT_WUMPUS
    : hit_grue
    ? M_HIT_GRUE
    : M_HIT_NOTHING);}

bool move_wumpus(bool shot)
/* Lets the wumpus take its turn. shot indicates whether or not
the player fired an arrow this turn. Returns true iff the wumpus
killed the player. */
 {if (!wumpus_alive || !shot || g_random_double() > WUMPUS_MOVE_PROB)
      return false;
  wumpus = r_connected(&wumpus);
  return point_eq(&wumpus, &player);}

bool move_grue(void)
/* Lets the grue take its turn. Returns true iff it killed the
player. */
 {if (grue_stun)
     {--grue_stun;
      return false;}
  enum {UP, DOWN, FWD, UNSET} dir, uod;
  int px = player.x; int py = player.y;
  int gx = grue.x; int gy = grue.y;
  uod = py > gy ? UP : DOWN;
  point new_pos; new_pos.x = gx; new_pos.y = gy;
  // If the player is within three moves, the grue moves closer.
  int dm = gx % 2 == gy % 2 ? 1 : -1;
    // "dm" stands for "directional multiplier".
  dir = // Because you can't use the trinary operator too much.
        gx == px && abs(gy - py) <= 3
      ? uod
      : (gx + dm*2 == px && abs(gy - py) == 1) ||
        (gx + dm*1 == px && abs(gy - py) < 2)
      ? FWD
      : gx + dm*1 == px && abs(gy - py) == 2
      ? (g_random_boolean() ? FWD : uod)
      : gx - dm*1 == px &&
        (abs(gy - py) == 1 || abs(gy - py) == 2)
      ? uod
      : gx - dm*1 == px && gy == py
      ? (g_random_boolean() ? UP : DOWN)
      : UNSET;
  point a, b;
  switch (dir)
     {case UP:    ++new_pos.y;       break;
      case DOWN:  --new_pos.y;       break;
      case FWD:   new_pos.x += dm;   break;
      default:
          // The player isn't nearby, so the grue wanders.
          other_rooms(&grue, &grue_before, &a, &b);
          new_pos = a;}
  grue_before = grue;
  grue = new_pos;
  return point_eq(&grue, &player);}

// ------------------------------------------------------------
// General utility functions
// ------------------------------------------------------------

nat r_int(nat max)
/* Returns a random natural number from 0 to max inclusive. */
 {return g_random_int_range(0, max+1);}

point * r_point(GHashTable *h)
/* Returns a handom key from a point-hash. */
 {nat index = r_int( g_hash_table_size(h) - 1 ) + 1;
  void *k;
  GHashTableIter i;
  g_hash_table_iter_init(&i, h);
  for (nat n = 0 ; n < index ; ++n)
      g_hash_table_iter_next(&i, &k, NULL);
  return k;}

point r_connected(point *p)
/* Returns a random point connected to p. */
 {GHashTable *temp = connected_rooms(p);
  point q = *r_point(temp);
  g_hash_table_destroy(temp);
  return q;}

bool set_if_legal(point *p, nat x, nat y)
/* Sets p and returns true iff (x, y) is a legal place to go
from the player's position. */
 {if (x > MAX_X || y > MAX_Y ||
      !( (x == player.x &&
           (y == player.y + 1 || y == player.y - 1)) ||
         (y == player.y && x == player.x +
           (player.x % 2 == player.y % 2
             ? 1 : -1))) )
      return false;
  p->x = x;
  p->y = y;
  return true;}

void add_point(GHashTable *h, nat x, nat y)
/* Creates a new point with the given coordinates and adds it as
a key to h, unless the point doesn't exist, in which case nothing
happens. */
 {if (x > MAX_X || y > MAX_Y) return;
  point *n = kmalloc( sizeof(point) );
  n->x = x;
  n->y = y;
  g_hash_table_insert(h, n, NULL);}

bool other_rooms(point *center, point *exclude, point *a, point *b)
/* Sets a and b to the two rooms adjacent to center that aren't
exclude. Which room goes into which variable is intentionally
random. Iff there's actually only one such room, a is set to it and
the function returns false. */
 {nat x = center->x;
  nat ex = exclude->x;
  nat y = center->y;
  nat ey = exclude->y;
  bool set_a = false;
  bool set_b = false;
  a->x = x; a->y = y;
  b->x = x; b->y = y;
  if (y != MAX_Y && ey != y + 1)
     {a->y = y + 1;
      set_a = true;}
  if (y != 0 && ey != y - 1)
     {if (set_a)
         {b->y = y - 1;
          set_b = true;}
      else
         {a->y = y - 1;
          set_a = true;}}
  if (x % 2 == y % 2)
     {if (x != MAX_X && ex != x + 1)
         {if (set_a)
             {b->x = x + 1;
              set_b = true;}
          else
             {a->x = x + 1;
              set_a = true;}}}
  else  
     {if (x != 0 && ex != x - 1)
         {if (set_a)
             {b->x = x - 1;
              set_b = true;}
          else a->x = x - 1;}}
  if (!set_b) return false;
  if ( g_random_boolean() )
     {point temp = *a;
      *a = *b;
      *b = temp;}
  return true;}

GHashTable * connected_rooms(point *p)
/* Returns a hash of points connected to the given point. */
 {GHashTable *h = g_hash_table_new_full
     ((GHashFunc) point_hash, (GEqualFunc) point_eq,
      kfree, NULL);
  nat x = p->x;
  nat y = p->y;
  if (y > 0)                        add_point(h, x, y - 1);
  if (y < MAX_Y)                    add_point(h, x, y + 1);
  if (x % 2 == y % 2 && x < MAX_X)  add_point(h, x + 1, y);
  if (x % 2 != y % 2 && x > 0)      add_point(h, x - 1, y);
  return h;}

GHashTable * rooms_nearby(point *p)
/* Returns a hash with a key for every room within three moves
of p. */
 {GHashTable *rooms = g_hash_table_new_full
     ((GHashFunc) point_hash, (GEqualFunc) point_eq,
      kfree, NULL);
  int px = p->x;
  int py = p->y;
  for (int x = px - 1 ; x <= px + 1 ; ++x)
      for (int y = py - 2 ; y <= py + 2  ; ++y)
          if (!(x == px && y == py)) add_point(rooms, x, y);
  add_point(rooms, px, py - 3);
  add_point(rooms, px, py + 3);
  int sign = px % 2 == py % 2 ? 1 : -1;
  add_point(rooms, px + sign*2, py - 1);
  add_point(rooms, px + sign*2, py + 1);
  return rooms;}

// ------------------------------------------------------------
// I/O
// ------------------------------------------------------------

void describe_surroundings(void)
/* Prints out information describing the player's position and
status. */
 {nat x = player.x;
  nat y = player.y;
  bool sp = x % 2 == y % 2 ? true : false;
    // "sp" stands for "same parities".
  puts("\x1b\x5b\x48\x1b\x5b\x32\x4a");
    // That incantation clears the screen.

  // First, the "map".
  printf("➳: %u", arrows);
  if (y < MAX_Y)
     {printf(sp ? "  " : "          ");
      printf(F_COORDS, x, y + 1);
      printf("\nW: %c", wumpus_alive ? 'A' : 'D');
      puts(sp
        ? "      \\\n           \\"
        : "            /\n               /");}
  else
      printf("\nW: %c\n\n", wumpus_alive ? 'A' : 'D');
  printf(" ");
  if (!sp && x > 0)
     {printf(F_COORDS, x - 1, y);
      printf("--");}
  else
     {printf("         ");}
  printf(F_COORDS, x, y);
  if (sp && x < MAX_X)
     {printf("--");
      printf(F_COORDS, x + 1, y);}
  puts("");
  if (y > 0)
     {printf(sp
        ? "            /\n           /\n      "
        : "              \\\n               \\\n              ");
      printf(F_COORDS, x, y - 1);}
  else
      {puts("\n");}
  puts("\n");

  //printf("[(%2u,%2u) : (%2u,%2u) : (%2u,%2u)]\n\n",
  //    wumpus.x, wumpus.y, grue.x, grue.y, the_exit.x, the_exit.y);
  
  /* Warn if the monsters or the exit is nearby. The order in
  which the warnings are given may give the player free hints,
  but shuffling them would be too much of a pain in the neck to
  implement. I hate C. */
  if ( point_eq(&player, &the_exit) ) puts(M_CANTEXIT);
  GHashTableIter i;
  void *p;
  GHashTable *nearby = rooms_nearby(&player);
  g_hash_table_iter_init(&i, nearby);
  while ( g_hash_table_iter_next(&i, &p, NULL) )
     {if ( point_eq(p, &wumpus) ) puts(M_WUMPUS);
      if ( point_eq(p, &grue) ) puts(grue_stun
        ? M_STUNNED_GRUE
        : M_GRUE);
      if ( point_eq(p, &the_exit) ) puts(M_EXIT);}
  g_hash_table_destroy(nearby);
  puts("");

  // Print out any messages in the message queue, then clear
  // the queue.
  if (buffer_size > 0)
     {puts(buffer);
      buffer_size = 0;}}

cmd get_command(void)
/* Reads a command from the keyboard. */
 {char in;
  cmd c;
  nat dkey;
  c.type = MOVE;
  for (;;)
     {in = getchar();
      if (in >= 'A' && in <= 'Z') in += 'a' - 'A';
      switch (in)
         {case KEY_SHOOT:
              if (arrows < 1)
                 {puts(M_NOARROWS);
                  break;}
              puts(M_SHOOTWHERE);
              dkey = 2;
              while (dkey == 2)
                  dkey = handle_dirkey(getchar(), &c, M_CANTSHOOT);
              if (dkey)
                 {c.type = SHOOT;
                  return c;}
              puts(M_CANCELSHOT);
              break;
          case KEY_DESCRIBE:
              describe_surroundings();
              break;
          case KEY_WAIT:
              msg(M_WAIT);
              c.type = WAIT;
              return c;
          case KEY_RESTART:
              if ( yes_or_no(M_CONFIRMRESTART) )
                 {c.type = RESTART;
                  return c;}
              puts(M_DIDNTRESTART);
              break;
          case KEY_QUIT:
              if ( yes_or_no(M_CONFIRMQUIT) )
                 {c.type = QUIT;
                  return c;}
              puts(M_DIDNTQUIT);
              break;
          default:
              dkey = handle_dirkey(in, &c, M_CANTGO);
              if (dkey == 1) return c;
              if (dkey == 0) puts(M_NOTCMD);}}}

nat handle_dirkey(char k, cmd *c, char *msg)
/* If k isn't a direction key, nothing happens and the function
returns 0. Otherwise, if its direction is a valid one,
c->arg is set to its direction and the function returns 1.
Otherwise, msg is printed and the function returns 2. */
 {nat x = player.x;
  nat y = player.y;
  switch (k)
     {case KEY_UP:
          if (set_if_legal(&c->arg, x, y + 1)) return 1;
          puts(msg); return 2;
      case KEY_DOWN:
          if (set_if_legal(&c->arg, x, y - 1)) return 1;
          puts(msg); return 2;
      case KEY_LEFT:
          if (set_if_legal(&c->arg, x - 1, y)) return 1;
          puts(msg); return 2;
      case KEY_RIGHT:
          if (set_if_legal(&c->arg, x + 1, y)) return 1;
          puts(msg); return 2;}
  return 0;}

void msg(char *m)
// Adds a message to the queue.
 {nat n = 0;
  nat pos = buffer_size;
  char c = m[n];
  while (c != 0)
     {buffer[pos] = c;
      ++n;
      c = m[n];
      ++pos;}
  buffer[pos] = '\n';
  buffer[++pos] = 0;
  buffer_size = pos;}

bool yes_or_no(char *prompt)
// Asks a yes-or-no question and returns the answer.
 {puts(prompt);
  char in;
  for (;;)
     {in = getchar();
      if (in >= 'A' && in <= 'Z') in += 'a' - 'A';
      switch (in)
         {case KEY_YES: return true;
          case KEY_NO: return false;
          default: printf("Please press \"%c\" or \"%c\".\n",
              KEY_YES, KEY_NO);}}}

bool endgame(char *message)
/* Prints out any queued messages, prints the given message, asks
if the player wants to play again, and returns the answer. */
 {if (buffer_size > 0)
     {puts(buffer);
      buffer_size = 0;}
  puts(message);
  return yes_or_no(M_AGAIN);}

// The code for these two functions is largely stolen from
// "http://www.steve.org.uk/Reference/Unix/faq_4.html".

void munge_terminal(void)
/* Sets the terminal to single-character, no-echo mode. */
 {struct termios x;

  tcgetattr(0, &original_terminal_settings);
  x = original_terminal_settings;

  x.c_lflag &= (~ICANON);
  x.c_cc[VTIME] = 0;
  x.c_cc[VMIN] = 1;
  x.c_lflag &= (~ECHO);

  tcsetattr(0, TCSANOW, &x);}

void unmunge_terminal(void)
/* Reverses the effect of a previous munge_terminal call. Be
sure to call this before quitting the program. */
 {tcsetattr(0, TCSANOW, &original_terminal_settings);}

// ------------------------------------------------------------

//GList * add_point(GList *l, nat x, nat y)
///* Creates a new point with the given coordinates and prepends it
//to l. */
// {point *n = kmalloc( sizeof(point) );
//  n->x = x;
//  n->y = y;
//  return g_list_prepend(l, n);}
//
//GList * connected_rooms(point *p)
///* Returns a GList of points connected to the given point. Don't
//forget to free all of the points and the list itself when you're
//done with them. */
// {GList *l = NULL;
//  nat x = p->x;
//  nat y = p->y;
//  if (y > 0)
//      l = add_point(l, x, y - 1);
//  if (y < MAX_Y)
//      l = add_point(l, x, y + 1);
//  if (x % 2 == y % 2 && x < MAX_X)
//      l = add_point(l, x + 1, y);
//  if (x % 2 != y % 2 && x > 0)
//      l = add_point(l, x - 1, y);
//  return l;}
//
//GList * connected_rooms_except(point *p, point *e)
///* Like connected_rooms, but omits the other given point. */
// {GList *l = NULL;
//  nat x = p->x;
//  nat y = p->y;
//  nat ex = e->x;
//  nat ey = e->y;
//  if (y > 0 && !(x == ex && y - 1 == ey))
//      l = add_point(l, x, y - 1);
//  if (y < MAX_Y && !(x == ex && y + 1 == ey))
//      l = add_point(l, x, y + 1);
//  if (x % 2 == y % 2 && x < MAX_X && !(x + 1 == ex && y == ey))
//      l = add_point(l, x + 1, y);
//  if (x % 2 != y % 2 && x > 0 && !(x - 1 == ex && y == ey))
//      l = add_point(l, x - 1, y);
//  return l;}
