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

//Asteroid images
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




#define DEG_TO_RAD M_PI / 180.0

#define INITIAL_ASTEROIDS 1
#define SCREEN_W 800
#define SCREEN_H 600

#define DEAD_ZONE_OVER_2 120

#define FRAME_DELAY_MS  10
#define BULLET_DELAY_MS 500
#define BULLET_LIFE_MS  1000

#define SHIP_SIZE 50
#define BULLET_SIZE 26
#define AST_SIZE_3 100
#define AST_SIZE_2 40
#define AST_SIZE_1 15

#define BULLET_VEL 6.0

#define SHIP_MAX_VEL 8.0
#define SHIP_ACCEL 0.1
#define SHIP_AVEL  6.0

#define BACKGROUND_AVEL 0.01

#define AST_MAX_VEL_3 2.0
#define AST_MAX_VEL_2 2.0
#define AST_MAX_VEL_1 2.0
#define AST_MAX_AVEL_3 2.0
#define AST_MAX_AVEL_2 2.0
#define AST_MAX_AVEL_1 2.0

static xTaskHandle inputTaskHandle;
static xTaskHandle bulletTaskHandle;
static xTaskHandle updateTaskHandle;

//Mutex used synchronize usart usage
static xSemaphoreHandle usartMutex;

static xSemaphoreHandle xSnesMutex;
static object ship1;
static object ship2;

uint8_t fire_button1 = 0;
uint8_t fire_button2 = 0;

//linked lists for asteroids and bullets
static object *bullets = NULL;
static object *asteroids = NULL;

static xGroupHandle astGroup;
static xSpriteHandle background;

void init(void);
void reset(void);
int16_t getRandStartPosVal(int16_t dimOver2);
object *createAsteroid(float x, float y, float velx, float vely, int16_t angle, int8_t avel, int8_t size, object *nxt);
uint16_t sizeToPix(int8_t size);
object *createBullet(float x, float y, float velx, float vely, int16_t angle, object *nxt);
void spawnAsteroid(point *pos, uint8_t size);


//void controllerTask(void *vParam) {
	//// variable to hold ticks value of last task run
	//portTickType xLastWakeTime;
	//
	//// Initialize the xLastWakeTime variable with the current time.
	//xLastWakeTime = xTaskGetTickCount();
	//snesInit()
	//
	//for(;;)
	//{
		//uint16_t controller_data;
		//snesInit();
		//
		//while (1) {
			////xQueueReceive( xSnesDataQueue, &controller_data, portMAX_DELAY );
			//controller_data = snesData();
		//vTaskDelay(250/portTICK_RATE_MS);
	//}
//}	
//
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
	vTaskDelay(1000/portTICK_RATE_MS);
	
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
      else
         ship1.accel =0;
   
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
      else
         ship2.accel =0;
      
      if(controller_data2 & SNES_Y_BTN)
         fire_button2 = 1;
      //xSemaphoreGive(xSnesMutex);
      vTaskDelayUntil( &xLastWakeTime, 17/portTICK_RATE_MS);
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
	portTickType xLastWakeTime;
   
	// Initialize the xLastWakeTime variable with the current time.
	xLastWakeTime = xTaskGetTickCount();

   while (1) {
      if(fire_button1 || fire_button2) {
         xSemaphoreTake(usartMutex, portMAX_DELAY);
         if(fire_button1)
         {     
            fire_button1 = 0;
            //Make a new bullet and add to linked list
            bullets = createBullet(
            ship1.pos.x,
            ship1.pos.y,
            -sin(ship1.angle*DEG_TO_RAD)*BULLET_VEL,
            -cos(ship1.angle*DEG_TO_RAD)*BULLET_VEL,
            ship1.angle,
            bullets);
         }
         if(fire_button2)
         {
            fire_button2 = 0;
            //Make a new bullet and add to linked list
            bullets = createBullet(
            ship2.pos.x,
            ship2.pos.y,
            -sin(ship2.angle*DEG_TO_RAD)*BULLET_VEL,
            -cos(ship2.angle*DEG_TO_RAD)*BULLET_VEL,
            ship2.angle,
            bullets);
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
		
		if (ship1.pos.x < 0.0) {
			ship1.pos.x += SCREEN_W;
		} else if (ship1.pos.x > SCREEN_W) {
			ship1.pos.x -= SCREEN_W;
		}
		if (ship1.pos.y < 0.0) {
			ship1.pos.y += SCREEN_H;
		} else if (ship1.pos.y > SCREEN_H) {
			ship1.pos.y -= SCREEN_H;
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

      if (ship2.pos.x < 0.0) {
         ship2.pos.x += SCREEN_W;
      } else if (ship2.pos.x > SCREEN_W) {
         ship2.pos.x -= SCREEN_W;
      }
      if (ship2.pos.y < 0.0) {
         ship2.pos.y += SCREEN_H;
      } else if (ship2.pos.y > SCREEN_H) {
         ship2.pos.y -= SCREEN_H;
      }
      
		// move bullets
		objPrev = NULL;
		objIter = bullets;
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
					bullets = objIter->next;
					vPortFree(objIter);
					objIter = bullets;
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
		
		// move asteroids
		objPrev = NULL;
		objIter = asteroids;
      //iterate through asteroids linked list
		while (objIter != NULL) {
         //update position from velocity
			objIter->pos.x += objIter->vel.x;
			objIter->pos.y += objIter->vel.y;
         
         //wrap around horizontal edges
			if (objIter->pos.x < 0.0) {
				objIter->pos.x += SCREEN_W;
			} else if (objIter->pos.x > SCREEN_W) {
				objIter->pos.x -= SCREEN_W;
			}
         
         //wrap around vertical edges
			if (objIter->pos.y < 0.0) {
				objIter->pos.y += SCREEN_H;
			} else if (objIter->pos.y > SCREEN_H) {
				objIter->pos.y -= SCREEN_H;
			}
         
         //move to the next asteroid on the list
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
	object *objIter, *objPrev, *astIter, *astPrev, *objTemp;
	xSpriteHandle hit, handle;
	point pos;
	uint8_t size;
	
	vTaskSuspend(updateTaskHandle);
	vTaskSuspend(bulletTaskHandle);
	vTaskSuspend(inputTaskHandle);
	init();
	vTaskResume(updateTaskHandle);
	vTaskResume(bulletTaskHandle);
	vTaskResume(inputTaskHandle);
	
	for (;;) {
		xSemaphoreTake(usartMutex, portMAX_DELAY);
		vSpriteSetRotation(ship1.handle, (uint16_t)ship1.angle);
		vSpriteSetPosition(ship1.handle, (uint16_t)ship1.pos.x, (uint16_t)ship1.pos.y);

      vSpriteSetRotation(ship2.handle, (uint16_t)ship2.angle);
      vSpriteSetPosition(ship2.handle, (uint16_t)ship2.pos.x, (uint16_t)ship2.pos.y);
		objPrev = NULL;
		objIter = bullets;
		while (objIter != NULL) {
   		vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
   		if (uCollide(objIter->handle, astGroup, &hit, 1) > 0) {
      		vSpriteDelete(objIter->handle);
      		
      		if (objPrev != NULL) {
         		objPrev->next = objIter->next;
         		vPortFree(objIter);
         		objIter = objPrev->next;
      		} else {
         		bullets = objIter->next;
         		vPortFree(objIter);
         		objIter = bullets;
      		}
      		astPrev = NULL;
      		astIter = asteroids;
      		while (astIter != NULL) {
         		if (astIter->handle == hit) {
            		pos = astIter->pos;
            		size = astIter->size;
            		vSpriteDelete(astIter->handle);
            		if (astPrev != NULL) {
               		astPrev->next = astIter->next;
               		vPortFree(astIter);
               		astIter = astPrev->next;
            		} else {
               		asteroids = astIter->next;
               		vPortFree(astIter);
            		}
            		spawnAsteroid(&pos, size);
            		break;
            		
         		} else {
            		astPrev = astIter;
            		astIter = astIter->next;
         		}
      		}
   		} else {
      		objPrev = objIter;
      		objIter = objIter->next;
   		}
		}
		
		objIter = asteroids;
		while (objIter != NULL) {
   		vSpriteSetPosition(objIter->handle, (uint16_t)objIter->pos.x, (uint16_t)objIter->pos.y);
   		vSpriteSetRotation(objIter->handle, objIter->angle);
   		objIter = objIter->next;
		}
		
		if (uCollide(ship1.handle, astGroup, &hit, 1) > 0 || asteroids == NULL || uCollide(ship2.handle, astGroup, &hit, 1) > 0) {
   		vTaskSuspend(updateTaskHandle);
   		vTaskSuspend(bulletTaskHandle);
   		vTaskSuspend(inputTaskHandle);
   		
   		if (asteroids == NULL)
   		handle = xSpriteCreate("win.png", SCREEN_W>>1, SCREEN_H>>1, 20, SCREEN_W>>1, SCREEN_H>>1, 100);
   		else
   		handle = xSpriteCreate("lose.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W>>1, SCREEN_H>>1, 100);
   		
   		vTaskDelay(3000 / portTICK_RATE_MS);
   		vSpriteDelete(handle);
   		
   		reset();
   		init();
   		
   		vTaskResume(updateTaskHandle);
   		vTaskResume(bulletTaskHandle);
   		vTaskResume(inputTaskHandle);
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
	//xTaskCreate(inputTask, (signed char *) "p2", 80, (void*)&player2, 6, &inputTaskHandle);
	xTaskCreate(bulletTask, (signed char *) "b", 250, NULL, 2, &bulletTaskHandle);
	xTaskCreate(updateTask, (signed char *) "u", 200, NULL, 4, &updateTaskHandle);
	xTaskCreate(drawTask, (signed char *) "d", 600, NULL, 3, NULL);
	xTaskCreate(USART_Write_Task, (signed char *) "w", 200, NULL, 5, NULL);
	
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
	int i;
	
	bullets = NULL;
	asteroids = NULL;
	astGroup = ERROR_HANDLE;
	
	background = xSpriteCreate("stars.png", SCREEN_W>>1, SCREEN_H>>1, 0, SCREEN_W, SCREEN_H, 0);
	
	srand(TCNT0);
	
	astGroup = xGroupCreate();
	
	for (i = 0; i < INITIAL_ASTEROIDS; i++) {
		asteroids = createAsteroid(
         getRandStartPosVal(SCREEN_W >> 1),
         getRandStartPosVal(SCREEN_H >> 1),
         (rand() % (int8_t)(AST_MAX_VEL_3 * 10)) / 5.0 - AST_MAX_VEL_3,
         (rand() % (int8_t)(AST_MAX_VEL_3 * 10)) / 5.0 - AST_MAX_VEL_3,
         rand() % 360,
         (rand() % (int8_t)(AST_MAX_AVEL_3 * 10)) / 5.0 - AST_MAX_AVEL_3,
         3,
         asteroids);
	}
	
   // Ship1 create
	ship1.handle = xSpriteCreate(
      "ship2.png", 
      SCREEN_W >> 1,
      SCREEN_H >> 1, 
      0, 
      SHIP_SIZE, 
      SHIP_SIZE, 
      1);
   
	ship1.pos.x = SCREEN_W >> 2;
	ship1.pos.y = SCREEN_H >> 1;
	ship1.vel.x = 0;
	ship1.vel.y = 0;
	ship1.accel = 0;
	ship1.angle = 0;
	ship1.a_vel = 0;

   // Ship2 create
   ship2.handle = xSpriteCreate(
      "ship.png",
      SCREEN_W >> 1,
      SCREEN_H >> 1,
      0,
      SHIP_SIZE,
      SHIP_SIZE,
      1);
   
   ship2.pos.x = SCREEN_W >> 1;
   ship2.pos.y = SCREEN_H >> 1;
   ship2.vel.x = 0;
   ship2.vel.y = 0;
   ship2.accel = 0;
   ship2.angle = 0;
   ship2.a_vel = 0;
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
   
	// removes asteroids
	while (asteroids != NULL) {
		vSpriteDelete(asteroids->handle);
		nextObject = asteroids->next;
		vPortFree(asteroids);
		asteroids = nextObject;
	}
	vGroupDelete(astGroup);
	
	// removes bullets
	while (bullets != NULL) {
   	vSpriteDelete(bullets->handle);
   	nextObject = bullets->next;
   	vPortFree(bullets);
   	bullets = nextObject;
	}
   
   //removes the ship
   vSpriteDelete(ship1.handle);
   vSpriteDelete(ship2.handle);
   //removes the background
   vSpriteDelete(background);
}

/*------------------------------------------------------------------------------
 * Function: getRandStartPosVal
 *
 * Description: This function calculates a safe starting coordinate for an
 *  asteroid given the half extent of the current window.
 *
 * param dimOver2: Half of the dimension of the window for which a random
 *  coordinate value is desired.
 * return: A safe, pseudorandom coordinate value.
 *----------------------------------------------------------------------------*/
int16_t getRandStartPosVal(int16_t dimOver2) {
   return rand() % (dimOver2 - DEAD_ZONE_OVER_2) + (rand() % 2) * (dimOver2 + DEAD_ZONE_OVER_2);
}

/*------------------------------------------------------------------------------
 * Function: createAsteroid
 *
 * Description: This function creates a new asteroid object with a random
 *  sprite image.
 *
 * param x: The starting x position of the asteroid in window coordinates.
 * param y: The starting y position of the asteroid in window coordinates.
 * param velx: The starting x velocity of the asteroid in pixels per frame.
 * param vely: The starting y velocity of the asteroid in pixels per frame.
 * param angle: The starting angle of the asteroid in degrees.
 * param avel: The starting angular velocity of the asteroid in degrees per
 *  frame.
 * param size: The starting size of the asteroid. Must be in the range [1,3].
 * param nxt: A pointer to the next asteroid object in a linked list.
 * return: A pointer to a malloc'd asteroid object. Must be freed by the calling
 *  process.
 *----------------------------------------------------------------------------*/
object *createAsteroid(float x, float y, float velx, float vely, int16_t angle, int8_t avel, int8_t size, object *nxt) {
	/* ToDo:
     * Create a new asteroid object using a reentrant malloc() function
     * Setup the pointers in the linked list using:
     * asteroid->next = nxt;
     * Create a new sprite using xSpriteCreate()
     * Add new asteroid to the group "astGroup" using:
     *	vGroupAddSprite() 
     */
      //allocate space for a new asteroid
      object *newAsteroid = pvPortMalloc(sizeof(object));
      
      //setup asteroid sprite
      newAsteroid->handle = xSpriteCreate(
         astImages[rand() % 3],  //reference to png filename
         x,                      //xPos
         y,                      //yPos
         angle,                  //rAngle
         sizeToPix(size),        //width
         sizeToPix(size),        //height
         1);                     //depth
      
      //set position
      newAsteroid->pos.x = x;
      newAsteroid->pos.y = y;
      //set velocity
      newAsteroid->vel.x = velx;
      newAsteroid->vel.y = vely;
      //set angle
      newAsteroid->angle = angle;
      //set angular acceleration
      newAsteroid->a_vel = avel;
      newAsteroid->size = size;
      //link to asteroids list
      newAsteroid->next = asteroids;
      //add to asteroids sprite group
      vGroupAddSprite(astGroup, newAsteroid->handle);
      
      //return pointer to new asteroid
      return newAsteroid;
}

/*------------------------------------------------------------------------------
 * Function: sizeToPix
 *
 * Description: This function converts a size enumeration value in the range
 *  [1,3] to its corresponding pixel dimension.
 *
 * param size: A number in the range [1-3]
 * return: A pixel size which may be used to appropriately scale an asteroid
 *  sprite.
 *----------------------------------------------------------------------------*/
uint16_t sizeToPix(int8_t size) {
	switch (size) {
		case 3:
		    return AST_SIZE_3;
		case 2:
		    return AST_SIZE_2;
		case 1:
		    return AST_SIZE_1;
		default:
		    return AST_SIZE_3 << 2;
	}
	return AST_SIZE_3 << 2;
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
object *createBullet(float x, float y, float velx, float vely, int16_t angle, object *nxt) {
	//Create a new bullet object using a reentrant malloc() function
	object *newBullet = pvPortMalloc(sizeof(object));
	
	//Setup the pointers in the linked list
	//Create a new sprite using xSpriteCreate()
	newBullet->handle = xSpriteCreate(
	"bullet.png",			//reference to png filename
	x,                   //xPos
	y,                   //yPos
	angle,                //rAngle
	BULLET_SIZE,			//width
	BULLET_SIZE,			//height
	1);                  //depth
   
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

/*------------------------------------------------------------------------------
 * Function: spawnAsteroid
 *
 * Description: This function decomposes a larger asteroid into three smaller
 *  ones with random velocities and appends them to the global list of
 *  asteroids.
 *
 * param pos: A pointer to the position at which the new asteroids will be
 *  created.
 * param size: The size of the asteroid being destroyed.
 *----------------------------------------------------------------------------*/
void spawnAsteroid(point *pos, uint8_t size) {
	/* ToDo:
     * Spawn 3 smaller asteroids, or no asteroids depending on the size of this asteroid
     * Use createAsteroid()
     */
   int vel, accel;
   
   if (size > 1) {
      //spawn 3 new asteroids
	   for(int i = 0; i < 3; i++) {
         //set max velocity for the new size
         switch (size - 1) {
            case 2:
            vel = AST_MAX_VEL_2;
            accel = AST_MAX_AVEL_2;
            break;
            case 1:
            vel = AST_MAX_VEL_1;
            accel = AST_MAX_AVEL_1;
            break;
            default:
            vel = AST_MAX_VEL_3;
            accel = AST_MAX_AVEL_3;
            break;
         }
      
         asteroids = createAsteroid(
            pos->x,                                         //x pos
            pos->y,                                         //y pos
            (rand() % (int8_t)(vel * 10)) / 5.0 - vel,      //x vel
            (rand() % (int8_t)(vel * 10)) / 5.0 - vel,      //y vel
            rand() % 360,                                   //angle
            (rand() % (int8_t)(accel * 10)) / 5.0 - accel,  //accel
            size - 1,                                       //size
            asteroids);                                     //next asteroid
      }
   }
}																																		