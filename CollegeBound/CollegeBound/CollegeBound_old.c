/*******************************************************************************
* File: asteroids.c
*
* Description: This FreeRTOS program uses the AVR graphics module to display
*  and track the game state of the classic arcade game "Asteroids." When
*  running on the AVR STK500, connect the switches to port B. SW7 turns left,
*  SW6 turns right, SW1 accelerates forward, and SW0 shoots a bullet. Initially,
*  five large asteroids are spawned around the player. As the player shoots the
*  asteroids with bullets, they decompose into three smaller asteroids. When
*  the player destroys all of the asteroids, they win the game. If the player
*  collides with an asteroid, they lose the game. In both the winning and losing
*  conditions, the game pauses for three seconds and displays an appropriate
*  message. It is recommended to compile the FreeRTOS portion of this project
*  with heap_2.c instead of heap_1.c since pvPortMalloc and vPortFree are used.
*
* Author(s): Doug Gallatin & Andrew Lehmer
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

const char* tank_images[] = {
   "tank0.png",
   "tank1.png",
   "tank2.png",
   "tank3.png"};

const char* bullet_images[] = {
   "bullet0.png",
   "bullet1.png",
   "bullet2.png",
   "bullet3.png"};

const char* bro_tank_images[] = {
   "bro_tank0.png",
   "bro_tank1.png",
   "bro_tank2.png",
   "bro_tank3.png"};
   
   
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

//object is used to represent the ship, bullets, and asteroids
typedef struct wall {
   xSpriteHandle handle;
   point topLeft, botRight;
   int16_t angle;
   struct wall *next;
} wall;


#define DEG_TO_RAD M_PI / 180.0

#define SCREEN_W 960
#define SCREEN_H 640

#define DEAD_ZONE_OVER_2 120

#define FRAME_DELAY_MS  10
#define BULLET_DELAY_MS 500
#define BULLET_LIFE_MS  600

#define WALL_SIZE 50
#define WALL_WIDTH 19.2
#define WALL_HEIGHT 12.8
#define WALL_BLOCK 2
#define WALL_BOUNCE 5
#define WALL_EDGE WALL_SIZE / 2.2

#define SHIP_SIZE 60
#define SHIP_OFFSET SHIP_SIZE / 2.0
#define BULLET_SIZE 20

#define TANK_SEL_BANNER_SIZE 100
#define TANK_NOT_SELECTED 4

#define BULLET_VEL 8.0

// Tank Parameters
#define SHIP_MAX_VEL 3.0
#define SHIP_ACCEL 0.05
#define SHIP_AVEL  1.0

#define BACKGROUND_AVEL 0.01

// Game Statuses
#define IN_PLAY        0
#define PLAYER_ONE_WIN 1
#define PLAYER_TWO_WIN 2


static xTaskHandle inputTaskHandle;
static xTaskHandle bulletTaskHandle;
static xTaskHandle updateTaskHandle;
static xTaskHandle uartTaskHandle;

//Mutex used synchronize usart usage
static xSemaphoreHandle usartMutex;

static xSemaphoreHandle xSnesMutex;
static object ship1;
static object ship2;

uint8_t fire_button1 = 0;
uint8_t fire_button2 = 0;


uint8_t p1_tank_num, p2_tank_num;
//linked lists for bullets
static object *bullets_ship1 = NULL;
static object *bullets_ship2 = NULL;
static wall *walls = NULL;
static wall *borders = NULL;

static xGroupHandle wallGroup;
static xGroupHandle shipGroup1;
static xGroupHandle shipGroup2;
static xSpriteHandle background;

void init(void);
void reset(void);
wall *createWall(char * image, float x, float y, int16_t angle, wall *nxt, float height, float width);
object *createBullet(float x, float y, float velx, float vely, uint8_t ship_num, int16_t angle, object *nxt);
void startup(void);

/*------------------------------------------------------------------------------
 * Function: inputTask
 *
 * Description: This task polls PINB for the current button state to determine
 *  if the player should turn, accelerate, or both. This task never blocks, so
 *  it should run at the lowest priority above the idle task priority.
 *
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void inputTask(void *vParam) {
    /* Note:
     * ship.accel stores if the ship is moving
     * ship.a_vel stores which direction the ship is moving in
     */
	//vTaskDelay(1000/portTICK_RATE_MS);
	
	// variable to hold ticks value of last task run
	portTickType xLastWakeTime;
	
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();
	
	snesInit(SNES_2P_MODE); 
	//snesInit(player_num);
	
    while (1) {
		//xQueueReceive( xSnesDataQueue, &controller_data, portMAX_DELAY );
      //xSemaphoreTake(xSnesMutex, portMAX_DELAY);
		//controller_data = snesData(player_num);
		
		 
      DDRF = 0xFF;

      uint16_t controller_data1, controller_data2;

      controller_data1 = snesData(SNES_P1);
      controller_data2 = snesData(SNES_P2);
      //PORTF = ((controller_data1) & 0xF0)|((controller_data2>>4) & 0x0F);

      if(controller_data1 & SNES_LEFT_BTN)
         ship1.a_vel = +SHIP_AVEL;
      else if(controller_data1 & SNES_RIGHT_BTN)
         ship1.a_vel = -SHIP_AVEL;
      else
         ship1.a_vel = 0;
   
      if(controller_data1 & SNES_B_BTN)
         ship1.accel = SHIP_ACCEL;
      else {
         ship1.accel =0;
         ship1.vel.x = ship1.vel.y = 0;
      }      
   
      if(controller_data1 & SNES_Y_BTN)
         fire_button1 = 1;


      if(controller_data2 & SNES_LEFT_BTN)
         ship2.a_vel = +SHIP_AVEL;
      else if(controller_data2 & SNES_RIGHT_BTN)
         ship2.a_vel = -SHIP_AVEL;
      else
         ship2.a_vel = 0;
      
      if(controller_data2 & SNES_B_BTN)
         ship2.accel = SHIP_ACCEL;
      else {
         ship2.vel.x = ship2.vel.y = 0;
         ship2.accel = 0;
      }            
            
      if(controller_data2 & SNES_Y_BTN)
         fire_button2 = 1;
      //xSemaphoreGive(xSnesMutex);
      vTaskDelayUntil( &xLastWakeTime, 100/portTICK_RATE_MS);
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
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void bulletTask(void *vParam) {
	 /* Note:
     * The ship heading is stored in ship.angle.
     * The ship's position is stored in ship.pos.x and ship.pos.y
     *
     * You will need to use the following code to add a new bullet:
     * bullets = createBullet(x, y, vx, vy, bullets);
     */
	// variable to hold ticks value of last task run
	//portTickType xLastWakeTime;
   
	// Initialize the xLastWakeTime variable with the current time.
	//xLastWakeTime = xTaskGetTickCount();
   uint8_t firebutton;
   uint8_t tank_num = (uint8_t) *vParam; 
   if(tank_num == 2)
      firebutton =
   while (1) {
      if(fire_button1 || fire_button2) {
         xSemaphoreTake(usartMutex, portMAX_DELAY);
         if(fire_button1) {
            fire_button1 = 0;
            //Make a new bullet and add to linked list
            bullets_ship1 = createBullet(
               ship1.pos.x,
               ship1.pos.y,
               -sin(ship1.angle*DEG_TO_RAD)*BULLET_VEL,
               -cos(ship1.angle*DEG_TO_RAD)*BULLET_VEL,
               SNES_P1,
               ship1.angle,
               bullets_ship1);
         }
         if(fire_button2) {
            fire_button2 = 0;
            //Make a new bullet and add to linked list
            bullets_ship2 = createBullet(
               ship2.pos.x,
               ship2.pos.y,
               -sin(ship2.angle*DEG_TO_RAD)*BULLET_VEL,
               -cos(ship2.angle*DEG_TO_RAD)*BULLET_VEL,
               SNES_P2,
               ship2.angle,
               bullets_ship2);
         }
         xSemaphoreGive(usartMutex);
         vTaskDelay(BULLET_DELAY_MS/portTICK_RATE_MS);
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
 *  updates the ship's velocities based on its current acceleration and angle.
 *  If a bullet has been in flight for too long, this task will delete it. This
 *  task runs every 10 milliseconds.
 *
 * param vParam: This parameter is not used.
 *----------------------------------------------------------------------------*/
void updateTask(void *vParam) {
	float vel;
	object *objIter, *objPrev;
	for (;;) {
		// spin ship1
		ship1.angle += ship1.a_vel;
		if (ship1.angle >= 360)
         ship1.angle -= 360;
		else if (ship1.angle < 0)
		   ship1.angle += 360;
         
      // spin ship2
      ship2.angle += ship2.a_vel;
      if (ship2.angle >= 360)
      ship2.angle -= 360;
      else if (ship2.angle < 0)
      ship2.angle += 360;     

		// move ship1
		ship1.vel.x += ship1.accel * -sin(ship1.angle * DEG_TO_RAD);
		ship1.vel.y += ship1.accel * -cos(ship1.angle * DEG_TO_RAD);
		vel = ship1.vel.x * ship1.vel.x + ship1.vel.y * ship1.vel.y;
		if (vel > SHIP_MAX_VEL) {
			ship1.vel.x *= SHIP_MAX_VEL / vel;
			ship1.vel.y *= SHIP_MAX_VEL / vel;
		}
		ship1.pos.x += ship1.vel.x;
		ship1.pos.y += ship1.vel.y;
		
		if (ship1.pos.x - SHIP_OFFSET < WALL_EDGE) {
   		ship1.pos.x += WALL_BOUNCE;
   		ship1.vel.x = 0;
   		ship1.vel.y = 0;
   		ship1.accel = 0;
   		ship1.a_vel = 0;
		} else if (ship1.pos.x + SHIP_OFFSET > SCREEN_W - (WALL_EDGE)) {
   		ship1.pos.x -= WALL_BOUNCE;
   		ship1.vel.x = 0;
   		ship1.vel.y = 0;
   		ship1.accel = 0;
   		ship1.a_vel = 0;
		}
		if (ship1.pos.y - SHIP_OFFSET < WALL_EDGE) {
   		ship1.pos.y += WALL_BOUNCE;
   		ship1.vel.x = 0;
   		ship1.vel.y = 0;
   		ship1.accel = 0;
   		ship1.a_vel = 0;
		} else if (ship1.pos.y + SHIP_OFFSET > SCREEN_H - (WALL_EDGE)) {
   		ship1.pos.y -= WALL_BOUNCE;
   		ship1.vel.x = 0;
   		ship1.vel.y = 0;
   		ship1.accel = 0;
   		ship1.a_vel = 0;
		}

      // move ship2
      ship2.vel.x += ship2.accel * -sin(ship2.angle * DEG_TO_RAD);
      ship2.vel.y += ship2.accel * -cos(ship2.angle * DEG_TO_RAD);
      vel = ship2.vel.x * ship2.vel.x + ship2.vel.y * ship2.vel.y;
      if (vel > SHIP_MAX_VEL) {
         ship2.vel.x *= SHIP_MAX_VEL / vel;
         ship2.vel.y *= SHIP_MAX_VEL / vel;
      }
      ship2.pos.x += ship2.vel.x;
      ship2.pos.y += ship2.vel.y;

      if (ship2.pos.x - SHIP_OFFSET < WALL_EDGE) {
         ship2.pos.x += WALL_BOUNCE;
         ship2.vel.x = 0;
         ship2.vel.y = 0;
         ship2.accel = 0;
         ship2.a_vel = 0;
      } else if (ship2.pos.x + SHIP_OFFSET > SCREEN_W - (WALL_EDGE)) {
         ship2.pos.x -= WALL_BOUNCE;
         ship2.vel.x = 0;
         ship2.vel.y = 0;
         ship2.accel = 0;
         ship2.a_vel = 0;
      }
      if (ship2.pos.y - SHIP_OFFSET < WALL_EDGE) {
         ship2.pos.y += WALL_BOUNCE;
         ship2.vel.x = 0;
         ship2.vel.y = 0;
         ship2.accel = 0;
         ship2.a_vel = 0;
      } else if (ship2.pos.y + SHIP_OFFSET > SCREEN_H - (WALL_EDGE)) {
         ship2.pos.y -= WALL_BOUNCE;
         ship2.vel.x = 0;
         ship2.vel.y = 0;
         ship2.accel = 0;
         ship2.a_vel = 0;
      }
      
		// move bullets_ship1
		objPrev = NULL;
		objIter = bullets_ship1;
		while (objIter != NULL) {
			// Kill bullet after a while
			objIter->life += FRAME_DELAY_MS;
			if (objIter->life >= BULLET_LIFE_MS) {
				xSemaphoreTake(usartMutex, portMAX_DELAY);
				vSpriteDelete(objIter->handle);
				if (objPrev != NULL) {
					objPrev->next = objIter->next;
					vPortFree(objIter);
					objIter = objPrev->next;
				} else {
					bullets_ship1 = objIter->next;
					vPortFree(objIter);
					objIter = bullets_ship1;
				}
				xSemaphoreGive(usartMutex);
			} else {
            objIter->pos.x += objIter->vel.x;
            objIter->pos.y += objIter->vel.y;

            if (objIter->pos.x < 0.0) {
             objIter->pos.x += SCREEN_W;
            } else if (objIter->pos.x > SCREEN_W) {
             objIter->pos.x -= SCREEN_W;
            }

            if (objIter->pos.y < 0.0) {
             objIter->pos.y += SCREEN_H;
            } else if (objIter->pos.y > SCREEN_H) {
             objIter->pos.y -= SCREEN_H;
            }
            objPrev = objIter;
            objIter = objIter->next;
			}			
		}

      // move bullets_ship2
      objPrev = NULL;
      objIter = bullets_ship2;
      while (objIter != NULL) {
         // Kill bullet after a while
         objIter->life += FRAME_DELAY_MS;
         if (objIter->life >= BULLET_LIFE_MS) {
            xSemaphoreTake(usartMutex, portMAX_DELAY);
            vSpriteDelete(objIter->handle);
            if (objPrev != NULL) {
               objPrev->next = objIter->next;
               vPortFree(objIter);
               objIter = objPrev->next;
            } else {
               bullets_ship2 = objIter->next;
               vPortFree(objIter);
               objIter = bullets_ship2;
            }
            xSemaphoreGive(usartMutex);
         } else {
            objIter->pos.x += objIter->vel.x;
            objIter->pos.y += objIter->vel.y;

            if (objIter->pos.x < 0.0) {
               objIter->pos.x += SCREEN_W;
            } else if (objIter->pos.x > SCREEN_W) {
               objIter->pos.x -= SCREEN_W;
            }

            if (objIter->pos.y < 0.0) {
               objIter->pos.y += SCREEN_H;
            } else if (objIter->pos.y > SCREEN_H) {
               objIter->pos.y -= SCREEN_H;
            }
            objPrev = objIter;
            objIter = objIter->next;
         }
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
	
	vTaskSuspend(updateTaskHandle);
	vTaskSuspend(bulletTaskHandle);
	vTaskSuspend(inputTaskHandle);
	init();
	vTaskResume(updateTaskHandle);
	vTaskResume(bulletTaskHandle);
	vTaskResume(inputTaskHandle);
	
	for (;;) {
		xSemaphoreTake(usartMutex, portMAX_DELAY);
		if (uCollide(ship1.handle, wallGroup, &hit, 1) > 0) {
   		wallPrev = NULL;
   		wallIter = walls;
   		while (wallIter != NULL) {
            if (wallIter->handle == hit) {
      		   topLeft = wallIter->topLeft;
      		   botRight = wallIter->botRight;
      		   //checks collision on x-axis
      		   if (ship1.pos.x > topLeft.x && ship1.pos.x < botRight.x) {
         		   if (abs(ship1.pos.y - topLeft.y) < abs(ship1.pos.y - botRight.y))
         		      ship1.pos.y -= WALL_BOUNCE;
         		   else
         		      ship1.pos.y += WALL_BOUNCE;
      		   }
      		   
      		   //checks collision on y-axis
      		   if (ship1.pos.y > topLeft.y && ship1.pos.y < botRight.x) {
         		   if (abs(ship1.pos.x - topLeft.x) < abs(ship1.pos.x - botRight.x))
         		      ship1.pos.x -= WALL_BOUNCE;
         		   else
         		      ship1.pos.x += WALL_BOUNCE;
               }               
               ship1.vel.x = 0;
               ship1.vel.y = 0;
               ship1.accel = 0;
               ship1.a_vel = 0;
               
               break;
            } else {
               wallPrev = wallIter;
               wallIter = wallIter->next;
            }
         }
      }
		vSpriteSetRotation(ship1.handle, (uint16_t)ship1.angle);
		vSpriteSetPosition(ship1.handle, (uint16_t)ship1.pos.x, (uint16_t)ship1.pos.y);
      
      if (uCollide(ship2.handle, wallGroup, &hit, 1) > 0) {
         wallPrev = NULL;
         wallIter = walls;
         while (wallIter != NULL) {
            if (wallIter->handle == hit) {
               topLeft = wallIter->topLeft;
               botRight = wallIter->botRight;
               //checks collision on x-axis
               if (ship2.pos.x > topLeft.x && ship2.pos.x < botRight.x) {
                  if (abs(ship2.pos.y - topLeft.y) < abs(ship2.pos.y - botRight.y))
                  ship2.pos.y -= WALL_BOUNCE;
                  else
                  ship2.pos.y += WALL_BOUNCE;
               }
         
               //checks collision on y-axis
               if (ship2.pos.y > topLeft.y && ship2.pos.y < botRight.x) {
                  if (abs(ship2.pos.x - topLeft.x) < abs(ship2.pos.x - botRight.x))
                  ship2.pos.x -= WALL_BOUNCE;
                  else
                  ship2.pos.x += WALL_BOUNCE;
               }
         
               ship2.vel.x = 0;
               ship2.vel.y = 0;
               ship2.accel = 0;
               ship2.a_vel = 0;
         
               break;
            } else {
               wallPrev = wallIter;
               wallIter = wallIter->next;
            }
         }
      }
      vSpriteSetRotation(ship2.handle, (uint16_t)ship2.angle);
      vSpriteSetPosition(ship2.handle, (uint16_t)ship2.pos.x, (uint16_t)ship2.pos.y);
      
      // Check hits from ship1
		objPrev = NULL;
		objIter = bullets_ship1;
		while (objIter != NULL) {
   		vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
         //// Check hits from ship1 on ship2
         if (uCollide(objIter->handle, shipGroup2, &hit, 1) > 0) {
      		vSpriteDelete(objIter->handle);
      		
      		if (objPrev != NULL) {
         		objPrev->next = objIter->next;
         		vPortFree(objIter);
         		objIter = objPrev->next;
      		}
            else {
         		bullets_ship1 = objIter->next;
         		vPortFree(objIter);
         		objIter = bullets_ship1;
      		}
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
               bullets_ship1 = objIter->next;
               vPortFree(objIter);
               objIter = bullets_ship1;
            }
         }
         else {
            objPrev = objIter;
            objIter = objIter->next;
         }
      }

      // Check hits from ship2
      objPrev = NULL;
      objIter = bullets_ship2;
      while (objIter != NULL) {
         vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
         //// Check hits from ship2 on ship1
         if (uCollide(objIter->handle, shipGroup1, &hit, 1) > 0) {
            vSpriteDelete(objIter->handle);
      
            if (objPrev != NULL) {
               objPrev->next = objIter->next;
               vPortFree(objIter);
               objIter = objPrev->next;
            }
            else {
               bullets_ship2 = objIter->next;
               vPortFree(objIter);
               objIter = bullets_ship2;
            }
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
               bullets_ship2 = objIter->next;
               vPortFree(objIter);
               objIter = bullets_ship2;
            }
         }
         else {
            objPrev = objIter;
            objIter = objIter->next;
         }
      }

      switch(game_status)
      {
         case PLAYER_ONE_WIN:
            vTaskDelete(updateTaskHandle);
            vTaskDelete(bulletTaskHandle);
            vTaskDelete(inputTaskHandle);
            
            handle = xSpriteCreate("p1_win.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
            
            vTaskDelay(3000 / portTICK_RATE_MS);
            
            vSpriteDelete(handle);
            reset();
            init();            
            
            xTaskCreate(inputTask, (signed char *) "p1", 80, NULL, 6, &inputTaskHandle);
            xTaskCreate(bulletTask, (signed char *) "b", 250, NULL, 2, &bulletTaskHandle);
            xTaskCreate(updateTask, (signed char *) "u", 200, NULL, 4, &updateTaskHandle);
            game_status = IN_PLAY;
            break;
         case PLAYER_TWO_WIN: 
            vTaskDelete(updateTaskHandle);
            vTaskDelete(bulletTaskHandle);
            vTaskDelete(inputTaskHandle);
            
            handle = xSpriteCreate("p2_win.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
            
            vTaskDelay(3000 / portTICK_RATE_MS);

            vSpriteDelete(handle);
            reset();
            init();

            xTaskCreate(inputTask, (signed char *) "p1", 80, NULL, 6, &inputTaskHandle);
            xTaskCreate(bulletTask, (signed char *) "b", 250, NULL, 2, &bulletTaskHandle);
            xTaskCreate(updateTask, (signed char *) "u", 200, NULL, 4, &updateTaskHandle);
            game_status = IN_PLAY;
            break;
         default:
            break;
      }
		
		xSemaphoreGive(usartMutex);
		vTaskDelay(FRAME_DELAY_MS / portTICK_RATE_MS);
	}
}

int main(void) {
	DDRB = 0x00;
	TCCR2A = _BV(CS00);

   //----test code begin----//
	//snesInit(SNES_2P_MODE);
   //uint16_t controller_data1, controller_data2;      	
   //DDRF = 0xFF;
	//while(1)
	//{
		//controller_data1 = snesData(SNES_P1);
      //controller_data2 = snesData(SNES_P2);
      //PORTF = ((controller_data1) & 0xF0)|((controller_data2>>4) & 0x0F);
		//_delay_ms(16);
      //
	//}
   //----test code end----//

   xSnesMutex = xSemaphoreCreateMutex();
	usartMutex = xSemaphoreCreateMutex();
	
	vWindowCreate(SCREEN_W, SCREEN_H);
	
	sei();

	xTaskCreate(inputTask, (signed char *) "p1", 80, NULL, 6, &inputTaskHandle);
	xTaskCreate(bulletTask, (signed char *) "b", 250, NULL, 2, &bulletTaskHandle);
	xTaskCreate(updateTask, (signed char *) "u", 200, NULL, 4, &updateTaskHandle);
	xTaskCreate(drawTask, (signed char *) "d", 800, NULL, 3, NULL);
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
	bullets_ship1 = NULL;
   bullets_ship2 = NULL;
   shipGroup1 = ERROR_HANDLE;
   shipGroup2 = ERROR_HANDLE;
	
   // function to initialize program
   startup();
	background = xSpriteCreate("map.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W, SCREEN_H, 0);
	
	srand(TCNT0);
	
	wallGroup = xGroupCreate();
	shipGroup1 = xGroupCreate();
   shipGroup2 = xGroupCreate();

   // Ship1 create
	ship1.handle = xSpriteCreate(
      tank_images[p1_tank_num], 
      SCREEN_W >> 2,
      SCREEN_H >> 1, 
      270, 
      SHIP_SIZE, 
      SHIP_SIZE, 
      1);
   
	ship1.pos.x = SCREEN_W >> 2;
	ship1.pos.y = SCREEN_H >> 1;
	ship1.vel.x = 0;
	ship1.vel.y = 0;
	ship1.accel = 0;
	ship1.angle = 270;
	ship1.a_vel = 0;

   // Ship2 create
   ship2.handle = xSpriteCreate(
      tank_images[p2_tank_num],
      SCREEN_W - (SCREEN_W >> 2),
      SCREEN_H >> 1,
      90,
      SHIP_SIZE,
      SHIP_SIZE,
      1);
   
   ship2.pos.x = SCREEN_W - (SCREEN_W >> 2);
   ship2.pos.y = SCREEN_H >> 1;
   ship2.vel.x = 0;
   ship2.vel.y = 0;
   ship2.accel = 0;
   ship2.angle = 90;
   ship2.a_vel = 0;

   fire_button1 = 0;
   fire_button2 = 0;
   
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

   vGroupAddSprite(shipGroup1, ship1.handle);
   vGroupAddSprite(shipGroup2, ship2.handle);
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
   
   // removes bullets_ship1
	while (bullets_ship1 != NULL) {
   	vSpriteDelete(bullets_ship1->handle);
   	nextObject = bullets_ship1->next;
   	vPortFree(bullets_ship1);
   	bullets_ship1 = nextObject;
   }
	// removes bullets_ship2
	while (bullets_ship2 != NULL) {
   	vSpriteDelete(bullets_ship2->handle);
   	nextObject = bullets_ship2->next;
   	vPortFree(bullets_ship2);
   	bullets_ship2 = nextObject;
	}
   
   //removes the ships
   vSpriteDelete(ship1.handle);
   vSpriteDelete(ship2.handle);

   vGroupDelete(shipGroup1);
   vGroupDelete(shipGroup2);
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
   //xSpriteCreate("bullet1.png", newWall->topLeft.x, newWall->topLeft.y, 0, SHIP_SIZE, SHIP_SIZE, 15);
   
   newWall->botRight.x = x + ((width / 2) * WALL_SIZE);
   newWall->botRight.y = y + ((height / 2) * WALL_SIZE);
   //xSpriteCreate("bullet2.png", newWall->botRight.x, newWall->botRight.y, 0, SHIP_SIZE, SHIP_SIZE, 16);
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
object *createBullet(float x, float y, float velx, float vely, uint8_t ship_num, int16_t angle, object *nxt) {
	//Create a new bullet object using a reentrant malloc() function
	object *newBullet = pvPortMalloc(sizeof(object));
	
	//Setup the pointers in the linked list
	//Create a new sprite using xSpriteCreate()
   if(ship_num == 2) {
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
   //set bullets life
   newBullet->life = 0;
   //link to bullets list
   newBullet->next = nxt;
   //return pointer to the new bullet
   return (newBullet); 
}

void startup(void) {
   p1_tank_num = TANK_NOT_SELECTED;
   p2_tank_num = TANK_NOT_SELECTED;
   uint16_t controller_data1 = 0;
   uint16_t controller_data2 = 0;
   uint8_t press_start_loop_count = 0;
   xSpriteHandle p1, p2;
   xSpriteHandle press_start;
   // Print opening start screen
   xSpriteHandle start_screen = xSpriteCreate("start_screen.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W, SCREEN_H, 0);
   //xSpriteHandle press_start = xSpriteCreate("press_start.png", SCREEN_W>>1, SCREEN_H - (SCREEN_H>>2), 0, SCREEN_W>>1, SCREEN_H>>1, 1);
   
   // Initailize SNES Controllers
   snesInit(SNES_2P_MODE);
   // Wait for player1 to press start
   while(!(controller_data1 & SNES_STRT_BTN)) {
      controller_data1 = snesData(SNES_P1);
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
   
   // get a valid tank selection from both controllers
   while((p1_tank_num == TANK_NOT_SELECTED) || (p2_tank_num == TANK_NOT_SELECTED)) {
      if(p1_tank_num == TANK_NOT_SELECTED) {
         controller_data1 = snesData(SNES_P1);
         switch(controller_data1) {
            case SNES_A_BTN:
               p1_tank_num = 0;
               break;
            case SNES_B_BTN:
               p1_tank_num = 1;
               break;
            case SNES_X_BTN:
               p1_tank_num = 2;
               break;
            case SNES_Y_BTN:
               p1_tank_num = 3;
               break;
            default:
               p1_tank_num = TANK_NOT_SELECTED;
               break;
         }
         
         if(p1_tank_num != TANK_NOT_SELECTED) {
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
                  break;
            }
         }
      }

      
      if(p2_tank_num == TANK_NOT_SELECTED) {
         controller_data2 = snesData(SNES_P2);
         switch(controller_data2) {
            case SNES_A_BTN:
               p2_tank_num = 0;
               break;
            case SNES_B_BTN:
               p2_tank_num = 1;
               break;
            case SNES_X_BTN:
               p2_tank_num = 2;
               break;
            case SNES_Y_BTN:
               p2_tank_num = 3;
               break;
            default:
               p2_tank_num = TANK_NOT_SELECTED;
               break;
         }
         if(p2_tank_num != TANK_NOT_SELECTED) {
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
                  break;
            }
         }         
      }
      
      _delay_ms(17);
   }
   _delay_ms(1000);
   vSpriteDelete(p1);
   vSpriteDelete(p2);
   vSpriteDelete(select_screen);
}
																															