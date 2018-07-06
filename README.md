# lightsd

`lightsd` is a small daemon to make your ~~(actually, my)~~ ambient
light sensor on your laptop useful in Linux <sup>interjection</sup>
without using a full desktop environment (or `systemd`). It even works
in a tty!

This service watches the readings from the ambient light sensor and
control the backlight of the screen and keyboard. It also creates a
fifo so that you can adjust relative brightness of the lcd.

It only supports sensors using the industrial I/O bus. It has a generic
class for working with most types of iio devices though.

The project also demostrates how damn stupid a C++ program could look like
(not yet to its maximum extent).

Hopefully it does not yet hog the CPU.

# Warning
WIP. Does not yet do any kind of input sanitation. May segmentation fault
at any time. The author uses Gentoo. _Very_ shitty code.

As this daemon manipulates sysfs, IT ONLY RUNS AS ROOT!

# Dependencies
 - CMake
 - gcc 8.x or clang 6 with C++17 support  

# Building
Just `mkdir build && cd build && cmake .. && make`.

# Installing
`sudo make install`. If you are using `OpenRC`, an init script is also installed.
Sorry to `systemd` users but my only computer running `systemd` does not have
iio sensors.

# Documentation
None. The code documents itself.

## fifo usage
- `u [x=5,0<x<=100]`  
Makes lcd x% brighter.
- `d [x=5,0<x<=100]`  
Makes lcd x% darker.
- `s <x,-100<=x<=100>`  
Sets relative brightness of lcd.
- `r`  
Resets relative brightness of lcd, equivalent to `s 0`.
- `f`  
Forces an adjustment to be made. You may want to call this when the lid
is being opened in order to turn on the keyboard backlight.

Not implemented:
- `m`  
Disables automatic brightness. (\_M\_anual)  
Some of the commands (r/s/f) does nothing in manual brightness mode.
- `a`  
Enables automatic brightness.
- `i`  
Print current status to stdout. Example output:
```
Mode: <Automatic|Manual>
ALS value: <%f|-->
Display brightness: %d%% [(+%d%%)]
Keyboard backlight brightness: %d%%
```

The fifo is owned by `root:video` and has permission `0220` so that
everyone in the video group could potentially mess with your brightness.
Surprise!

# Tested on
 - Lenovo ThinkPad X1 Yoga 1st gen.

# To-do
 - Less segmentation faults
 - Less data races
 - Input sanitation
 - Actual, _real_ logging: not printf'ing to `stdout`.
 - More fifo commands: disabling auto adjustment, set absolute brightness etc.
 - use iio triggers instead?
 - auto orientation using accelerometer?
 - hogging cpu and battery?
 - ability to embed an email client and to order pizza?
