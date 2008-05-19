/* -*- C -*- */

// ------------------------------------------------------------
// Settings
// ------------------------------------------------------------

#define HEXES_WIDE 6 /* In the longer rows of the map. */
#define HEXES_TALL 6 /* Ditto for the columns. */
  // Alternating rows and columns of hexes always differ in their
  // total number of hexes by one.
#define STARTING_ARROWS 3 /* How many arrows you start with. */
#define WUMPUS_MOVE_PROB 0.75
  // Probability the wumpus moves each time you shoot an arrow.
#define GRUE_STUN_DURATION 3
  // How many moves the grue misses for each time you hit it
  // with an arrow.

// Keyboard commands; letters should be lowercase.
#define   KEY_YES        'y'
#define   KEY_NO         'n'
#define   KEY_UP         'w'
#define   KEY_DOWN       's'
#define   KEY_LEFT       'a'
#define   KEY_RIGHT      'd'
#define   KEY_SHOOT      'p'
#define   KEY_WAIT       ' '
#define   KEY_DESCRIBE   'm'
#define   KEY_RESTART    '`'
#define   KEY_QUIT       '\x1b' /* 0x1b is the value of Escape. */

// Formats for printf.
#define F_COORDS "(%2u,%2u)"

#define BUFLEN 10240
  // Length of the message buffer. Ensure it's long enough
  // to never overflow.
// Messages.
#define M_CONFIRMQUIT "Are you sure you want to quit?"
#define M_QUIT "Chicken!"
#define M_DIDNTQUIT "Okay, let the hunt resume."
#define M_CONFIRMRESTART "Are you sure you want to abandon this game and start a new one?"
#define M_RESTART "A new game it is.\n  Press any key to continue."
#define M_DIDNTRESTART "All right, you can lose this game before moving on to the next."
#define M_AGAIN "Do you want to play again?"
#define M_NOTCMD "I don't recognize that command."
#define M_CANTGO "You can't go that way."
#define M_SHOOTWHERE "In which direction do you wish to shoot?\n  Press something other than a direction key to cancel the shot."
#define M_NOARROWS "You don't have any arrows left."
#define M_CANTSHOOT "I'm not going to let you waste an arrow shooting a wall."
#define M_CANCELSHOT "Shot aborted."
#define M_WAIT "You wait."
#define M_WUMPUS "I smell a wumpus!"
#define M_HIT_WUMPUS "Aha! You got the wumpus! Now to make good your escape."
#define M_LOST_ATTACKING_WUMPUS "Tsk, tsk, tsk— the wumpus got you!"
#define M_LOST_DEFENDING_WUMPUS "Oops! Bumped the wumpus!"
#define M_GRUE "I hear a horrible gurgling noise!"
#define M_HIT_GRUE "You hit the grue! Run, before the terrible beast recovers!"
#define M_STUNNED_GRUE "I hear a subdued gurgling noise."
#define M_LOST_ATTACKING_GRUE "Oh, no! The grue slithered into the room and devoured you!"
#define M_LOST_DEFENDING_GRUE "Oh, no! You have walked into the slavering fangs of the grue!"
#define M_HIT_BOTH "Groovy! You both slew the wumpus and stunned the grue with a\n  single shot! Now, run for the exit!"
#define M_HIT_NOTHING "Bummer, you didn't hit anything."
#define M_EXIT "I see a ray of sunlight."
#define M_CANTEXIT "The exit is here, but your quarry is still alive."
#define M_WON "You emerge from the dungeon victorious!"
#define M_LOST_NOARROWS "Alas, that was your last arrow, and the wumpus still lives."

// ------------------------------------------------------------
// Typedefs and related declarations
// ------------------------------------------------------------

typedef unsigned int nat; // "Nat" is short for "natural number".

typedef struct
 {nat x;
  nat y;} point;

typedef struct
 {enum {MOVE, SHOOT, WAIT, RESTART, QUIT} type;
  point arg;} cmd;

// The greatest legal coordinates.
#define MAX_X (2*HEXES_WIDE - 1)
#define MAX_Y (2*HEXES_TALL)

// ------------------------------------------------------------
// Global variables
// ------------------------------------------------------------

point player, wumpus, grue, the_exit; // The location of each.
nat arrows; // How many the player has left.
bool wumpus_alive;
point grue_before; // Where the grue was last turn.
nat grue_stun;
  // How many turns the grue must miss before it can move again.

char buffer[BUFLEN]; // The message buffer.
nat buffer_size;
  // How much stuff is in the buffer at any given moment.
struct termios original_terminal_settings;

// ------------------------------------------------------------
// Prototypes
// ------------------------------------------------------------

void setup(void);
void add_to_map(GHashTable *rooms, point *obj);
guint point_hash(void *p);
gboolean point_eq(void *a, void *b);

bool play(void);
void shoot(point *origin, point *dest);
bool move_wumpus(bool shot);
bool move_grue(void);

nat r_int(nat max);
point * r_point(GHashTable *h);
point r_connected(point *p);
bool set_if_legal(point *p, nat x, nat y);
void add_point(GHashTable *h, nat x, nat y);
bool other_rooms(point *center, point *exclude, point *a, point *b);
GHashTable * connected_rooms(point *p);
GHashTable * rooms_nearby(point *p);

void describe_surroundings(void);
cmd get_command(void);
nat handle_dirkey(char k, cmd *c, char *msg);
void msg(char *m);
bool yes_or_no(char *prompt);
bool endgame(char *message);
void munge_terminal(void);
void unmunge_terminal(void);

//GList * add_point(GList *l, nat x, nat y);
//GList * connected_rooms(point *p);
//GList * connected_rooms_except(point *p, point *e);
