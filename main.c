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


typedef struct TU_ATTR_PACKED
{
  uint8_t buffer[128];  
} hid_generic_hid_report_t;


void led_blinking_task(void);
extern void hid_task(void);

int main(void) {
    board_init();
    tusb_init();

    while (1) {
        // tinyusb host task
        tuh_task();
        led_blinking_task();

        hid_task();
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

