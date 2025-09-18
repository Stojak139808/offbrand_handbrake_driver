#include <linux/device.h>
#include <linux/hid.h>
#include <linux/module.h>
#include <linux/kernel.h>

#define USB_VENDOR_ID_OFFBRAND_HANDBRAKE 0x1eaf
#define USB_DEVICE_ID_OFFBRAND_HANDBRAKE 0x0024

/* original HID descriptor from ./hid-decode
static u8 orig_rdesc[] = {
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

static u8 fixed_rdesc[] = {
    /* Top-level Application Collection: Game Pad */
    0x05, 0x01,        /* Usage Page (Generic Desktop) */
    0x09, 0x05,        /* Usage (Game Pad) */
    0xA1, 0x01,        /* Collection (Application) */

        /* ---- Report ID ---- */
        0x85, 0x14,    /* Report ID = 0x14 */

        /* ---- Byte 1: Button 1 (bit 0) ---- */
        0x05, 0x09,    /* Usage Page (Button) */
        0x19, 0x01,    /* Usage Minimum (Button 1) */
        0x29, 0x02,    /* Usage Maximum (Button 2) */
        0x15, 0x00,    /* Logical Minimum (0) */
        0x25, 0x01,    /* Logical Maximum (1) */
        0x75, 0x01,    /* Report Size (1 bit) */
        0x95, 0x01,    /* Report Count (2 field) */
        0x81, 0x02,    /* Input (Data,Var,Abs) -> Button 1 */
        0x75, 0x07,    /* Report Size (6 bits) */
        0x95, 0x01,    /* Report Count (1 field) */
        0x81, 0x03,    /* Input (Const,Var,Abs) -> padding bits */

        /* ---- Physical Collection for Axes ---- */
        0xA1, 0x00,    /* Collection (Physical) */

            /* Byte 2: Axis X (real) full range 0–32767 */
            0x05, 0x01,        /* Usage Page (Generic Desktop) */
            0x09, 0x30,        /* Usage (X) */
            0x15, 0x00,        /* Logical Minimum = 0 */
            0x26, 0xFF, 0x7F,  /* Logical Maximum = 32767 */
            0x75, 0x10,        /* Report Size = 8 bits (device sends 1 byte) */
            0x95, 0x01,        /* Report Count = 1 */
            0x81, 0x02,        /* Input (Data,Var,Abs) -> Axis X */

            /* Byte 3: Dummy Axis Y (Data) */
            0x09, 0x31,        /* Usage (Y) */
            0x15, 0x00,        /* Logical Minimum = 0 */
            0x26, 0xFF, 0x7F,  /* Logical Maximum = 32767 */
            0x75, 0x08,        /* Report Size = 8 bits */
            0x95, 0x01,        /* Report Count = 1 */
            0x81, 0x03,        /* Input (Const,Var,Abs) -> Dummy Y axis */

        0xC0,            /* End Physical Collection */

        /* ---- Extra dummy buttons or padding ---- */
        0x75, 0x08,    /* Report Size = 8 bits */
        0x95, 0x08,    /* Report Count = 8 fields */
        0x81, 0x03,    /* Input (Const,Var,Abs) -> padding */

    0xC0               /* End Application Collection */
};

static const __u8* offbrand_handbrake_HID_descriptor_fixup(struct hid_device *hdev,
                                      __u8 *rdesc,
                                      unsigned int *rsize)
{
    hid_info(hdev, "Fixing up HID Descriptor Report for the off-brand handbrake");
    *rsize = sizeof(fixed_rdesc);
    return fixed_rdesc;
}

static int offbrand_handbrake_data_correction(struct hid_device *hdev, struct hid_report *report,
    u8 *data, int size){

    u8 handbrake_axis_input = data[size - 1];
    u16 mapped_data = (u16)((handbrake_axis_input * 0x7FFF) / 255);

    data[2U] = (u8)(mapped_data & 0xFF);
    data[3U] = (u8)((mapped_data & 0xFF00) >> 8);
    for (int i = 4; i < size; i++){
        data[i] = 0x00U;
    }

    return 0;
}


static const struct hid_device_id offbrand_handbrake_id_table[] = {
    { HID_USB_DEVICE(USB_VENDOR_ID_OFFBRAND_HANDBRAKE, USB_DEVICE_ID_OFFBRAND_HANDBRAKE)},
    { }
};

static struct hid_driver handbrake_driver = {
    .name = "offbrand handbrake",
    .id_table = offbrand_handbrake_id_table,
    .report_fixup = offbrand_handbrake_HID_descriptor_fixup,
    .raw_event = offbrand_handbrake_data_correction,
};

module_hid_driver(handbrake_driver);

MODULE_LICENSE("GPL");
