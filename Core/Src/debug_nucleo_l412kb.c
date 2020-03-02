#include "board.h"
#include "stdout.h"
#include "fifo.h"

typedef struct _Array{
	uint8_t *data;
	uint8_t len;
	uint8_t empty;
}Array;

static fifo_t serial_tx_fifo;
static fifo_t serial_rx_fifo;
static uint8_t rx_data;

void usart_putchar(char c){
	if(HAL_UART_Transmit_IT(&huart2, (uint8_t*)&c, 1) != HAL_OK){
		fifo_put(&serial_tx_fifo, *(uint8_t*)&c);
	}
}

static void usart_puts(const char* str){
uint32_t len = 0;
	while( *((const char*)(str + len)) != '\0'){ len++; }
	if(HAL_UART_Transmit_IT(&huart2, ((uint8_t*)str), len) != HAL_OK){
		while(len--){
			fifo_put(&serial_tx_fifo, *(uint8_t*)str++);
		}
	}
}  

static char usart_getchar(void){
    uint8_t c;
    HAL_UART_Receive(&huart2, &c, 1, HAL_MAX_DELAY);
    return (char)c;
}

static uint8_t usart_getCharNonBlocking(char *c){    
    if(fifo_avail(&serial_rx_fifo))
        return fifo_get(&serial_rx_fifo, (uint8_t*)c);
    return 0;
}

static uint8_t usart_kbhit(void){
    return fifo_avail(&serial_rx_fifo);
}

void debug_io_init(void){
	fifo_init(&serial_tx_fifo);
	fifo_init(&serial_rx_fifo);
	HAL_UART_Receive_IT(&huart2, &rx_data, 1);
}

stdout_t pcom = {
    .init = debug_io_init,
    .xgetchar = usart_getchar,
    .xputchar = usart_putchar,
    .xputs = usart_puts,
    .getCharNonBlocking = usart_getCharNonBlocking,
    .kbhit = usart_kbhit,
    .user_ctx = NULL
};

void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart){
	if(huart == &huart2){
		fifo_put(&serial_rx_fifo, rx_data);
		HAL_UART_Receive_IT(&huart2, &rx_data, 1);
	}
}

void HAL_UART_TxCpltCallback(UART_HandleTypeDef *huart){
	if(huart == &huart2){
		if(fifo_avail(&serial_tx_fifo)){
			uint8_t c;
			fifo_get(&serial_tx_fifo, &c);
			HAL_UART_Transmit_IT(&huart2, (uint8_t*)&c, 1);
		}
	}
}

