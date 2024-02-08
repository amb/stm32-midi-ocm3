#include "midi.h"

usbd_device *usbd_dev;
static const char *usb_strings[] = {"ambi.tech", "midifiddler", usb_serial_number};
uint32_t total_received = 0;

static void usbmidi_data_rx_cb(usbd_device *ubd, uint8_t ep) {
    (void)ep;

    char buf[64];
    int len = usbd_ep_read_packet(ubd, 0x01, buf, 64);

    // Sysex ID send
    // if(len) {
    //     while(usbd_ep_write_packet(ubd, 0x81, sysex_identity, sizeof(sysex_identity)) == 0) {};
    // }

    total_received++;

    gpio_toggle(GPIOC, GPIO5);
}

static void usbmidi_set_config(usbd_device *ubd, uint16_t wValue) {
    (void)wValue;
    usbd_ep_setup(ubd, 0x01, USB_ENDPOINT_ATTR_BULK, 64, usbmidi_data_rx_cb);
    usbd_ep_setup(ubd, 0x81, USB_ENDPOINT_ATTR_BULK, 64, NULL);
}

static void send_test_output(void) {
    char buf[4] = {
        0x08, /* USB framing: virtual cable 0, note on */
        0x80, /* MIDI command: note on, channel 1 */
        60,   /* Note 60 (middle C) */
        64,   /* "Normal" velocity */
    };

    // buf[0] |= pressed;
    // buf[1] |= pressed << 4;

    while(usbd_ep_write_packet(usbd_dev, 0x81, buf, sizeof(buf)) == 0) {};
}

static void usb_midi_setup(void) {
    usbd_dev = usb_start(usb_strings);
    usbd_register_set_config_callback(usbd_dev, usbmidi_set_config);
}

int main(void) {
    rcc_clock_setup_pll(&rcc_hse_configs[RCC_CLOCK_HSE8_72MHZ]);
    usb_midi_setup();
    while(1) {
        for(unsigned i = 0; i < 800000; i++) { usbd_poll(usbd_dev); }
        send_test_output();
    }
}
