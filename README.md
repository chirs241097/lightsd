# lightsd

`lightsd` is a small daemon to make the ambient light sensor on your
~~(actually, my)~~ laptop useful in Linux <sup>interjection</sup>
without using a full desktop environment (or `systemd`). It even works
in a framebuffer console!

This daemon watches the reading from the ambient light sensor and
controls the backlight of the screen and keyboard. It also creates a
fifo so that you can adjust relative brightness of the screen.

It only supports sensors using the industrial I/O bus. It has a generic
class for working with many types of iio devices though.

The project also demostrates how damn stupid a C++ program could look like
(not yet to its maximum extent).

Hopefully it does not yet hog the CPU.

# Warning
Alpha quality. May segmentation fault at any time. Featuring my terrible
code style. Ugly implementation (it works nevertheless).

As this daemon manipulates sysfs, IT ONLY RUNS AS ROOT!

Do not use `lightsd` with GNOME. It has good chance to conflict with GNOME's
implementation `iio-sensor-proxy`.

# Dependencies
 - CMake
 - gcc 8.x or clang 6 with C++17 support  

# Building
Just `mkdir build && cd build && cmake .. && make`.

# Installing
`sudo make install`. If you are using `OpenRC`, an init script is also installed.
Sorry to `systemd` users but my only computer with iio sensors is not powered by
that.

# Documentation
None. The code documents itself (in a bad way).

## fifo usage
- `u [x=5,0<x<=100]`  
Makes lcd x% brighter. ([U]p)
- `d [x=5,0<x<=100]`  
Makes lcd x% darker. ([D]own)
- `s <x,-100<=x<=100>`  
[S]ets relative brightness of lcd.
- `r`  
[R]esets relative brightness of lcd, equivalent to `s 0`.
- `f`  
[F]orces an adjustment to be made. You may want to call this when the lid
is being opened in order to turn on the keyboard backlight.
- `m`  
Disables automatic brightness. ([M]anual)  
Some of the commands (`r`/`f`) does nothing in manual mode. 
Command `s` sets absolute brightness in manual mode.
- `a`  
Enables [a]utomatic brightness.
- `i`  
Print current status to stdout. ([I]nfo)  
Output is in the following format:
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

# Integration
Take `acpid` as an example.

`/etc/acpi/events/brightness-down`:
```
event=video/brightnessdown
action=echo d 5 > /tmp/lightsd.cmd.fifo
```

`/etc/acpi/events/brightness-up`:
```
event=video/brightnessup
action=echo u 5 > /tmp/lightsd.cmd.fifo
```

If your laptop turns keyboard backlight off automatically when closing the
lid (which I believe is what most laptops do), you may also want the
following to turn it back on when you open the lid:

`/etc/acpi/events/lid-open`:
```
event=button/lid.*open
action=echo f > /tmp/lightsd.cmd.fifo
```

# To-do
 - Less segmentation faults (already eliminated in my daily usage)
 - Less data races (???)
 - Input sanitation (partially done)
 - Actual, _real_ logging: not printf'ing to `stdout`. (probably done)
 - More commands: disabling auto adjustment, set absolute brightness etc. (done)
 - D-Bus interface?
 - Change configuration format so that one can control something other than
   screen backlight and keyboard backlight? (will anybody actually use it?)
 - auto orientation using accelerometer? (otherwise my `Sensor` class is a waste /s)
 - hogging cpu and battery? (oh no)
 - triggers custom scripts? (detonate your computer once the reading reaches a
   certain value?)
 - ability to embed an email client and to order pizza? (not happening)
