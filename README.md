# Off-brand simracing handbrake driver

The goal of this kernel module is to make those cheap chinese handbrakes 
(and possibly other devices) work under linux. When I bought mine the HID
module only detected a button press when the handbrake was fully engaged, even
though I could see and axis input with `hid-recorder`.

From what I've heard those handbrakes use repurposed some flight joystick 
firmware, hence why it sends that it has 32 buttons and multiple axes.

This kernel module fixes that, it replaces the HID descriptor so it only has
one button and one axis (plus one dummy button and one dummy axis, since 
otherwise top-level applications don't register `/dev/input/event*` as a 
joystick, just some standard HID device). Then it corrects the incoming data
to match the new descriptor, giving us a clean input.

## Using the kernel module

The module can be built with:

```
make
```

then to load the module use:

```
insmod hid-noname_handbrake.ko
```

To unload the module use:

```
rmmod hid_noname_handbrake 
```

To verify that the module was loaded correctly use:

```
dmesg
```

and verify that loading of this module is in the log.

## Testing your device

First step should be to see if the device sends data, for that use:

```
sudo hid-recorder /dev/hidrawx
```
(where x is the right input number) in my case the original data looked like this:

```
# LeafLabs Maple
# 0x05, 0x01,                    // Usage Page (Generic Desktop)        0
# 0x09, 0x04,                    // Usage (Joystick)                    2
# 0xa1, 0x01,                    // Collection (Application)            4
# 0x85, 0x14,                    //  Report ID (20)                     6
# 0x15, 0x00,                    //  Logical Minimum (0)                8
# 0x25, 0x01,                    //  Logical Maximum (1)                10
# 0x75, 0x01,                    //  Report Size (1)                    12
# 0x95, 0x20,                    //  Report Count (32)                  14
# 0x05, 0x09,                    //  Usage Page (Button)                16
# 0x19, 0x01,                    //  Usage Minimum (1)                  18
# 0x29, 0x20,                    //  Usage Maximum (32)                 20
# 0x81, 0x02,                    //  Input (Data,Var,Abs)               22
# 0x15, 0x00,                    //  Logical Minimum (0)                24
# 0x25, 0x07,                    //  Logical Maximum (7)                26
# 0x35, 0x00,                    //  Physical Minimum (0)               28
# 0x46, 0x3b, 0x01,              //  Physical Maximum (315)             30
# 0x75, 0x04,                    //  Report Size (4)                    33
# 0x95, 0x01,                    //  Report Count (1)                   35
# 0x65, 0x14,                    //  Unit (EnglishRotation: deg)        37
# 0x05, 0x01,                    //  Usage Page (Generic Desktop)       39
# 0x09, 0x39,                    //  Usage (Hat switch)                 41
# 0x81, 0x42,                    //  Input (Data,Var,Abs,Null)          43
# 0x05, 0x01,                    //  Usage Page (Generic Desktop)       45
# 0x09, 0x01,                    //  Usage (Pointer)                    47
# 0xa1, 0x00,                    //  Collection (Physical)              49
# 0x15, 0x00,                    //   Logical Minimum (0)               51
# 0x26, 0xff, 0x03,              //   Logical Maximum (1023)            53
# 0x75, 0x0a,                    //   Report Size (10)                  56
# 0x95, 0x04,                    //   Report Count (4)                  58
# 0x09, 0x30,                    //   Usage (X)                         60
# 0x09, 0x31,                    //   Usage (Y)                         62
# 0x09, 0x33,                    //   Usage (Rx)                        64
# 0x09, 0x34,                    //   Usage (Ry)                        66
# 0x81, 0x02,                    //   Input (Data,Var,Abs)              68
# 0xc0,                          //  End Collection                     70
# 0x15, 0x00,                    //  Logical Minimum (0)                71
# 0x26, 0xff, 0x03,              //  Logical Maximum (1023)             73
# 0x75, 0x0a,                    //  Report Size (10)                   76
# 0x95, 0x02,                    //  Report Count (2)                   78
# 0x09, 0x36,                    //  Usage (Slider)                     80
# 0x09, 0x36,                    //  Usage (Slider)                     82
# 0x81, 0x02,                    //  Input (Data,Var,Abs)               84
# 0xc0,                          // End Collection                      86
# 
R: 87 05 01 09 04 a1 01 85 14 15 00 25 01 75 01 95 20 05 09 19 01 29 20 81 02 15 00 25 07 35 00 46 3b 01 75 04 95 01 65 14 05 01 09 39 81 42 05 01 09 01 a1 00 15 00 26 ff 03 75 0a 95 04 09 30 09 31 09 33 09 34 81 02 c0 15 00 26 ff 03 75 0a 95 02 09 36 09 36 81 02 c0
N: LeafLabs Maple
I: 3 1eaf 0024
# ReportID: 20 / Button: 0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0  0 | Hat switch:  15 
#              | X:   512 | Y:   512 | Rx:     0 | Ry:     0 | Slider:     0 ,     7 
E: 000000.000000 13 14 00 00 00 00 0f 20 80 00 00 00 c0 01
```

move some buttons around and see how that affects the incoming data. In
my case I could see that the last byte from the incoming data reflected how
hard I pull the handbrake, but it wasn't interpreted as any axis (which is why
this module was created).

## Adjusting the module to your device

If you intend to use this module it's very likely that it will need
modifications, because your device will send different data, have different ID,
etc., but on the other hand, it should be fairly simple to adjust it to a
completely different device, not just some aliexpress handbrake.

The steps would probably be as follows:

1. Change the device and vendor ID to match your device
1. Create a new HID device descriptor and use `hid_driver.report_fixup` to
    correct it.
1. If you want to edit the raw incoming data, use `hid_driver.raw_event`
    to write a hook.
1. If everything seems fine on `hid-recorder` output, make sure that
    `evtest` lists your joystick, otherwise games won't realize that it is
    a joystick (if it doesn't you might need to add dummy buttons/axes).