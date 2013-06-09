/*******************************************************************************
* File: deathTanks.c
*
* Description: This FreeRTOS program uses the AVR graphics module to display
*  and track the game state of a battle tanks rendition called "DeathTanks." When
*  running on the AVR STK600, connect the switches to port B. SW7 turns left,
*  SW6 turns right, SW1 accelerates forward, and SW0 shoots a bullet. Initially,
*  five large asteroids are spawned around the player. As the player shoots the
*  asteroids with bullets, they decompose into three smaller asteroids. When
*  the player destroys all of the asteroids, they win the game. If the player
*  collides with an asteroid, they lose the game. In both the winning and losing
*  conditions, the game pauses for three seconds and displays an appropriate
*  message. It is recommended to compile the FreeRTOS portion of this project
*  with heap_2.c instead of heap_1.c since pvPortMalloc and vPortFree are used.
*
* Author(s): Doug Gallatin & Andrew Lehmer, Haleigh Vierra & Matt Cruse
*
* Revisions:
* 5/8/12 MAC implemented spawn and create asteroid functions. 
* 5/8/12 HAV implemented bullet functions and added queue.
*******************************************************************************/

#define F_CPU 16000000
#include <avr/interrupt.h>
#include <avr/io.h>
#include <stdlib.h>
#include <math.h>
#include "FreeRTOS.h"
#include "task.h"
#include "semphr.h"
#include "graphics.h"
#include "usart.h"
#include "snes.h"

// Array of tank sprite image names
const char* tank_images[] = {
   "tank0.png",
   "tank1.png",
   "tank2.png",
   "tank3.png"};

// Array of bullet sprite image names
const char* bullet_images[] = {
   "bullet0.png",
   "bullet1.png",
   "bullet2.png",
   "bullet3.png"};

// Array of number sprite image names
const char* num_images[] = {
   "3.png",
   "2.png",
   "1.png"};
// Array of "round" sprite image names
const char* round_images[] = {
   "round1.png",
   "round2.png",
   "round3.png"};

// Array of health sprite image names
const char* health_images1[] = {
   "p1_health5.png",
   "p1_health4.png",
   "p1_health3.png",
   "p1_health2.png",
   "p1_health1.png",
   "health0.png"};

// Array of health sprite image names
const char* health_images2[] = {
   "p2_health5.png",
   "p2_health4.png",
   "p2_health3.png",
   "p2_health2.png",
   "p2_health1.png",
   "health0.png"};
//represents a point on the screen
typedef struct {
	float x;
	float y;
} point;

//object is used to represent the tank, bullets 
typedef struct object_s {
	xSpriteHandle handle;
	point pos;
	point vel;
	float accel;
	int16_t angle;
	int8_t a_vel;
	uint8_t size;
	uint8_t life;
	struct object_s *next;
} object;

//struct used to represent the walls
typedef struct wall {
   xSpriteHandle handle;
   point topLeft, botRight;
   int16_t angle;
   struct wall *next;
} wall;

// struct used to track info on a tank
typedef struct tank_info{
   object* tank; 
   object** bullets;
   uint8_t* fire_button;
   uint8_t number;    
}tank_info;

#define DEG_TO_RAD M_PI / 180.0

#define SCREEN_W 960
#define SCREEN_H 640

#define DEAD_ZONE_OVER_2 120

#define FRAME_DELAY_MS  10
#define GAME_RESET_DELAY_MS  2000
#define CONTROLLER_DELAY_MS 100

#define WALL_SIZE 50
#define WALL_WIDTH 19.2
#define WALL_HEIGHT 12.8
#define WALL_BLOCK 2
#define WALL_BOUNCE 5
#define WALL_EDGE WALL_SIZE / 2.2

#define TANK_SIZE 60
#define TANK_OFFSET TANK_SIZE / 2.0
#define NUM_TANK_SPRITES 4


#define BULLET_SIZE 20
#define BULLET_DELAY_MS 1000
#define BULLET_VEL 8.0
#define MAX_LIFE 100
#define DAMAGE 20
#define HEALTH_BAR_SIZE 150 
#define HEALTH_BAR_OFFSET_P1 20
#define HEALTH_BAR_OFFSET_P2 SCREEN_W-5 

#define TANK_SEL_BANNER_SIZE 100
#define TANK_NOT_SELECTED 0
#define TANK_SELECTED 1
#define NUM_ROUNDS 3

// Tank Parameters
#define TANK_MAX_VEL 3.0
#define TANK_ACCEL 0.05
#define TANK_AVEL  1.0

#define BACKGROUND_AVEL 0.01

// Game Statuses
#define IN_PLAY        0
#define PLAYER_ONE_WIN 1
#define PLAYER_TWO_WIN 2


static xTaskHandle input1TaskHandle, input2TaskHandle;
static xTaskHandle bullet1TaskHandle, bullet2TaskHandle;
static xTaskHandle update1TaskHandle, update2TaskHandle;
static xTaskHandle uartTaskHandle;

//Mutex used synchronize usart usage
static xSemaphoreHandle usartMutex;

static xSemaphoreHandle xSnesMutex;


// variables used to track the selected tank sprite for each player
uint8_t p1_tank_num, p2_tank_num;


static wall *walls = NULL;
static wall *borders = NULL;

static object tank1;
static object tank2;


uint8_t p1_score = 0, p2_score = 0, game_round = 0;
uint8_t tank1_health_img = 0, tank2_health_img = 0;
//linked lists for bullets
static object *bullets_tank1 = NULL;
static object *bullets_tank2 = NULL;

uint8_t fire_button1 = 0;
uint8_t fire_button2 = 0;

// Initialize tank_info for all players
static tank_info tank_info1;
static tank_info tank_info2;

static xGroupHandle wallGroup;
static xGroupHandle tankGroup1;
static xGroupHandle tankGroup2;
static xSpriteHandle background;
static xSpriteHandle health1, health2;

void init(void);
void reset(void);
wall *createWall(char * image, float x, float y, int16_t angle, wall *nxt, float height, float width);
object *createBullet(float x, float y, float velx, float vely, uint8_t tank_num, int16_t angle, object *nxt);
void startup(void);

/*------------------------------------------------------------------------------
 * Function: inputTask
 *
 * Description: This task polls PINB for the current button state to determine
 *  if the player should turn, accelerate, or both. This task never blocks, so
 *  it should run at the lowest priority above the idle task priority.
 *
 * param vParam: This parameter is a pointer to a tank_info struct
 *----------------------------------------------------------------------------*/
void inputTask(void *vParam) {
    /* Note:
     * tank.accel stores if the tank is moving
     * tank.a_vel stores which direction the tank is moving in
     */

	tank_info* tank_stuff = (tank_info*)vParam; 

	snesInit(tank_stuff->number); 
   
	DDRF = 0xFF;

	uint16_t controller_data;
   while (1) {
      xSemaphoreTake(xSnesMutex, portMAX_DELAY);
      
      controller_data = snesData(tank_stuff->number);

      if(controller_data & SNES_LEFT_BTN)
         tank_stuff->tank->a_vel = +TANK_AVEL;
      else if(controller_data & SNES_RIGHT_BTN)
         tank_stuff->tank->a_vel = -TANK_AVEL;
      else
         tank_stuff->tank->a_vel = 0;
   
      if(controller_data & SNES_B_BTN)
         tank_stuff->tank->accel = TANK_ACCEL;
      else if(controller_data & SNES_A_BTN)
         tank_stuff->tank->accel = -(TANK_ACCEL/2.0);   
      else {
         tank_stuff->tank->accel = 0;
         tank_stuff->tank->vel.x = tank_stuff->tank->vel.y = 0;
      }      
   
      if(controller_data & SNES_Y_BTN)
          *(tank_stuff->fire_button) = 1;

      xSemaphoreGive(xSnesMutex);

      vTaskDelay(CONTROLLER_DELAY_MS/portTICK_RATE_MS);
   }
}

/*------------------------------------------------------------------------------
 * Function: bulletTask
 *
 * Description: This task polls the state of the button that fires a bullet
 *  every 10 milliseconds. If a bullet is fired, this task blocks for half a
 *  second to regulate the fire rate.  If a bullet is not fired, the task
 *  blocks for a frame delay (FRAME_DELAY_MS)
 *
 * param vParam: This parameter the tank object whose bullets should be made.
 *----------------------------------------------------------------------------*/
void bulletTask(void *vParam) {
	 /* Note:
     * The tank heading is stored in tank.angle.
     * The tank's position is stored in tank.pos.x and tank.pos.y
     */

   tank_info* tank_stuff = (tank_info*)vParam;
     
   while (1) {
      // Check if the fire button has been pressed
      if(*(tank_stuff->fire_button)) {
         xSemaphoreTake(usartMutex, portMAX_DELAY);
         
         //Make a new bullet and add to linked list
         *(tank_stuff->bullets) = createBullet(
            tank_stuff->tank->pos.x,
            tank_stuff->tank->pos.y,
            -sin(tank_stuff->tank->angle*DEG_TO_RAD)*BULLET_VEL,
            -cos(tank_stuff->tank->angle*DEG_TO_RAD)*BULLET_VEL,
            tank_stuff->number,
            tank_stuff->tank->angle,
            *(tank_stuff->bullets));
         
         
         xSemaphoreGive(usartMutex);
         vTaskDelay(BULLET_DELAY_MS/portTICK_RATE_MS);
         // Clear the fire button
         *(tank_stuff->fire_button) = 0;
      }
      else
         vTaskDelay(FRAME_DELAY_MS / portTICK_RATE_MS);
   }
}

/*------------------------------------------------------------------------------
 * Function: updateTask
 *
 * Description: This task observes the currently stored velocities for every
 *  game object and updates their position and rotation accordingly. It also
 *  updates the tank's velocities based on its current acceleration and angle.
 *  If a bullet has been in flight for too long, this task will delete it. This
 *  task runs every 10 milliseconds.
 *
 * param vParam: This parameter is a pointer to a tank_info struct.
 *----------------------------------------------------------------------------*/
void updateTask(void *vParam) {
	float vel;
	object *objIter, *objPrev;
   tank_info* tank_stuff = (tank_info*)vParam;
   
	for (;;) {
		// spin tank
		tank_stuff->tank->angle += tank_stuff->tank->a_vel;
		if (tank_stuff->tank->angle >= 360)
         tank_stuff->tank->angle -= 360;
		else if (tank_stuff->tank->angle < 0)
		   tank_stuff->tank->angle += 360;
 

		// Update tank velocity
		tank_stuff->tank->vel.x += tank_stuff->tank->accel * -sin(tank_stuff->tank->angle * DEG_TO_RAD);
		tank_stuff->tank->vel.y += tank_stuff->tank->accel * -cos(tank_stuff->tank->angle * DEG_TO_RAD);
		vel = tank_stuff->tank->vel.x * tank_stuff->tank->vel.x + tank_stuff->tank->vel.y * tank_stuff->tank->vel.y;
		// Check if tank is at max velocity
      if (vel > TANK_MAX_VEL) {
			tank_stuff->tank->vel.x *= TANK_MAX_VEL / vel;
			tank_stuff->tank->vel.y *= TANK_MAX_VEL / vel;
		}
      // Update tank position
		tank_stuff->tank->pos.x += tank_stuff->tank->vel.x;
		tank_stuff->tank->pos.y += tank_stuff->tank->vel.y;
		
      // Check if tank is near boundries, and stop it if it hits a wall
		if (tank_stuff->tank->pos.x - TANK_OFFSET < WALL_EDGE) {
   		tank_stuff->tank->pos.x += WALL_BOUNCE;
   		tank_stuff->tank->vel.x = 0;
   		tank_stuff->tank->vel.y = 0;
   		tank_stuff->tank->accel = 0;
   		tank_stuff->tank->a_vel = 0;
		} else if (tank_stuff->tank->pos.x + TANK_OFFSET > SCREEN_W - (WALL_EDGE)) {
   		tank_stuff->tank->pos.x -= WALL_BOUNCE;
   		tank_stuff->tank->vel.x = 0;
   		tank_stuff->tank->vel.y = 0;
   		tank_stuff->tank->accel = 0;
   		tank_stuff->tank->a_vel = 0;
		}
		if (tank_stuff->tank->pos.y - TANK_OFFSET < WALL_EDGE) {
   		tank_stuff->tank->pos.y += WALL_BOUNCE;
   		tank_stuff->tank->vel.x = 0;
   		tank_stuff->tank->vel.y = 0;
   		tank_stuff->tank->accel = 0;
   		tank_stuff->tank->a_vel = 0;
		} else if (tank_stuff->tank->pos.y + TANK_OFFSET > SCREEN_H - (WALL_EDGE)) {
   		tank_stuff->tank->pos.y -= WALL_BOUNCE;
   		tank_stuff->tank->vel.x = 0;
   		tank_stuff->tank->vel.y = 0;
   		tank_stuff->tank->accel = 0;
   		tank_stuff->tank->a_vel = 0;
		}

		// move bullets
		objPrev = NULL;
		objIter = *(tank_stuff->bullets);
		while (objIter != NULL) {
         objIter->pos.x += objIter->vel.x;
         objIter->pos.y += objIter->vel.y;

         objPrev = objIter;
         objIter = objIter->next;
		}
		
		vTaskDelay(FRAME_DELAY_MS / portTICK_RATE_MS);
	}
}

/*------------------------------------------------------------------------------
 * Function: drawTask
 *
 * Description: This task sends the appropriate commands to update the game
 *  graphics every 10 milliseconds for a target frame rate of 100 FPS. It also
 *  checks collisions and performs the proper action based on the types of the
 *  colliding objects.
 *
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void drawTask(void *vParam) {   
	object *objIter, *objPrev;
   wall *wallIter, *wallPrev;
	xSpriteHandle hit, handle;
	point topLeft, botRight;
	uint8_t game_status = IN_PLAY;
	
	vTaskSuspend(update1TaskHandle);
   vTaskSuspend(update2TaskHandle);
	vTaskSuspend(bullet1TaskHandle);
   vTaskSuspend(bullet2TaskHandle);
	vTaskSuspend(input1TaskHandle);
   vTaskSuspend(input2TaskHandle);
	init();
	vTaskResume(update1TaskHandle);
   vTaskResume(update2TaskHandle);
	vTaskResume(bullet1TaskHandle);
   vTaskResume(bullet2TaskHandle);
	vTaskResume(input1TaskHandle);
	vTaskResume(input2TaskHandle);
   
	for (;;) {
		xSemaphoreTake(usartMutex, portMAX_DELAY);
		if (uCollide(tank1.handle, wallGroup, &hit, 1) > 0) {
   		wallPrev = NULL;
   		wallIter = walls;
   		while (wallIter != NULL) {
            if (wallIter->handle == hit) {
      		   topLeft = wallIter->topLeft;
      		   botRight = wallIter->botRight;
      		   //checks collision on x-axis
      		   if (tank1.pos.x > topLeft.x && tank1.pos.x < botRight.x) {
         		   if (abs(tank1.pos.y - topLeft.y) < abs(tank1.pos.y - botRight.y))
         		      tank1.pos.y -= WALL_BOUNCE;
         		   else
         		      tank1.pos.y += WALL_BOUNCE;
      		   }
      		   
      		   //checks collision on y-axis
      		   if (tank1.pos.y > topLeft.y && tank1.pos.y < botRight.x) {
         		   if (abs(tank1.pos.x - topLeft.x) < abs(tank1.pos.x - botRight.x))
         		      tank1.pos.x -= WALL_BOUNCE;
         		   else
         		      tank1.pos.x += WALL_BOUNCE;
               }               
               tank1.vel.x = 0;
               tank1.vel.y = 0;
               tank1.accel = 0;
               tank1.a_vel = 0;
               
               break;
            } else {
               wallPrev = wallIter;
               wallIter = wallIter->next;
            }
         }
      }
		vSpriteSetRotation(tank1.handle, (uint16_t)tank1.angle);
		vSpriteSetPosition(tank1.handle, (uint16_t)tank1.pos.x, (uint16_t)tank1.pos.y);
      
      if (uCollide(tank2.handle, wallGroup, &hit, 1) > 0) {
         wallPrev = NULL;
         wallIter = walls;
         while (wallIter != NULL) {
            if (wallIter->handle == hit) {
               topLeft = wallIter->topLeft;
               botRight = wallIter->botRight;
               //checks collision on x-axis
               if (tank2.pos.x > topLeft.x && tank2.pos.x < botRight.x) {
                  if (abs(tank2.pos.y - topLeft.y) < abs(tank2.pos.y - botRight.y))
                     tank2.pos.y -= WALL_BOUNCE;
                  else
                     tank2.pos.y += WALL_BOUNCE;
               }
         
               //checks collision on y-axis
               if (tank2.pos.y > topLeft.y && tank2.pos.y < botRight.x) {
                  if (abs(tank2.pos.x - topLeft.x) < abs(tank2.pos.x - botRight.x))
                  tank2.pos.x -= WALL_BOUNCE;
                  else
                  tank2.pos.x += WALL_BOUNCE;
               }
         
               tank2.vel.x = 0;
               tank2.vel.y = 0;
               tank2.accel = 0;
               tank2.a_vel = 0;
         
               break;
            } else {
               wallPrev = wallIter;
               wallIter = wallIter->next;
            }
         }
      }
      vSpriteSetRotation(tank2.handle, (uint16_t)tank2.angle);
      vSpriteSetPosition(tank2.handle, (uint16_t)tank2.pos.x, (uint16_t)tank2.pos.y);
      
      // Check hits from tank1
		objPrev = NULL;
		objIter = bullets_tank1;
		while (objIter != NULL) {
   		vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
         //// Check hits from tank1 on tank2
         if (uCollide(objIter->handle, tankGroup2, &hit, 1) > 0) {
      		vSpriteDelete(objIter->handle);
      		tank2.life -= DAMAGE; 
            tank2_health_img++;
            vSpriteDelete(health2);
            health2 = xSpriteCreate(health_images2[tank2_health_img],HEALTH_BAR_OFFSET_P2, SCREEN_H>>3, 0, HEALTH_BAR_SIZE, HEALTH_BAR_SIZE, 20);
      		if (objPrev != NULL) {
         		objPrev->next = objIter->next;
         		vPortFree(objIter);
         		objIter = objPrev->next;
      		}
            else {
         		bullets_tank1 = objIter->next;
         		vPortFree(objIter);
         		objIter = bullets_tank1;
      		}
            if(tank2.life <= 0)
               game_status = PLAYER_ONE_WIN;
		   }
         else if (uCollide(objIter->handle, wallGroup, &hit, 1) > 0) {
            vSpriteDelete(objIter->handle);
            
            if (objPrev != NULL) {
               objPrev->next = objIter->next;
               vPortFree(objIter);
               objIter = objPrev->next;
            }
            else {
               bullets_tank1 = objIter->next;
               vPortFree(objIter);
               objIter = bullets_tank1;
            }
         }
         else {
            objPrev = objIter;
            objIter = objIter->next;
         }
      }

      // Check hits from tank2
      objPrev = NULL;
      objIter = bullets_tank2;
      while (objIter != NULL) {
         vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
         //// Check hits from tank2 on tank1
         if (uCollide(objIter->handle, tankGroup1, &hit, 1) > 0) {
            vSpriteDelete(objIter->handle);
            tank1.life -= DAMAGE;
            tank1_health_img++;
            vSpriteDelete(health1);
            health1 = xSpriteCreate(health_images1[tank1_health_img],HEALTH_BAR_OFFSET_P1, SCREEN_H>>3, 0, HEALTH_BAR_SIZE, HEALTH_BAR_SIZE, 20);
            
            if (objPrev != NULL) {
               objPrev->next = objIter->next;
               vPortFree(objIter);
               objIter = objPrev->next;
            }
            else {
               bullets_tank2 = objIter->next;
               vPortFree(objIter);
               objIter = bullets_tank2;
            }
            if(tank1.life <= 0)
               game_status = PLAYER_TWO_WIN;
         }
         else if (uCollide(objIter->handle, wallGroup, &hit, 1) > 0) {
            vSpriteDelete(objIter->handle);
            
            if (objPrev != NULL) {
               objPrev->next = objIter->next;
               vPortFree(objIter);
               objIter = objPrev->next;
            }
            else {
               bullets_tank2 = objIter->next;
               vPortFree(objIter);
               objIter = bullets_tank2;
            }
         }
         else {
            objPrev = objIter;
            objIter = objIter->next;
         }
      }


      if((game_status == PLAYER_ONE_WIN)||(game_status == PLAYER_TWO_WIN)){
         vTaskDelete(update1TaskHandle);
         vTaskDelete(update2TaskHandle);
         vTaskDelete(bullet1TaskHandle);
         vTaskDelete(bullet2TaskHandle);
         vTaskDelete(input1TaskHandle);
         vTaskDelete(input2TaskHandle);
         switch(game_status){
            case PLAYER_ONE_WIN:
               handle = xSpriteCreate("p1_win_round.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
               p1_score ++;
               game_round++;
               break;
            case PLAYER_TWO_WIN:
               handle = xSpriteCreate("p2_win_round.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
               p2_score ++;
               game_round++;
               break;
            default:
               break;
         }
         vTaskDelay(GAME_RESET_DELAY_MS / portTICK_RATE_MS);
         
         vSpriteDelete(handle);
         if((game_round >= NUM_ROUNDS)||(p1_score == NUM_ROUNDS - 1)||(p2_score == NUM_ROUNDS - 1))
         {
            if(p1_score > p2_score)
               handle = xSpriteCreate("p1_win.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
            else
               handle = xSpriteCreate("p2_win.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
               
            _delay_ms(GAME_RESET_DELAY_MS);
            vSpriteDelete(handle);
            p1_score = 0;
            p2_score = 0;
            game_round = 0;
            
            
         }
         vSpriteDelete(health1);
         vSpriteDelete(health2);
         tank1_health_img = tank2_health_img = 0;
         reset();
         init();
         
         xTaskCreate(inputTask, (signed char *) "p1", 80, &tank_info1, 4, &input1TaskHandle);
         xTaskCreate(inputTask, (signed char *) "p2", 80, &tank_info2, 4, &input2TaskHandle);
         xTaskCreate(bulletTask, (signed char *) "b1", 250, &tank_info1, 1, &bullet1TaskHandle);
         xTaskCreate(bulletTask, (signed char *) "b2", 250, &tank_info2, 1, &bullet2TaskHandle);
         xTaskCreate(updateTask, (signed char *) "u", 200, &tank_info1, 3, &update1TaskHandle);
         xTaskCreate(updateTask, (signed char *) "u", 200, &tank_info2, 3, &update2TaskHandle);
         game_status = IN_PLAY;
      }
      
		
		xSemaphoreGive(usartMutex);
		vTaskDelay(FRAME_DELAY_MS / portTICK_RATE_MS);
	}
}

int main(void) {
	DDRB = 0x00;
	TCCR2A = _BV(CS00);

   tank_info1 = (tank_info){
      .tank = &tank1,
      .bullets = &bullets_tank1,
      .fire_button = &fire_button1,
      .number = 1};
   
   tank_info2 = (tank_info){
      .tank = &tank2,
      .bullets = &bullets_tank2,
      .fire_button = &fire_button2,
      .number = 2};
   
   xSnesMutex = xSemaphoreCreateMutex();
	usartMutex = xSemaphoreCreateMutex();
	
	vWindowCreate(SCREEN_W, SCREEN_H);
	
	sei();

	xTaskCreate(inputTask, (signed char *) "p1", 80, &tank_info1, 4, &input1TaskHandle);
	xTaskCreate(inputTask, (signed char *) "p2", 80, &tank_info2, 4, &input2TaskHandle);
	xTaskCreate(bulletTask, (signed char *) "b1", 250, &tank_info1, 1, &bullet1TaskHandle);
   xTaskCreate(bulletTask, (signed char *) "b2", 250, &tank_info2, 1, &bullet2TaskHandle);
	xTaskCreate(updateTask, (signed char *) "u", 200, &tank_info1, 3, &update1TaskHandle);
	xTaskCreate(updateTask, (signed char *) "u", 200, &tank_info2, 3, &update2TaskHandle);
	xTaskCreate(drawTask, (signed char *) "d", 800, NULL, 2, NULL);
	xTaskCreate(USART_Write_Task, (signed char *) "w", 500, NULL, 5, &uartTaskHandle);
	
	vTaskStartScheduler();
	
	for(;;);
	return 0;
}

/*------------------------------------------------------------------------------
 * Function: init
 *
 * Description: This function initializes a new game of asteroids. A window
 *  must be created before this function may be called.
 *----------------------------------------------------------------------------*/
void init(void) {
	bullets_tank1 = NULL;
   bullets_tank2 = NULL;
   tankGroup1 = ERROR_HANDLE;
   tankGroup2 = ERROR_HANDLE;
	xSpriteHandle number; 
   
   tank1.life =MAX_LIFE;
   tank2.life =MAX_LIFE;
   // function to initialize program
   if(game_round == 0)
      startup();
	background = xSpriteCreate("map.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W, SCREEN_H, 0);
	
	srand(TCNT0);
	
	wallGroup = xGroupCreate();
	tankGroup1 = xGroupCreate();
   tankGroup2 = xGroupCreate();

   // tank1 create
	tank1.handle = xSpriteCreate(
      tank_images[p1_tank_num], 
      SCREEN_W >> 2,
      SCREEN_H >> 1, 
      270, 
      TANK_SIZE, 
      TANK_SIZE, 
      1);
   
	tank1.pos.x = SCREEN_W >> 2;
	tank1.pos.y = SCREEN_H >> 1;
	tank1.vel.x = 0;
	tank1.vel.y = 0;
	tank1.accel = 0;
	tank1.angle = 270;
	tank1.a_vel = 0;

   // tank2 create
   tank2.handle = xSpriteCreate(
      tank_images[p2_tank_num],
      SCREEN_W - (SCREEN_W >> 2),
      SCREEN_H >> 1,
      90,
      TANK_SIZE,
      TANK_SIZE,
      1);
   
   tank2.pos.x = SCREEN_W - (SCREEN_W >> 2);
   tank2.pos.y = SCREEN_H >> 1;
   tank2.vel.x = 0;
   tank2.vel.y = 0;
   tank2.accel = 0;
   tank2.angle = 90;
   tank2.a_vel = 0;

   fire_button1 = 0;
   fire_button2 = 0;
   
   health1 = xSpriteCreate(health_images1[tank1_health_img], HEALTH_BAR_OFFSET_P1, SCREEN_H>>3, 0, HEALTH_BAR_SIZE, HEALTH_BAR_SIZE, 20);
   health2 = xSpriteCreate(health_images2[tank2_health_img],HEALTH_BAR_OFFSET_P2, SCREEN_H>>3, 0, HEALTH_BAR_SIZE, HEALTH_BAR_SIZE, 20);
   
   borders = createWall(
      "width_wall.bmp",
      SCREEN_W >> 1,
      0,
      0,
      borders,
      1,
      WALL_WIDTH);
      
   borders = createWall(
      "width_wall.bmp",
      SCREEN_W >> 1,
      SCREEN_H,
      0,
      borders,
      1,
      WALL_WIDTH);
   
   borders = createWall(
      "side_wall.bmp",
      0,
      SCREEN_H >> 1,
      0,
      borders,
      WALL_HEIGHT,
      1);
   
   borders = createWall(
      "side_wall.bmp",
      SCREEN_W,
      SCREEN_H >> 1,
      0,
      borders,
      WALL_HEIGHT,
      1);
      
   walls = createWall(
      "wall.bmp",
      SCREEN_W >> 1,
      SCREEN_H >> 1,
      0,
      walls,
      8,
      1);
      
   walls = createWall(
      "small_wall.bmp",
      SCREEN_W - 2.5 * WALL_SIZE,
      SCREEN_H >> 2,
      0,
      walls,
      1,
      4);
      
   walls = createWall(
      "small_wall.bmp",
      2.5 * WALL_SIZE,
      SCREEN_H - (SCREEN_H >> 2),
      0,
      walls,
      1,
      4);
      
   walls = createWall(
      "block_wall.bmp",
      SCREEN_W - 4.5 * WALL_SIZE,
      SCREEN_H - 1.5 * WALL_SIZE,
      0,
      walls,
      WALL_BLOCK,
      WALL_BLOCK);
      
   walls = createWall(
      "block_wall.bmp",
      4.5 * WALL_SIZE,
      1.5 * WALL_SIZE,
      0,
      walls,
      WALL_BLOCK,
      WALL_BLOCK);
   fire_button2 = 0;   

   vGroupAddSprite(tankGroup1, tank1.handle);
   vGroupAddSprite(tankGroup2, tank2.handle);
   
   
   number = xSpriteCreate(round_images[game_round], SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 20);
   _delay_ms(1000);
   vSpriteDelete(number);
   for(uint8_t i = 0; i < 3; i++)
   {
      number = xSpriteCreate(num_images[i], SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 20);
      _delay_ms(750);
      vSpriteDelete(number);
   }   
   number = xSpriteCreate("go.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 20);
   _delay_ms(1000);
   vSpriteDelete(number);   
}

/*------------------------------------------------------------------------------
 * Function: reset
 *
 * Description: This function destroys all game objects in the heap and clears
 *  their respective sprites from the window.
 *----------------------------------------------------------------------------*/
void reset(void) {
	/* Note:
     * You need to free all resources here using a reentrant function provided by
     * the freeRTOS API and clear all sprites from the game window.
     * Use vGroupDelete for the asteroid group.
     *
     * Remember bullets and asteroids are object lists so you should traverse the list
     * using something like:  
     *	while (thisObject != NULL) {
     *		vSpriteDelete(thisObject)
     *		nextObject = thisObject->next.
     *		delete thisObject using a reentrant function
     *		thisObject = nextObject
     *	}
     */
	object *nextObject;
	wall *nextWall;

	while (walls != NULL) {
   	vSpriteDelete(walls->handle);
   	nextWall = walls->next;
   	vPortFree(walls);
   	walls = nextWall;
	}
	vGroupDelete(wallGroup);

   while (borders != NULL) {
      vSpriteDelete(borders->handle);
      nextWall = borders->next;
      vPortFree(borders);
      borders = nextWall;
   }
   
   // removes bullets_tank1
	while (bullets_tank1 != NULL) {
   	vSpriteDelete(bullets_tank1->handle);
   	nextObject = bullets_tank1->next;
   	vPortFree(bullets_tank1);
   	bullets_tank1 = nextObject;
   }
	// removes bullets_tank2
	while (bullets_tank2 != NULL) {
   	vSpriteDelete(bullets_tank2->handle);
   	nextObject = bullets_tank2->next;
   	vPortFree(bullets_tank2);
   	bullets_tank2 = nextObject;
	}
   
   //removes the tanks
   vSpriteDelete(tank1.handle);
   vSpriteDelete(tank2.handle);

   vGroupDelete(tankGroup1);
   vGroupDelete(tankGroup2);
   //removes the background
   vSpriteDelete(background);
   
   
   
   USART_Let_Queue_Empty();
}

wall *createWall(char *image, float x, float y, int16_t angle, wall *nxt, float height, float width) {
   //allocate space for a new wall
   wall *newWall = pvPortMalloc(sizeof(wall));
   
   //setup wall sprite
   newWall->handle = xSpriteCreate(
      image,                  //reference to png filename
      x,                      //xPos
      y,                      //yPos
      angle,                  //rAngle
      WALL_SIZE * width,      //width
      WALL_SIZE * height,     //height
      1);                     //depth
   
   //set positions
   newWall->topLeft.x = 1 + x - ((width / 2) * WALL_SIZE);
   newWall->topLeft.y = 1 + y - ((height / 2) * WALL_SIZE);
   //xSpriteCreate("bullet1.png", newWall->topLeft.x, newWall->topLeft.y, 0, tank_SIZE, tank_SIZE, 15);
   
   newWall->botRight.x = x + ((width / 2) * WALL_SIZE);
   newWall->botRight.y = y + ((height / 2) * WALL_SIZE);
   //xSpriteCreate("bullet2.png", newWall->botRight.x, newWall->botRight.y, 0, tank_SIZE, tank_SIZE, 16);
   //set angle
   newWall->angle = angle;
   //link to asteroids list
   newWall->next = nxt;
   //add to asteroids sprite group
   vGroupAddSprite(wallGroup, newWall->handle);
   
   //return pointer to new asteroid
   return newWall;
}

/*------------------------------------------------------------------------------
 * Function: createBullet
 *
 * Description: This function creates a new bullet object.
 *
 * param x: The starting x position of the new bullet sprite.
 * param y: The starting y position of the new bullet sprite.
 * param velx: The new bullet's x velocity.
 * param vely: The new bullet's y velocity.
 * param nxt: A pointer to the next bullet object in a linked list of bullets.
 * return: A pointer to a malloc'd bullet object. This pointer must be freed by
 *  the caller.
 *----------------------------------------------------------------------------*/
object *createBullet(float x, float y, float velx, float vely, uint8_t tank_num, int16_t angle, object *nxt) {
	//Create a new bullet object using a reentrant malloc() function
	object *newBullet = pvPortMalloc(sizeof(object));
	
	//Setup the pointers in the linked list
	//Create a new sprite using xSpriteCreate()
   if(tank_num == 2) {
   	newBullet->handle = xSpriteCreate(
	   bullet_images[p2_tank_num],			//reference to png filename
	      x,                   //xPos
	      y,                   //yPos
	      angle,                //rAngle
	      BULLET_SIZE,			//width
	      BULLET_SIZE,			//height
	      1);                  //depth
   }
   else {
      newBullet->handle = xSpriteCreate(
         bullet_images[p1_tank_num],			//reference to png filename
         x,                   //xPos
         y,                   //yPos
         angle,                //rAngle
         BULLET_SIZE,			//width
         BULLET_SIZE,			//height
         1);                  //depth
   }
   //set position
   newBullet->pos.x = x;
   newBullet->pos.y = y;
   //set velocity
   newBullet->vel.x = velx;
   newBullet->vel.y = vely;
   //set size
   newBullet->size = BULLET_SIZE;
   //link to bullets list
   newBullet->next = nxt;
   //return pointer to the new bullet
   return (newBullet); 
}

void startup(void) {
   p1_tank_num = TANK_NOT_SELECTED;
   p2_tank_num = TANK_NOT_SELECTED;
   uint8_t p1_sel = TANK_NOT_SELECTED, p2_sel = TANK_NOT_SELECTED;
   
   uint16_t controller_data1 = 0;
   uint16_t controller_data2 = 0;
   uint8_t press_start_loop_count = 0;
   
   xSpriteHandle p1, p2;
   xSpriteHandle press_start;
   // Print opening start screen
   xSpriteHandle start_screen = xSpriteCreate("start_screen.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W, SCREEN_H, 0);
   
   // Initailize SNES Controllers
   snesInit(SNES_2P_MODE);
   // Wait for player1 to press start
   while(!((controller_data1 & SNES_STRT_BTN)||(controller_data2 & SNES_STRT_BTN))) {
      controller_data1 = snesData(SNES_P1);
      controller_data2 = snesData(SNES_P2);
      _delay_ms(17);

      if(press_start_loop_count++ == 30)
         press_start = xSpriteCreate("press_start.png", SCREEN_W>>1, SCREEN_H - (SCREEN_H>>2), 0, SCREEN_W>>1, SCREEN_H>>1, 1);
      else if(press_start_loop_count == 60) {
         vSpriteDelete(press_start);
         press_start_loop_count = 0; 
      } 
   }
   _delay_ms(250);
   if(press_start_loop_count >= 30)
      vSpriteDelete(press_start);
      
   _delay_ms(500);
   vSpriteDelete(start_screen);
   
   // Display the tank select screen
   xSpriteHandle select_screen = xSpriteCreate("select_screen.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W, SCREEN_H, 0);
    
   controller_data1 = 0;
   controller_data2 = 0;
   
   p1_tank_num = p2_tank_num = 0;
   p1 = xSpriteCreate("p1.png", ((2*p1_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
   p2 = xSpriteCreate("p2.png", ((2*p2_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
   // get a valid tank selection from both controllers
   while((p1_sel == TANK_NOT_SELECTED) || (p2_sel == TANK_NOT_SELECTED)) {
      
      
      if(p1_sel == TANK_NOT_SELECTED) {
         controller_data1 = snesData(SNES_P1);
         switch(controller_data1) {
            case SNES_RIGHT_BTN:
               p1_tank_num++;
               if(p1_tank_num > 3)
                  p1_tank_num = 0;
               break;
            case SNES_LEFT_BTN:
               if(p1_tank_num <= 0)
                  p1_tank_num = 3;
               else
                  p1_tank_num--;
               break;
            case SNES_A_BTN:
               p1_sel = TANK_SELECTED;
               break;
            default:
               break;
         }
         
         if((controller_data1 & SNES_RIGHT_BTN)||(controller_data1 & SNES_LEFT_BTN)) {
            vSpriteDelete(p1);
            switch(p1_tank_num) {
               case 0:
                  p1 = xSpriteCreate("p1.png", ((2*p1_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               case 1:
                  p1 = xSpriteCreate("p1.png", ((2*p1_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               case 2:
                  p1 = xSpriteCreate("p1.png", ((2*p1_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               case 3:
                  p1 = xSpriteCreate("p1.png", ((2*p1_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               default:
                  p1 = xSpriteCreate("p1.png", ((2*p1_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
            }
         }
      }

      if(p2_sel == TANK_NOT_SELECTED) {
         controller_data2 = snesData(SNES_P2);
         switch(controller_data2) {
            case SNES_RIGHT_BTN:
               p2_tank_num++;
               if(p2_tank_num > 3)
                  p2_tank_num = 0;
               break;
            case SNES_LEFT_BTN:
               if(p2_tank_num <= 0)
                  p2_tank_num = 3;
               else
                  p2_tank_num--;
               break;
            case SNES_A_BTN:
               p2_sel = TANK_SELECTED;
               break;
            default:
               break;
         }
        if((controller_data2 & SNES_RIGHT_BTN)||(controller_data2 & SNES_LEFT_BTN)) {
            vSpriteDelete(p2);
            switch(p2_tank_num) {
               case 0:
                  p2 = xSpriteCreate("p2.png", ((2*p2_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               case 1:
                  p2 = xSpriteCreate("p2.png", ((2*p2_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               case 2:
                  p2 = xSpriteCreate("p2.png", ((2*p2_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               case 3:
                  p2 = xSpriteCreate("p2.png", ((2*p2_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
               default:
                  p2 = xSpriteCreate("p2.png", ((2*p2_tank_num + 1)*SCREEN_W)/8, SCREEN_H>>1, 0, TANK_SEL_BANNER_SIZE, TANK_SEL_BANNER_SIZE, 1);
                  break;
            }
         }         
      }    
      _delay_ms(150);
   }
   _delay_ms(1000);
   vSpriteDelete(p1);
   vSpriteDelete(p2);
   vSpriteDelete(select_screen);
}
																															