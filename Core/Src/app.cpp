
#include <stdlib.h>
#include <stdio.h>
#include "lcd.h"
#include "console.h"
#include "board.h"

uint32_t nLehmer = 0;
uint32_t Lehmer32(void){
	nLehmer += 0xE120FC15;
	uint64_t tmp = (uint64_t)nLehmer * 0x4A39B70D;
	uint32_t m1 = (tmp >> 32) ^ tmp;
	tmp = (uint64_t)m1 * 0x12FAD5C9;
	uint32_t m2 = (tmp >> 32) ^ tmp;
	return m2;
}

int rndInt(int min, int max){
	return (Lehmer32() % (max - min)) + min;
}

void rndIntSeed(int seed){
	nLehmer = seed;
}

uint32_t xrand(void){
	CRC->DR = HAL_GetTick();
	return CRC->DR;
}

class CmdHelp : public ConsoleCommand {
	Console *console;
public:
    CmdHelp() : ConsoleCommand("help") {}
	void init(void *params) { console = static_cast<Console*>(params); }

	void help(void) {
		console->xputs("Available commands:\n\n");

		for (uint8_t i = 0; i < console->getCmdListSize(); i++) {
				console->print("\t%s\n", console->getCmdIndexed(i)->getName());
		}
		console->xputchar('\n');
	}

	char execute(void *ptr) {
		help();
		return CMD_OK;
	}
};



class CmdLcd : public ConsoleCommand {
	Console *console;
public:
    CmdLcd() : ConsoleCommand("lcd") {}
	void init(void *params) { console = static_cast<Console*>(params); }

	void help(void) {

	}

	char execute(void *ptr) {
		char *p1 = (char*)ptr;
		uint8_t args[5], idx = 0;
		uint32_t val;


		if( p1 == NULL || *p1 == '\0'){
			help();
			return CMD_OK;
		}

		while(*p1 != '\0'){
			if(nextHex((char**)&p1, &val)){
				args[idx++] = (uint8_t)val;
			}else{
				p1 = nextWord(p1);
			}
		}

		LCD_CS0;
		LCD_CD0;
		SPI_Send(args[0]);
		LCD_CD1;
		for(uint8_t i = 1; i < idx; i++){
			SPI_Send(args[i]);
		}
		LCD_CS1;

		return CMD_OK;
	}
};

class CmdTest : public ConsoleCommand {
	Console *console;
public:
    CmdTest() : ConsoleCommand("test") {}
	void init(void *params) { console = static_cast<Console*>(params); }

	void help(void) {

	}

	char execute(void *ptr) {
		LCD_Window(0,0, LCD_GetWidth(), LCD_GetHeight());
			for(uint8_t y = 0; y < LCD_GetHeight(); y++){
				for(uint8_t x = 0; x < LCD_GetWidth(); x++){
					uint16_t seed = y << 8 | x;
					//srand(seed);

					//nLehmer = seed;
					//seed = Lehmer32();

					//seed = HAL_CRC_Accumulate(&hcrc, (uint32_t*)&seed, 1);

					//seed = xrand();

					seed = rndInt(10, 140 - 46);

					uint16_t color = (seed % 256) < 32 ? WHITE : BLACK;
					LCD_Pixel(x,y, color);
				}
			}

		return CMD_OK;
	}
};

void FLAPPY_Init(void);
void FLAPPY_Loop(void);

extern "C"
void App(){
	Console con;
	CmdHelp hlp;
	CmdLcd lcd;
	CmdTest test;

	con.init(&pcom, "NUCLEO >");
	con.addCommand(&hlp);
	con.addCommand(&lcd);
	con.addCommand(&test);
	con.cls();

	FLAPPY_Init();
	DISPLAY_SetAttribute(FONT_TRANSPARENT);

	while(1){
		con.process();
		FLAPPY_Loop();
	}
}
