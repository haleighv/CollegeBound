/*
 * CollegeBound.h
 *
 * Created: 6/2/2013 10:18:39 PM
 *  Author: Matt
 */ 
#define PANEL_NUM 12
#define PANEL_SIZE 80

#define UP     0b0001
#define RIGHT  0b0010
#define DOWN   0b0100
#define LEFT   0b1000
#define ALL    0b1111

#define CHAIR 2
#define TRUE 1
#define FALSE 0


//Background images
const char *bkgrndImages[] = {
   "floor0.png",
   "floor1.png",
   "floor2.png"
};

const char *astImages[] = {
   "a1.png",
   "a2.png",
   "a3.png"
};

//represents a point on the screen
typedef struct {
   float x;
   float y;
} point;

//object is used to represent the ship, bullets, and asteroids
typedef struct object_s {
   xSpriteHandle handle;
   point pos;
   point vel;
   float accel;
   int16_t angle;
   int8_t a_vel;
   uint8_t size;
   uint16_t life;
   struct object_s *next;
} object;

typedef struct move {
   char *name;
   int damage;
   object animation;
} move;

typedef struct character {
   object handler;
   int level, exp, health;
   move mv[4];
} character;
//represents a place on the map
typedef struct gridPanel{
   char vadidDir;
   char event;
   void *occupant;
} gridPanel;
//
//gridPanel floor0[PANEL_NUM][PANEL_NUM] = {
   ////row 0
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {0, TRUE, NULL}, {0, TRUE, NULL}},
   ////row 1
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, TRUE, NULL}, {RIGHT | DOWN, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {LEFT, FALSE, NULL}},
   ////row 2
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, TRUE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{UP | RIGHT | LEFT, FALSE, NULL}, {ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 3
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, TRUE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL},
   //{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 4
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL},
   //{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {UP | LEFT, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 5
   //{{0, TRUE, NULL}, {RIGHT | DOWN, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | DOWN | LEFT, FALSE, NULL}, {ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL},
   //{0, FALSE, NULL}, {UP | DOWN, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 6
   //{{0, TRUE, NULL}, {UP | RIGHT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{ALL, FALSE, NULL}, {ALL, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 7
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {DOWN | LEFT, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 8
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 9
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 10
   //{{RIGHT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL},
   //{UP | RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {LEFT, FALSE, NULL}},
   ////row 11
   //{{0, TRUE, NULL}, {0, TRUE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {0, TRUE, NULL}, {0, TRUE, NULL}}
//};
//
//gridPanel floor1[PANEL_NUM][PANEL_NUM] = {
   ////row 0
   //{{0, TRUE, NULL}, {0, TRUE, NULL}, {0, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {0, TRUE, NULL}, {0, TRUE, NULL}},
   ////row 1
   //{{RIGHT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL},
   //{RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {LEFT, FALSE, NULL}},
   ////row 2
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 3
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 4
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 5
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 6
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 7
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 8
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 9
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{DOWN | UP, FALSE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {DOWN | UP, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 10
   //{{RIGHT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL},
   //{UP | RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{RIGHT | LEFT, FALSE, NULL}, {RIGHT | LEFT, FALSE, NULL}, {LEFT, FALSE, NULL}},
   ////row 11
   //{{0, TRUE, NULL}, {0, TRUE, NULL}, {0, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, TRUE, NULL}, {0, FALSE, NULL}, {0, TRUE, NULL},
   //{0, FALSE, NULL}, {0, TRUE, NULL}, {0, TRUE, NULL}}
//};

//gridPanel floor2[PANEL_NUM][PANEL_NUM] = {
   ////row 0
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, TRUE, NULL}, {0, TRUE, NULL},
   //{0, TRUE, NULL}, {0, TRUE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 1
   //{{0, TRUE, NULL}, {RIGHT | DOWN, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | DOWN | LEFT, FALSE, NULL}, {DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 2
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {ALL, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{UP | RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 3
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | LEFT, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, CHAIR, NULL}, {UP | RIGHT | DOWN, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 4
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{UP | DOWN | LEFT, FALSE, NULL}, {0, CHAIR, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {RIGHT, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 5
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {LEFT, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, CHAIR, NULL}, {UP | RIGHT | DOWN, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 6
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{UP | DOWN | LEFT, FALSE, NULL}, {0, CHAIR, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {RIGHT, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 7
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {LEFT, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, CHAIR, NULL}, {UP | RIGHT | DOWN, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 8
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{UP | DOWN | LEFT, FALSE, NULL}, {0, CHAIR, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 9
   //{{0, FALSE, NULL}, {UP | RIGHT | DOWN, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL}, {RIGHT | DOWN | LEFT, FALSE, NULL},
   //{RIGHT | DOWN | LEFT, FALSE, NULL}, {ALL, FALSE, NULL}, {ALL, FALSE, NULL},
   //{ALL, FALSE, NULL}, {UP | DOWN | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 10
   //{{0, TRUE, NULL}, {UP | RIGHT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{UP | RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{UP | RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL}, {UP | RIGHT | LEFT, FALSE, NULL},
   //{UP | RIGHT | LEFT, FALSE, NULL}, {UP | LEFT, FALSE, NULL}, {0, FALSE, NULL}},
   ////row 11
   //{{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL},
   //{0, FALSE, NULL}, {0, FALSE, NULL}, {0, FALSE, NULL}}
//};