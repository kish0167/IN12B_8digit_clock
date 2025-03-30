#include "pico/stdlib.h"
#include <stdio.h>
#include "pico/multicore.h"
#include "hardware/uart.h"
#include "hardware/irq.h"


#define UART_ID uart0
#define BAUD_RATE 9600
#define TX_PIN 0
#define RX_PIN 1


// PCB constants 
uint lamps[4] = {3,6,7,8};
uint bcdA[4] = {9,10,11,12};
uint bcdB[4] = {14,15,28,29};
uint dotA = 5;
uint dotB = 2;
int bcdAmap[10][4] = {
    {1, 0, 1, 0},
    {0, 0, 1, 0},
    {1, 1, 0, 0},
    {1, 0, 1, 1},
    {0, 1, 0, 0},
    {1, 0, 0, 1},
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 0, 0, 1},
    {0, 0, 1, 1}};
int bcdBmap[10][4] = {
    {1, 0, 0, 1},
    {0, 0, 0, 0},
    {1, 0, 0, 0},
    {0, 0, 0, 1},
    {0, 0, 1, 1},
    {1, 0, 1, 0},
    {0, 0, 1, 0},
    {1, 1, 0, 0},
    {1, 0, 1, 1},
    {0, 1, 0, 0}};
uint tx = 0;
uint rx = 1;    
uint conv200V_shutdown = 13;



// midcore data
volatile int display[8] = {0,0,0,0,0,0,0,0};
volatile int dots[8] = {0,0,0,0,0,0,0,0};
volatile bool cycle_mode = false; // false - 8-cycle, true - 4-cycle



// CORE1 FUNCTIONS //

const int cycle_8_delay1_us = 2000;
const int cycle_8_delay2_us = 400;
const int cycle_4_delay1_us = 4000;
const int cycle_4_delay2_us = 500;

void bcdA_set(int num){

    if(num < 0 || num > 9){
        gpio_put(bcdA[0], 0);
        gpio_put(bcdA[1], 1);
        gpio_put(bcdA[2], 1);
        gpio_put(bcdA[3], 0);
        return;
    }

    gpio_put(bcdA[0], bcdAmap[num][0]);
    gpio_put(bcdA[1], bcdAmap[num][1]);
    gpio_put(bcdA[2], bcdAmap[num][2]);
    gpio_put(bcdA[3], bcdAmap[num][3]);
}

void bcdB_set(int num){

    if(num < 0 || num > 9){
        gpio_put(bcdB[0], 0);
        gpio_put(bcdB[1], 1);
        gpio_put(bcdB[2], 1);
        gpio_put(bcdB[3], 0);
        return;
    }

    gpio_put(bcdB[0], bcdBmap[num][0]);
    gpio_put(bcdB[1], bcdBmap[num][1]);
    gpio_put(bcdB[2], bcdBmap[num][2]);
    gpio_put(bcdB[3], bcdBmap[num][3]);
}


void cycle_8(){

    bcdA_set(-1);
    gpio_put(dotA, 0);

    gpio_put(lamps[3], 0);
    gpio_put(dotB, 0);
    sleep_us(cycle_8_delay2_us);
    gpio_put(lamps[0], 1);
    gpio_put(dotB, dots[0]);
    bcdB_set(display[0]);
    sleep_us(cycle_8_delay1_us);

    for (int i = 0; i < 3; i++){
        gpio_put(lamps[i], 0);
        gpio_put(dotB, 0);
        sleep_us(cycle_8_delay2_us);
        gpio_put(lamps[i+1], 1);
        gpio_put(dotB, dots[i+1]);
        bcdB_set(display[i+1]);
        sleep_us(cycle_8_delay1_us);
    }

    bcdB_set(-1);
    gpio_put(dotB, 0);

    gpio_put(lamps[3], 0);
    gpio_put(dotA, 0);
    sleep_us(cycle_8_delay2_us);
    gpio_put(lamps[0], 1);
    gpio_put(dotA, dots[4]);
    bcdA_set(display[4]);
    sleep_us(cycle_8_delay1_us);

    for (int i = 0; i < 3; i++){
        gpio_put(lamps[i], 0);
        gpio_put(dotA, 0);
        sleep_us(cycle_8_delay2_us);
        gpio_put(lamps[i+1], 1);
        gpio_put(dotA, dots[i+5]);
        bcdA_set(display[i+5]);
        sleep_us(cycle_8_delay1_us);
    }
}

void cycle_4(){

    gpio_put(lamps[3], 0);
    sleep_us(cycle_4_delay2_us);
    gpio_put(lamps[0], 1);
    gpio_put(dotB, dots[0]);
    gpio_put(dotA, dots[4]);
    bcdB_set(display[0]);
    bcdA_set(display[4]);
    sleep_us(cycle_4_delay1_us);

    for (int i = 0; i < 3; i++){
        gpio_put(lamps[i], 0);
        gpio_put(dotB, 0);
        gpio_put(dotA, 0);
        sleep_us(cycle_4_delay2_us);
        gpio_put(lamps[i+1], 1);
        gpio_put(dotB, dots[i+1]);
        gpio_put(dotA, dots[i+5]);
        bcdB_set(display[i+1]);
        bcdA_set(display[i+5]);
        sleep_us(cycle_4_delay1_us);
    }
}

void bcd_init_pins(){

    for (int i = 0; i < 4; i++){
        gpio_init(lamps[i]);
        gpio_init(bcdA[i]);
        gpio_init(bcdB[i]);
        gpio_set_dir(lamps[i], GPIO_OUT);
        gpio_set_dir(bcdA[i], GPIO_OUT);
        gpio_set_dir(bcdB[i], GPIO_OUT);
        gpio_put(lamps[i], 0);
        gpio_put(bcdA[i], 0);
        gpio_put(bcdB[i], 0);
    }

    gpio_init(dotA);
    gpio_init(dotB);
    gpio_set_dir(dotA, GPIO_OUT);
    gpio_set_dir(dotB, GPIO_OUT);
    gpio_put(dotA, 0);
    gpio_put(dotB, 0);
}

void core1_entry() {

    bcd_init_pins();

    while(true){
        if (cycle_mode){
            cycle_4();
        } else {
            cycle_8();
        }
    }
}

////////////////////////

void converter200V_init(){
    gpio_init(conv200V_shutdown);
    gpio_set_dir(conv200V_shutdown, GPIO_OUT);
    gpio_put(conv200V_shutdown,1);
}

void converter200V_shutdown(){
    gpio_put(conv200V_shutdown,1);
}

void converter200V_enable(){
    gpio_put(conv200V_shutdown,0);
}

void display_int(int num){
    num %= 100000000;
    int mask = 10000000;
    for (int i = 0; i < 8; i++){
        display[i] = num / mask;
        num %= mask;
        mask /= 10;
    }
}

#define BUF_SIZE 256
uint8_t rx_buf[BUF_SIZE];
volatile uint16_t rx_count = 0;

void on_uart_rx() {
    while (uart_is_readable(UART_ID)) {
        uint8_t ch = uart_getc(UART_ID);
        
        if (rx_count < BUF_SIZE) {
            rx_buf[rx_count++] = ch;
        }
    }
}

void bte_init(){
    uart_init(UART_ID, BAUD_RATE);
    gpio_set_function(TX_PIN, GPIO_FUNC_UART);
    gpio_set_function(RX_PIN, GPIO_FUNC_UART);
    uart_set_format(UART_ID, 8, 1, UART_PARITY_NONE);

    irq_set_exclusive_handler(UART0_IRQ, on_uart_rx);
    irq_set_enabled(UART0_IRQ, true);
    uart_set_irq_enables(UART_ID, true, false);
}

void clock_cycle(){

    while (true){
        sleep_ms(10);
        display[7] += 1;
        display[6] += display[7] / 10;
        display[7] %= 10;
    
        display[5] += display[6] / 10;
        display[6] %= 10;
    
        display[4] += display[5] / 10;
        display[5] %= 10;

        display[3] += display[4] / 6;
        display[4] %= 6;

        display[2] += display[3] / 10;
        display[3] %= 10;

        display[1] += display[2] / 6;
        display[2] %= 6;

        display[0] += display[1] / 10;
        display[1] %= 10;

        if (display[0] == 2 && display[1] == 4) {
            display[0] = 0;
            display[1] = 0;
            display[2] = 0;
            display[3] = 0;
            display[5] = 0;
            display[6] = 0;
            display[7] = 0;
        }
    }
}

bool rx_buf_end_check(){
    if (rx_count < 16)
        return false;
    if (rx_buf[rx_count-6] != 'e')
        return false;
    if (rx_buf[rx_count-5] != 'n')
        return false;        
    if (rx_buf[rx_count-4] != 'd')
        return false;
    if (rx_buf[rx_count-3] != '.')
        return false;

    return true;
}

int main() {

    stdio_init_all();
    gpio_init(25);
    gpio_set_dir(25, GPIO_OUT);

    bte_init();

    multicore_launch_core1(core1_entry);
    converter200V_init();
    converter200V_enable();

    while (1) {

        if (!rx_buf_end_check()){
            tight_loop_contents();
            continue;
        }

        for (int i = 0; i < 8; i++)
            display[i] = rx_buf[rx_count + i - 16] - '0';

        uint8_t mask = 0x08;    
        for (int i = 0; i < 4; i++){
            dots[i] = (mask & rx_buf[rx_count - 8]) != 0 ? 1 : 0;
            mask = mask >> 1;
        }
        mask = 0x08;   
        for (int i = 4; i < 8; i++){
            dots[i] = (mask & rx_buf[rx_count - 7]) != 0 ? 1 : 0;
            mask = mask >> 1;
        }

        rx_count = 0;

        tight_loop_contents();
    }
    
    return 0;
}
