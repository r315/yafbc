# Yet Another Flappy Bird clone

#### On more annoying Flappy Bird clone made with a ST NUCLEO-L412KB development board and a 0.96" 80x160 IPS screen

After acquiring a small 0.96" IPS display from ebay, I wanted to test his performance and image quality. After some googling I found mrt-prodz [ATmega328 Flappy Bird Clone](https://github.com/mrt-prodz/ATmega328-Flappy-Bird-Clone) project, which looked very nice, easy to port and had the same controller ST7735S.

The game is mostly the same as in mrt-prodz repository, only made some optimization on data structures and added a sprite for the grass.

This project was developed using STM32CubeIDE, which handles all the low level stuff with the integrated Device Configuration Tool Perspective. Although the configuration tool could add a RTOS I opted to keep thing simple and use states for the game and a debug interface.

## Features:

- Single on screen pipe Flappy Bird game that we all love.....
- Only one interface button
- Debug interface through ST-LinkV2
- Custom display controller driver

## Connections:

<pre>
Nucleo     
  A0   =>   TFT_D/C
  A1   =>   TFT_CLK
  A3   =>   TFT_CS
  A6   =>   TFT_DI
  D13  =>   TFT_RST
  D12  <=   Button
</pre>

## Devboard:

![Dev-Board](https://raw.githubusercontent.com/r315/yafbc/master/nucleo-display.jpg)

## Ingame:

![In-Game](https://raw.githubusercontent.com/r315/yafbc/master/flappy-in-game.jpg)

## Future work

There is a lot that can be done on the libraries and drivers, but one this that for sure will be an improvement 
is to take advantage of the DMA block for transferring data to the display





