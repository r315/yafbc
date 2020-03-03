// =============================================================================
//
// Arduino - Flappy Bird clone
// ---------------------------
// by Themistokle "mrt-prodz" Benetatos
//
// This is a Flappy Bird clone made for the ATMEGA328 and a Sainsmart 1.8" TFT
// screen (ST7735). It features an intro screen, a game over screen containing
// the player score and a similar gameplay from the original game.
//
// Developed and tested with an Arduino UNO and a Sainsmart 1.8" TFT screen.
//
// TODO: - debounce button ?
//
// Dependencies:
// - https://github.com/adafruit/Adafruit-GFX-Library
// - https://github.com/adafruit/Adafruit-ST7735-Library
//
// References:
// - http://www.tweaking4all.com/hardware/arduino/sainsmart-arduino-color-display/
// - http://www.koonsolo.com/news/dewitters-gameloop/
// - http://www.arduino.cc/en/Reference/PortManipulation
//
// --------------------
// http://mrt-prodz.com
// http://github.com/mrt-prodz/ATmega328-Flappy-Bird-Clone
//
// =============================================================================

#include "board.h"
#include "lcd.h"

// (we can change the size of the game easily that way)
#define TFTW             TFT_W //80     // screen width
#define TFTH             TFT_H //160    // screen height
#define TFTW2            (TFTW/2)     // half screen width
#define TFTH2            (TFTH/2)     // half screen height
// game constant
#define SPEED             1
#define GRAVITY         9.8
#define JUMP_FORCE     2.15
#define SKIP_TICKS     20.0     // 1000 / 50fps
#define MAX_FRAMESKIP     5
// bird size
#define BIRDW             8     // bird width
#define BIRDH             8     // bird height
#define BIRDW2            4     // half width
#define BIRDH2            4     // half height
// pipe size
#define PIPEW            12     // pipe width
#define GAPHEIGHT        36     // pipe gap height
// floor size
#define FLOORH           20     // floor height (from bottom of the screen)
// grass size
#define GRASSH            4     // grass height (inside floor, starts at floor y)
// Game area
#define GAMEH 			(TFTH - FLOORH)
// background
const unsigned int BCKGRDCOL = RGB565(138,235,244);
// bird
const unsigned int BIRDCOL = RGB565(255,254,174);
// pipe
const unsigned int PIPECOL  = RGB565(99,255,78);
// pipe highlight
const unsigned int PIPEHIGHCOL  = RGB565(250,255,250);
// pipe seam
const unsigned int PIPESEAMCOL  = RGB565(0,0,0);
// floor
const unsigned int FLOORCOL = RGB565(246,240,163);
// grass (col2 is the stripe color)
const unsigned int GRASSCOL  = RGB565(141,225,87);
const unsigned int GRASSCOL2 = RGB565(156,239,88);

// bird sprite
// bird sprite colors (Cx name for values to keep the array readable)
#define C0 BCKGRDCOL
#define C1 RGB565(195,165,75)
#define C2 BIRDCOL
#define C3 WHITE
#define C4 RED
#define C5 RGB565(251,216,114)

static const unsigned short bird_data[] ={
		C0, C0, C1, C1, C1, C1, C1, C0,
		C0, C1, C2, C2, C2, C1, C3, C1,
		C0, C2, C2, C2, C2, C1, C3, C1,
		C1, C1, C1, C2, C2, C3, C1, C1,
		C1, C2, C2, C2, C2, C2, C4, C4,
		C1, C2, C2, C2, C1, C5, C4, C0,
		C0, C1, C2, C1, C5, C5, C5, C0,
		C0, C0, C1, C5, C5, C5, C0, C0
};
// Grass tile
#define G0 RGB565(0x00,0x96,0x88)
#define G1 RGB565(0x4c,0xaf,0x50)
#define G2 RGB565(0xcd,0xdc,0x39)

static const unsigned short grass_data[] = {
		G0, G0, G1, G2, G0, G0, G0, G0,
		G0, G1, G2, G1, G0, G0, G0, G0,
		G1, G2, G1, G0, G0, G0, G0, G0,
		G2, G1, G0, G0, G0, G0, G0, G0,
};

// bird structure
static struct BIRD {
	signed short x, y, old_y;
	float vel_y;
} bird;

// pipe structure
static struct PIPE {
	signed short x, old_x;
	unsigned char gap_y;
} pipe;

// grass structure (for stripe animation)
static struct GRASS {
	signed short x, y;
	unsigned short *data;
} grass;

static struct GAME{
	// score
	unsigned short score;
	// game loop time variables
	unsigned int last_time, next_game_tick;
	unsigned int loops;
	// passed pipe flag to count score
	bool passed_pipe;
} flappy;

typedef enum {
	GAME_STARTING = 0,
	GAME_IDLE,
	GAME_RUNNING,
	GAME_ENDED
}gamestate_e;

// ---------------
// draw pixel
// ---------------
// faster drawPixel method by inlining calls and using setAddrWindow and pushColor
// using macro to force inlining
#define drawPixel LCD_Pixel
#define drawVline LCD_Line_V
#define drawHline LCD_Line_H
#define drawFillRect LCD_FillRect
#define fillScreen LCD_Clear
#define millis HAL_GetTick
#define delay	HAL_Delay

typedef struct{
	void (*setTextColor)(unsigned short);
	void (*setCursor)(unsigned short, unsigned short);
	void (*println)(const char* str, ...);
	void (*print)(const char* str, ...);
} tft_t;

static tft_t TFT = {
	DISPLAY_SetFcolor,
	DISPLAY_GotoAbsolute,
	DISPLAY_printf,
	DISPLAY_printf
};

int rndInt(int min, int max);
void rndIntSeed(int seed);

// ---------------
// game loop
// ---------------
gamestate_e game_loop() {
	// temporary x and y var
	unsigned short tmpx, tmpy, px;
	unsigned int now = millis();
	// this loop is some what time critical, if delayed
	// random lines will appear
	if( now > flappy.next_game_tick && flappy.loops < MAX_FRAMESKIP) {
		// ===============
		// input
		// ===============
		if ( BTN1_PRESSED ) {
			// if the bird is not too close to the top of the screen apply jump force
			if (bird.y > BIRDH2*0.5) bird.vel_y = -JUMP_FORCE;
			// else zero velocity
			else bird.vel_y = 0;
		}

		// ===============
		// update
		// ===============
		// calculate delta time
		// ---------------
		float delta = ((float)now - flappy.last_time)/1000;
		flappy.last_time = now;

		// bird
		// ---------------
		bird.vel_y += delta * GRAVITY;
		bird.y += bird.vel_y;

		// pipe
		// ---------------
		pipe.x -= SPEED;
		// if pipe reached edge of the screen reset its position and gap
		if (pipe.x < -PIPEW) {
			pipe.x = TFTW;
			pipe.gap_y = rndInt(10, GAMEH - (10 + GAPHEIGHT));
		}

		// ---------------
		flappy.next_game_tick += SKIP_TICKS;
		flappy.loops++;
		return GAME_RUNNING;
	}

	flappy.loops = 0;

	// ===============
	// draw
	// ===============
	// pipe
	// ---------------
	// we save cycles if we avoid drawing the pipe when outside the screen
	// or if pipe didn't move
	if(pipe.x != pipe.old_x){
		if (pipe.x >= 0 && pipe.x < TFTW) {
			// The pipe is formed by two lines, color and highlight, that
			// are drawn 3 pixels apart creating movement illusion on each iteration
			// by being drawn in front of the previous
			// pipe color
			drawVline(pipe.x+3, 0, pipe.gap_y, PIPECOL);
			drawVline(pipe.x+3, pipe.gap_y+GAPHEIGHT+1, GAMEH-(pipe.gap_y+GAPHEIGHT+1), PIPECOL);
			// highlight
			drawVline(pipe.x, 0, pipe.gap_y, PIPEHIGHCOL);
			drawVline(pipe.x, pipe.gap_y+GAPHEIGHT+1, GAMEH-(pipe.gap_y+GAPHEIGHT+1), PIPEHIGHCOL);
			// bottom and top border of pipe
			drawPixel(pipe.x, pipe.gap_y, PIPESEAMCOL);
			drawPixel(pipe.x, pipe.gap_y+GAPHEIGHT, PIPESEAMCOL);
			// pipe seam
			drawPixel(pipe.x, pipe.gap_y-6, PIPESEAMCOL);
			drawPixel(pipe.x, pipe.gap_y+GAPHEIGHT+6, PIPESEAMCOL);
			drawPixel(pipe.x+3, pipe.gap_y-6, PIPESEAMCOL);
			drawPixel(pipe.x+3, pipe.gap_y+GAPHEIGHT+6, PIPESEAMCOL);
		}
		// erase behind pipe
		if (pipe.x < TFTW - PIPEW)
			drawVline(pipe.x + PIPEW, 0, GAMEH, BCKGRDCOL);
		pipe.old_x = pipe.x;
	}
	// grass stripes
	// ---------------
	// Usually grass moves faster than the pipe.
	grass.x += SPEED;
	if (grass.x >= TFTW)
		grass.x = 0;
	LCD_Window(0, grass.y, TFTW, 4);
	for(unsigned short r = 0; r < 4 * 8; r += 8){
		for(unsigned short c = 0; c < TFTW; c++){
			LCD_Data(grass.data[r + ((grass.x + c) % 8)]);
		}
	}

	// bird
	// ---------------
	tmpx = BIRDW-1;
	do {
		px = bird.x + tmpx + BIRDW;
		// clear bird at previous position stored in old_y
		// we can't just erase the pixels before and after current position
		// because of the non-linear bird movement (it would leave 'dirty' pixels)
		tmpy = BIRDH - 1;
		do {
			drawPixel(px, bird.old_y + tmpy, BCKGRDCOL);
		} while (tmpy--);

		// draw bird sprite at new position
		tmpy = BIRDH - 1;
		do {
			drawPixel(px, bird.y + tmpy, bird_data[tmpx + (tmpy * BIRDW)]);
		} while (tmpy--);

	} while (tmpx--);
	// save position to erase bird on next draw
	bird.old_y = bird.y;


	// ===============
	// collision
	// ===============
	// if the bird hit the ground game over
	if (bird.y > GAMEH - BIRDH){
		return GAME_ENDED;
	}

	// checking for bird collision with pipe
	if (bird.x+BIRDW >= pipe.x-BIRDW2 && bird.x <= pipe.x+PIPEW-BIRDW) {
		// bird entered a pipe, check for collision
		if (bird.y < pipe.gap_y || bird.y+BIRDH > pipe.gap_y+GAPHEIGHT){
			return GAME_ENDED;
		}else {
			flappy.passed_pipe = true;
		}
	}
	// if bird has passed the pipe increase score
	else if (bird.x > pipe.x+PIPEW-BIRDW && flappy.passed_pipe) {
		flappy.passed_pipe = false;
		// erase score with background color
		TFT.setTextColor(BCKGRDCOL);
		TFT.setCursor( TFTW2, 4);
		TFT.print("%d",flappy.score);
		// set text color back to white for new score
		TFT.setTextColor(WHITE);
		// increase score since we successfully passed a pipe
		flappy.score++;
	}
	// update score
	// ---------------
	TFT.setCursor( TFTW2, 4);
	TFT.print("%d",flappy.score);
	return GAME_RUNNING;
}

// ---------------
// game init
// ---------------
void game_init() {
	// clear screen
	LCD_Clear(BCKGRDCOL);
	// reset score
	flappy.score = 0;
	// init bird
	bird.x = 20;
	bird.y = bird.old_y = TFTH2 - BIRDH;
	bird.vel_y = -JUMP_FORCE;
	// generate new random seed for the pipe gape
	rndIntSeed(millis());
	// init pipe
	pipe.x = TFTW;
	pipe.gap_y = rndInt(10, GAMEH - (10 + GAPHEIGHT));
	// init grass
	grass.x = TFTW;
	grass.y = GAMEH+1;
	grass.data = (unsigned short*)grass_data;

	// ===============
	// prepare game variables
	// draw floor
	// ===============
	// draw the floor once, we will not overwrite on this area in-game
	// black line
	drawHline(0, GAMEH, TFTW, BLACK);
	// black line
	drawHline(0, GAMEH + GRASSH + 1, TFTW, BLACK);
	// mud
	drawFillRect(0, GAMEH+GRASSH + 2, TFTW, FLOORH-GRASSH, FLOORCOL);

	flappy.passed_pipe = false;
	flappy.last_time = flappy.next_game_tick = millis();
	flappy.loops = 0;
}

// ---------------
// game start
// ---------------
gamestate_e game_start() {
	fillScreen(BLACK);
	drawHline(10, TFTH2 - 20, TFTW - 20, WHITE);
	drawHline(10, TFTH2 + 20, TFTW - 20, WHITE);
	TFT.setTextColor(WHITE);
	//TFT.setTextSize(3);
	// half width - num char * char width in pixels
	TFT.setCursor( TFTW2 - (3*8), TFTH2 - 8);
	TFT.println("FLAPPY");
	//TFT.setTextSize(3);
	TFT.setCursor( TFTW2 - (3*8), TFTH2 + 8);
	TFT.println("-BIRD-");
	//TFT.setTextSize(0);
	TFT.setCursor( 10, TFTH2 - 28);
	//TFT.println("ATMEGA328");
	TFT.setCursor( TFTW2 - (12*3) - 1, TFTH2 + 34);
	TFT.println("press button");
	return GAME_IDLE;
}

// ---------------
// game over
// ---------------
gamestate_e game_over() {
	delay(1200);
	fillScreen(BLACK);
	TFT.setTextColor(WHITE);
	//TFT.setTextSize(2);
	// half width - num char * char width in pixels
	TFT.setCursor( TFTW2 - (4*8), TFTH2 - 4);
	TFT.println("GAME OVER");
	//TFT.setTextSize(0);
	TFT.setCursor( 10, TFTH2 - 14);
	TFT.print("score: %d",flappy.score);
	TFT.setCursor( TFTW2 - (5*8), TFTH2 + 12);
	TFT.println("press button");
	return GAME_IDLE;
}

// ---------------
// initial setup
// ---------------
void FLAPPY_Init() {
	// Nothing to add here
}

// ---------------
// main loop
// ---------------
static gamestate_e game_state = GAME_STARTING;
void FLAPPY_Loop() {

	switch(game_state){
	case GAME_IDLE:
		// wait for push button
		if (BTN1_PRESSED){
			// init game settings
			game_init();
			game_state = GAME_RUNNING;
		}
		break;
	case GAME_STARTING:	// starting
		game_state = game_start();
		break;
	case GAME_RUNNING:
		game_state = game_loop();
		break;
	case GAME_ENDED:
		game_state = game_over();
		break;
	}
}
