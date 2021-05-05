/**
 * Simple USB HOST implementation for generic hid device (sublass 0)
 * 
 * Example inspired from Raspberry Pi Pico Examples.
 * see https://github.com/raspberrypi/pico-examples
 * 
 * Requires: https://github.com/hdo/tinyusb (Branch generic_hid_host)
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bsp/board.h"
#include "tusb.h"
#include "hardware/gpio.h"


#define GPIO_ENCODER_A  4
#define GPIO_ENCODER_B  5

#define GPIO_AXIS_X     6
#define GPIO_AXIS_Y     7
#define GPIO_AXIS_Z     8

#define GPIO_SPEED_1    10
#define GPIO_SPEED_2    11
#define GPIO_SPEED_3    12
#define GPIO_SPEED_4    13

#define GPIO_OK         14

#define GPIO_PROG_START  16
#define GPIO_PROG_STOP   17

#define GPIO_PWM_FEEDT   18
#define GPIO_PWM_SPINDLE 19


#define HALF_PULSE_LENGTH_MS 2
#define BUTTON_PULSE_LENGTH_MS 100

typedef struct TU_ATTR_PACKED
{
  uint8_t buffer[128];  
} hid_generic_hid_report_t;


void led_blinking_task(void);
extern void hid_task(void);

uint8_t gpio_init_array[] = {
    GPIO_ENCODER_A,
    GPIO_ENCODER_B,
    GPIO_AXIS_X,
    GPIO_AXIS_Y,
    GPIO_AXIS_Z,
    GPIO_SPEED_1,
    GPIO_SPEED_2,
    GPIO_SPEED_3,
    GPIO_SPEED_4,
    GPIO_OK,
    GPIO_PROG_START,
    GPIO_PROG_STOP
};

uint8_t last_axis = 0;
uint8_t last_button_code = 0;
int16_t current_encoder_value = 0;
uint8_t current_speed = 1;



void pulse_button(uint8_t pin) {
    gpio_put(pin, false);
    sleep_ms(BUTTON_PULSE_LENGTH_MS);
    gpio_put(pin, true);
}

void select_axis_pulse(uint8_t data) {
    printf("selected axis: %02X \r\n", data);
    gpio_put(GPIO_AXIS_X, true);
    gpio_put(GPIO_AXIS_Y, true);
    gpio_put(GPIO_AXIS_Z, true);
    switch(data) {
        case 0x11: pulse_button(GPIO_AXIS_X); break;
        case 0x12: pulse_button(GPIO_AXIS_Y); break;
        case 0x13: pulse_button(GPIO_AXIS_Z); break;
    }
    current_encoder_value = 0;
}

/**
 * Does not work with ESTLCAM (MEGA)
 */
void select_axis_deprecated(uint8_t data) {
    printf("selected axis: %02X \r\n", data);
    gpio_put(GPIO_AXIS_X, true);
    gpio_put(GPIO_AXIS_Y, true);
    gpio_put(GPIO_AXIS_Z, true);
    switch(data) {
        case 0x11: gpio_put(GPIO_AXIS_X, false); break;
        case 0x12: gpio_put(GPIO_AXIS_Y, false); break;
        case 0x13: gpio_put(GPIO_AXIS_Z, false); break;
    }
    current_encoder_value = 0;
}

void set_speed(uint8_t value) {
    printf("selected speed: %d \r\n", current_speed);
    gpio_put(GPIO_SPEED_1, true);
    gpio_put(GPIO_SPEED_2, true);
    gpio_put(GPIO_SPEED_3, true);
    gpio_put(GPIO_SPEED_4, true);
    switch(value) {
        case 1: gpio_put(GPIO_SPEED_1, false); break;
        case 2: gpio_put(GPIO_SPEED_2, false); break;
        case 3: gpio_put(GPIO_SPEED_3, false); break;
        case 4: gpio_put(GPIO_SPEED_4, false); break;
    }
}

void toggle_speed() {
    current_speed++;
    if (current_speed > 4) {
        current_speed = 1;
    }
    set_speed(current_speed);
}


void encoder_task() {
    if (current_encoder_value == 0) {
        return;
    }
    if (current_encoder_value > 0) {
        // CW
        gpio_put(GPIO_ENCODER_A, true);
        sleep_ms(HALF_PULSE_LENGTH_MS);
        gpio_put(GPIO_ENCODER_B, true);
        sleep_ms(HALF_PULSE_LENGTH_MS);
        gpio_put(GPIO_ENCODER_A, false);
        sleep_ms(HALF_PULSE_LENGTH_MS);
        gpio_put(GPIO_ENCODER_B, false);
        current_encoder_value--;
    } else if (current_encoder_value < 0) {
        // CCW
        gpio_put(GPIO_ENCODER_B, true);
        sleep_ms(HALF_PULSE_LENGTH_MS);
        gpio_put(GPIO_ENCODER_A, true);
        sleep_ms(HALF_PULSE_LENGTH_MS);
        gpio_put(GPIO_ENCODER_B, false);
        sleep_ms(HALF_PULSE_LENGTH_MS);
        gpio_put(GPIO_ENCODER_A, false);
        current_encoder_value++;
    }
    sleep_ms(HALF_PULSE_LENGTH_MS);
    printf("%d \r\n", current_encoder_value);
}

void setup_gpio() {
    uint8_t alen = sizeof(gpio_init_array);
    for(uint8_t i = 0; i < alen; i++) {
        uint8_t current_gpio = gpio_init_array[i];
        gpio_init(current_gpio);
        gpio_set_dir(current_gpio, GPIO_OUT);
        gpio_put(current_gpio, true);
    }
    gpio_put(GPIO_ENCODER_A, false);
    gpio_put(GPIO_ENCODER_B, false);
}

void setup() {
    
    setup_gpio();
    set_speed(1);
}


int main(void) {
    board_init();
    tusb_init();
    setup();

    puts("starting ...");
    while (1) {
        encoder_task();

        // tinyusb host task
        tuh_task();

        hid_task();
        led_blinking_task();
    }
    return 0;
}

//--------------------------------------------------------------------+
// USB HID
//--------------------------------------------------------------------+

CFG_TUSB_MEM_SECTION static hid_generic_hid_report_t usb_generic_hid_report;

static inline void process_generic_hid_report(hid_generic_hid_report_t const *p_new_report) {
    board_led_write(1);
    puts("process_generic_hid_report");
    printf("%02X %02X %02X %02X %02X %02X \r\n", 
        p_new_report->buffer[0], 
        p_new_report->buffer[1],
        p_new_report->buffer[2],
        p_new_report->buffer[3],
        p_new_report->buffer[4],
        p_new_report->buffer[5]);

    board_led_write(0);

    uint8_t axis_select = p_new_report->buffer[3];
    if (last_axis != axis_select) {
        last_axis = axis_select;
        select_axis_pulse(axis_select);
    }

    int8_t encoder = (int8_t) p_new_report->buffer[4];
    //printf("encoder: %d \r\n", encoder);

    if (encoder != 0) {
        current_encoder_value += encoder;
        //printf("encoder: %d \r\n", current_encoder_value);
    }

    uint8_t button = p_new_report->buffer[1];
    if (button == 0 && last_button_code != 0) {
        switch(last_button_code) {
            case 0x0D: toggle_speed(); break;
        }        
    }
    last_button_code = button;
}

void tuh_hid_generic_hid_mounted_cb(uint8_t dev_addr) {
    // application set-up
    printf("A generic device (address %d) is mounted\r\n", dev_addr);
}

void tuh_hid_generic_hid_unmounted_cb(uint8_t dev_addr) {
    // application tear-down
    printf("A generic device (address %d) is unmounted\r\n", dev_addr);
}

// invoked ISR context
void tuh_hid_generic_hid_isr(uint8_t dev_addr, xfer_result_t event) {
    (void) dev_addr;
    (void) event;
}



void hid_task(void) {
    uint8_t const addr = 1;

    if (tuh_hid_generic_hid_is_mounted(addr)) {
        if (!tuh_hid_generic_hid_is_busy(addr)) {
            process_generic_hid_report(&usb_generic_hid_report);
            tuh_hid_generic_hid_get_report(addr, &usb_generic_hid_report);
        }
    }

}


//--------------------------------------------------------------------+
// BLINKING TASK
//--------------------------------------------------------------------+
void led_blinking_task(void) {
    const uint32_t interval_ms = 250;
    static uint32_t start_ms = 0;

    static bool led_state = false;

    // Blink every interval ms
    if (board_millis() - start_ms < interval_ms) return; // not enough time
    start_ms += interval_ms;

    board_led_write(led_state);
    led_state = 1 - led_state; // toggle
}

