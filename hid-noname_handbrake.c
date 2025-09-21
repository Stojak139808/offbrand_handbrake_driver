#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/string.h>


/* static functions declarations */
static const __u8* offbrand_handbrake_HID_descriptor_fixup(
        struct hid_device *hdev, __u8 *rdesc, unsigned int *rsize);

static int offbrand_handbrake_data_correction(struct hid_device *hdev,
        struct hid_report *report, u8 *data, int size);

/* macros */
#define USB_VENDOR_ID_OFFBRAND_HANDBRAKE 0x1EAFU
#define USB_DEVICE_ID_OFFBRAND_HANDBRAKE 0x0024U

#define OFFBRAND_HANDBRAKE_MODULE_DESCRIPTION \
        "This module fixes the non standard input of off-brand simracing \
        handbrakes into a standard HID joystick input, readable and \
        interpretable by Linux"

// the original packet length sent by the handbrake
#define ORIGINAL_HANDBRAKE_PACKET_LENGTH    13U

// the new packet length, adjusted to the new descriptor, without the padding
#define NEW_HANDBRAKE_PACKET_LENGTH         5U

#define NEW_HANDBRAKE_PACKET_BUTTON_OFFSET  1U
#define NEW_HANDBRAKE_PACKET_X_AXIS_OFFSET  2U
#define NEW_HANDBRAKE_PACKET_Y_AXIS_OFFSET  4U

MODULE_LICENSE("GPL");
MODULE_DESCRIPTION(OFFBRAND_HANDBRAKE_MODULE_DESCRIPTION);


/* structures */
static const struct hid_device_id offbrand_handbrake_id_table[] = {
    {HID_USB_DEVICE(USB_VENDOR_ID_OFFBRAND_HANDBRAKE,
                    USB_DEVICE_ID_OFFBRAND_HANDBRAKE)},
    {}
};

static struct hid_driver handbrake_driver = {
    .name = "offbrand handbrake",
    .id_table = offbrand_handbrake_id_table,
    .report_fixup = offbrand_handbrake_HID_descriptor_fixup,
    .raw_event = offbrand_handbrake_data_correction,
};

/* original HID descriptor from ./hid-decode
static u8 orig_handbrake_rdesc[] = {
0x05, 0x01,                    // Usage Page (Generic Desktop)        0
0x09, 0x04,                    // Usage (Joystick)                    2
0xa1, 0x01,                    // Collection (Application)            4
0x85, 0x14,                    //  Report ID (20)                     6
0x15, 0x00,                    //  Logical Minimum (0)                8
0x25, 0x01,                    //  Logical Maximum (1)                10
0x75, 0x01,                    //  Report Size (1)                    12
0x95, 0x20,                    //  Report Count (32)                  14
0x05, 0x09,                    //  Usage Page (Button)                16
0x19, 0x01,                    //  Usage Minimum (1)                  18
0x29, 0x20,                    //  Usage Maximum (32)                 20
0x81, 0x02,                    //  Input (Data,Var,Abs)               22
0x15, 0x00,                    //  Logical Minimum (0)                24
0x25, 0x07,                    //  Logical Maximum (7)                26
0x35, 0x00,                    //  Physical Minimum (0)               28
0x46, 0x3b, 0x01,              //  Physical Maximum (315)             30
0x75, 0x04,                    //  Report Size (4)                    33
0x95, 0x01,                    //  Report Count (1)                   35
0x65, 0x14,                    //  Unit (EnglishRotation: deg)        37
0x05, 0x01,                    //  Usage Page (Generic Desktop)       39
0x09, 0x39,                    //  Usage (Hat switch)                 41
0x81, 0x42,                    //  Input (Data,Var,Abs,Null)          43
0x05, 0x01,                    //  Usage Page (Generic Desktop)       45
0x09, 0x01,                    //  Usage (Pointer)                    47
0xa1, 0x00,                    //  Collection (Physical)              49
0x15, 0x00,                    //   Logical Minimum (0)               51
0x26, 0xff, 0x03,              //   Logical Maximum (1023)            53
0x75, 0x0a,                    //   Report Size (10)                  56
0x95, 0x04,                    //   Report Count (4)                  58
0x09, 0x30,                    //   Usage (X)                         60
0x09, 0x31,                    //   Usage (Y)                         62
0x09, 0x33,                    //   Usage (Rx)                        64
0x09, 0x34,                    //   Usage (Ry)                        66
0x81, 0x02,                    //   Input (Data,Var,Abs)              68
0xc0,                          //  End Collection                     70
0x15, 0x00,                    //  Logical Minimum (0)                71
0x26, 0xff, 0x03,              //  Logical Maximum (1023)             73
0x75, 0x0a,                    //  Report Size (10)                   76
0x95, 0x02,                    //  Report Count (2)                   78
0x09, 0x36,                    //  Usage (Slider)                     80
0x09, 0x36,                    //  Usage (Slider)                     82
0x81, 0x02,                    //  Input (Data,Var,Abs)               84
0xc0,                          // End Collection                      86
};
*/

static const u8 fixed_HID_descriptor[] = {
    /* Top-level Application Collection: Game Pad */
    0x05U, 0x01U,        // Usage Page (Generic Desktop)
    0x09U, 0x05U,        // Usage (Game Pad)
    0xA1U, 0x01U,        // Collection (Application)

        /* ---- Report ID ---- */
        0x85U, 0x14U,    // Report ID = 0x14

        /* ---- Byte 1: Button 1 (bit 0) ---- */
        0x05U, 0x09U,    // Usage Page (Button)
        0x19U, 0x01U,    // Usage Minimum (Button 1)
        0x29U, 0x02U,    // Usage Maximum (Button 2)
        0x15U, 0x00U,    // Logical Minimum (0)
        0x25U, 0x01U,    // Logical Maximum (1)
        0x75U, 0x01U,    // Report Size (1 bit)
        0x95U, 0x02U,    // Report Count (2 field)
        0x81U, 0x02U,    // Input (Data,Var,Abs) -> Button 1
        0x75U, 0x06U,    // Report Size (6 bits)
        0x95U, 0x01U,    // Report Count (1 field)
        0x81U, 0x03U,    // Input (Const,Var,Abs) -> padding bits

        /* ---- Physical Collection for Axes ---- */
        0xA1U, 0x00U,    // Collection (Physical)

            // Byte 2 - 3: Axis X (real) full range 0–32767
            0x05U, 0x01U,        // Usage Page (Generic Desktop)
            0x09U, 0x30U,        // Usage (X) */
            0x15U, 0x00U,        // Logical Minimum = 0
            0x26U, 0xFFU, 0x7FU, // Logical Maximum = 32767
            0x75U, 0x10U,        // Report Size = 16 bits (device sends 2 bytes)
            0x95U, 0x01U,        // Report Count = 1
            0x81U, 0x02U,        // Input (Data,Var,Abs) -> Axis X

            // Byte 4: Dummy Axis Y (Data)
            0x09U, 0x31U,        // Usage (Y)
            0x15U, 0x00U,        // Logical Minimum = 0
            0x26U, 0xFFU, 0x7FU, // Logical Maximum = 32767
            0x75U, 0x08U,        // Report Size = 8 bits
            0x95U, 0x01U,        // Report Count = 1
            0x81U, 0x02U,        // Input (data,Var,Abs) -> Dummy Y axis

        0xC0U,            // End Physical Collection

        /* ---- Extra dummy buttons or padding ---- */
        0x75U, 0x08U,    // Report Size = 8 bits
        0x95U, 0x08U,    // Report Count = 8 fields
        0x81U, 0x03U,    // Input (Const,Var,Abs) -> padding

    0xC0U               // End Application Collection
};


/* static functions definitions */
static const __u8* offbrand_handbrake_HID_descriptor_fixup(
        struct hid_device *hdev, __u8 *rdesc, unsigned int *rsize)
{
    hid_info(hdev,
        "Fixing up HID Descriptor Report for the off-brand handbrake");

    *rsize = sizeof(fixed_HID_descriptor);
    return fixed_HID_descriptor;
}

static int offbrand_handbrake_data_correction(struct hid_device *hdev,
        struct hid_report *report, u8 *data, int size){

    if (ORIGINAL_HANDBRAKE_PACKET_LENGTH != size){
        // Device sent an undefined packet, ignore it
        return -1;
    }

    u8 handbrake_axis_input = data[ORIGINAL_HANDBRAKE_PACKET_LENGTH - 1];
    u16 mapped_axis_data = (u16)((handbrake_axis_input * 0x7FFFU) / 255U);

    // reorganize data from the old packet into the new packet

    /* TODO: should be 0x01, but games recognize this handbrake as xbox
       controller, and this button as A, which gets annoying in games
    */
    data[NEW_HANDBRAKE_PACKET_BUTTON_OFFSET] = data[1] & 0x00U;
    data[NEW_HANDBRAKE_PACKET_X_AXIS_OFFSET] = (u8)(mapped_axis_data & 0xFFU);
    data[NEW_HANDBRAKE_PACKET_X_AXIS_OFFSET + 1] = 
        (u8)((mapped_axis_data & 0xFF00U) >> 8U);
    data[NEW_HANDBRAKE_PACKET_Y_AXIS_OFFSET] = 0;

    // set rest of the bytes to 0x00U
    memset((u8*)(data + NEW_HANDBRAKE_PACKET_LENGTH), 
        0x00U, ORIGINAL_HANDBRAKE_PACKET_LENGTH - NEW_HANDBRAKE_PACKET_LENGTH);

    return 0U;
}

// register the new driver
module_hid_driver(handbrake_driver);
