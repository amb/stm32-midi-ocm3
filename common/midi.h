
#pragma once

#include <libopencm3/cm3/scb.h>
#include <libopencm3/stm32/desig.h>
#include <libopencm3/stm32/gpio.h>
#include <libopencm3/stm32/rcc.h>
#include <libopencm3/usb/audio.h>
#include <libopencm3/usb/midi.h>
#include <libopencm3/usb/usbd.h>
#include <stdlib.h>

/*
 * All references in this file come from Universal Serial Bus Device Class
 * Definition for MIDI Devices, release 1.0.
 */

/*
 * Table B-1: MIDI Adapter Device Descriptor
 */
static const struct usb_device_descriptor dev = {
    .bLength = USB_DT_DEVICE_SIZE,
    .bDescriptorType = USB_DT_DEVICE,
    .bcdUSB = 0x0200,  /* was 0x0110 in Table B-1 example descriptor */
    .bDeviceClass = 0, /* device defined at interface level */
    .bDeviceSubClass = 0,
    .bDeviceProtocol = 0,
    .bMaxPacketSize0 = 64,
    .idVendor = 0x6666,  /* Prototype product vendor ID */
    .idProduct = 0x5119, /* dd if=/dev/random bs=2 count=1 | hexdump */
    .bcdDevice = 0x0100,
    .iManufacturer = 1, /* index to string desc */
    .iProduct = 2,      /* index to string desc */
    .iSerialNumber = 3, /* index to string desc */
    .bNumConfigurations = 1,
};

/*
 * Midi specific endpoint descriptors.
 */
static const struct usb_midi_endpoint_descriptor midi_bulk_endp[] = {
    {
        /* Table B-12: MIDI Adapter Class-specific Bulk OUT Endpoint
         * Descriptor
         */
        .head =
            {
                .bLength = sizeof(struct usb_midi_endpoint_descriptor),
                .bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
                .bDescriptorSubType = USB_MIDI_SUBTYPE_MS_GENERAL,
                .bNumEmbMIDIJack = 1,
            },
        .jack[0] =
            {
                .baAssocJackID = 0x01,
            },
    },
    {
        /* Table B-14: MIDI Adapter Class-specific Bulk IN Endpoint
         * Descriptor
         */
        .head =
            {
                .bLength = sizeof(struct usb_midi_endpoint_descriptor),
                .bDescriptorType = USB_AUDIO_DT_CS_ENDPOINT,
                .bDescriptorSubType = USB_MIDI_SUBTYPE_MS_GENERAL,
                .bNumEmbMIDIJack = 1,
            },
        .jack[0] =
            {
                .baAssocJackID = 0x03,
            },
    }};

/*
 * Standard endpoint descriptors
 */
static const struct usb_endpoint_descriptor bulk_endp[] = {
    {/* Table B-11: MIDI Adapter Standard Bulk OUT Endpoint Descriptor */
     .bLength = USB_DT_ENDPOINT_SIZE,
     .bDescriptorType = USB_DT_ENDPOINT,
     .bEndpointAddress = 0x01,
     .bmAttributes = USB_ENDPOINT_ATTR_BULK,
     .wMaxPacketSize = 0x40,
     .bInterval = 0x00,

     .extra = &midi_bulk_endp[0],
     .extralen = sizeof(midi_bulk_endp[0])},
    {.bLength = USB_DT_ENDPOINT_SIZE,
     .bDescriptorType = USB_DT_ENDPOINT,
     .bEndpointAddress = 0x81,
     .bmAttributes = USB_ENDPOINT_ATTR_BULK,
     .wMaxPacketSize = 0x40,
     .bInterval = 0x00,

     .extra = &midi_bulk_endp[1],
     .extralen = sizeof(midi_bulk_endp[1])}};

/*
 * Table B-4: MIDI Adapter Class-specific AC Interface Descriptor
 */
static const struct {
    struct usb_audio_header_descriptor_head header_head;
    struct usb_audio_header_descriptor_body header_body;
} __attribute__((packed)) audio_control_functional_descriptors = {
    .header_head =
        {
            .bLength =
                sizeof(struct usb_audio_header_descriptor_head) + 1 * sizeof(struct usb_audio_header_descriptor_body),
            .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
            .bDescriptorSubtype = USB_AUDIO_TYPE_HEADER,
            .bcdADC = 0x0100,
            .wTotalLength =
                sizeof(struct usb_audio_header_descriptor_head) + 1 * sizeof(struct usb_audio_header_descriptor_body),
            .binCollection = 1,
        },
    .header_body =
        {
            .baInterfaceNr = 0x01,
        },
};

/*
 * Table B-3: MIDI Adapter Standard AC Interface Descriptor
 */
static const struct usb_interface_descriptor audio_control_iface[] = {
    {.bLength = USB_DT_INTERFACE_SIZE,
     .bDescriptorType = USB_DT_INTERFACE,
     .bInterfaceNumber = 0,
     .bAlternateSetting = 0,
     .bNumEndpoints = 0,
     .bInterfaceClass = USB_CLASS_AUDIO,
     .bInterfaceSubClass = USB_AUDIO_SUBCLASS_CONTROL,
     .bInterfaceProtocol = 0,
     .iInterface = 0,

     .extra = &audio_control_functional_descriptors,
     .extralen = sizeof(audio_control_functional_descriptors)}};

/*
 * Class-specific MIDI streaming interface descriptor
 */
static const struct {
    struct usb_midi_header_descriptor header;
    struct usb_midi_in_jack_descriptor in_embedded;
    struct usb_midi_in_jack_descriptor in_external;
    struct usb_midi_out_jack_descriptor out_embedded;
    struct usb_midi_out_jack_descriptor out_external;
} __attribute__((packed)) midi_streaming_functional_descriptors = {
    /* Table B-6: Midi Adapter Class-specific MS Interface Descriptor */
    .header =
        {
            .bLength = sizeof(struct usb_midi_header_descriptor),
            .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
            .bDescriptorSubtype = USB_MIDI_SUBTYPE_MS_HEADER,
            .bcdMSC = 0x0100,
            .wTotalLength = sizeof(midi_streaming_functional_descriptors),
        },
    /* Table B-7: MIDI Adapter MIDI IN Jack Descriptor (Embedded) */
    .in_embedded =
        {
            .bLength = sizeof(struct usb_midi_in_jack_descriptor),
            .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
            .bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_IN_JACK,
            .bJackType = USB_MIDI_JACK_TYPE_EMBEDDED,
            .bJackID = 0x01,
            .iJack = 0x00,
        },
    /* Table B-8: MIDI Adapter MIDI IN Jack Descriptor (External) */
    .in_external =
        {
            .bLength = sizeof(struct usb_midi_in_jack_descriptor),
            .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
            .bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_IN_JACK,
            .bJackType = USB_MIDI_JACK_TYPE_EXTERNAL,
            .bJackID = 0x02,
            .iJack = 0x00,
        },
    /* Table B-9: MIDI Adapter MIDI OUT Jack Descriptor (Embedded) */
    .out_embedded = {.head =
                         {
                             .bLength = sizeof(struct usb_midi_out_jack_descriptor),
                             .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
                             .bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_OUT_JACK,
                             .bJackType = USB_MIDI_JACK_TYPE_EMBEDDED,
                             .bJackID = 0x03,
                             .bNrInputPins = 1,
                         },
                     .source[0] =
                         {
                             .baSourceID = 0x02,
                             .baSourcePin = 0x01,
                         },
                     .tail =
                         {
                             .iJack = 0x00,
                         }},
    /* Table B-10: MIDI Adapter MIDI OUT Jack Descriptor (External) */
    .out_external =
        {
            .head =
                {
                    .bLength = sizeof(struct usb_midi_out_jack_descriptor),
                    .bDescriptorType = USB_AUDIO_DT_CS_INTERFACE,
                    .bDescriptorSubtype = USB_MIDI_SUBTYPE_MIDI_OUT_JACK,
                    .bJackType = USB_MIDI_JACK_TYPE_EXTERNAL,
                    .bJackID = 0x04,
                    .bNrInputPins = 1,
                },
            .source[0] =
                {
                    .baSourceID = 0x01,
                    .baSourcePin = 0x01,
                },
            .tail =
                {
                    .iJack = 0x00,
                },
        },
};

/*
 * Table B-5: MIDI Adapter Standard MS Interface Descriptor
 */
static const struct usb_interface_descriptor midi_streaming_iface[] = {
    {.bLength = USB_DT_INTERFACE_SIZE,
     .bDescriptorType = USB_DT_INTERFACE,
     .bInterfaceNumber = 1,
     .bAlternateSetting = 0,
     .bNumEndpoints = 2,
     .bInterfaceClass = USB_CLASS_AUDIO,
     .bInterfaceSubClass = USB_AUDIO_SUBCLASS_MIDISTREAMING,
     .bInterfaceProtocol = 0,
     .iInterface = 0,

     .endpoint = bulk_endp,

     .extra = &midi_streaming_functional_descriptors,
     .extralen = sizeof(midi_streaming_functional_descriptors)}};

static const struct usb_interface ifaces[] = {{
                                                  .num_altsetting = 1,
                                                  .altsetting = audio_control_iface,
                                              },
                                              {
                                                  .num_altsetting = 1,
                                                  .altsetting = midi_streaming_iface,
                                              }};

/*
 * Table B-2: MIDI Adapter Configuration Descriptor
 */
static const struct usb_config_descriptor config = {
    .bLength = USB_DT_CONFIGURATION_SIZE,
    .bDescriptorType = USB_DT_CONFIGURATION,
    .wTotalLength = 0,   /* can be anything, it is updated automatically
                            when the usb code prepares the descriptor */
    .bNumInterfaces = 2, /* control and data */
    .bConfigurationValue = 1,
    .iConfiguration = 0,
    .bmAttributes = 0x80, /* bus powered */
    .bMaxPower = 0x32,

    .interface = ifaces,
};

static char usb_serial_number[25]; /* 12 bytes of desig and a \0 */

/* Buffer to be used for control requests. */
static uint8_t usbd_control_buffer[128];

/* SysEx identity message, preformatted with correct USB framing information */
static const uint8_t sysex_identity[] = {
    0x04, /* USB Framing (3 byte SysEx) */
    0xf0, /* SysEx start */
    0x7e, /* non-realtime */
    0x00, /* Channel 0 */
    0x04, /* USB Framing (3 byte SysEx) */
    0x7d, /* Educational/prototype manufacturer ID */
    0x66, /* Family code (byte 1) */
    0x66, /* Family code (byte 2) */
    0x04, /* USB Framing (3 byte SysEx) */
    0x51, /* Model number (byte 1) */
    0x19, /* Model number (byte 2) */
    0x00, /* Version number (byte 1) */
    0x04, /* USB Framing (3 byte SysEx) */
    0x00, /* Version number (byte 2) */
    0x01, /* Version number (byte 3) */
    0x00, /* Version number (byte 4) */
    0x05, /* USB Framing (1 byte SysEx) */
    0xf7, /* SysEx end */
    0x00, /* Padding */
    0x00, /* Padding */
};

usbd_device* usb_start(const char *usb_strings[]);
