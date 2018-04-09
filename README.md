# lightsd

`lightsd` is a small daemon to make your ~~(actually, my)~~ ambient
light sensor on your laptop useful in a Linux <sup>interjection</sup>
desktop without using a full desktop environment (or `systemd`).

This service watches the readings from the ambient light sensor and
control the backlight of the screen and keyboard. It also creates a
fifo so that you can adjust relative brightness of the lcd.

The project also demostrates how damn stupid a C++ program could look like.

# Warning
WIP. Does not yet do any kind of input sanitation. May segmentation fault
at any time. The author uses Gentoo. _Very_ shitty code.

AS A DAEMON, IT ONLY RUNS AS ROOT!

# Building
Building _requires_ C++17. Just `mkdir build && cd build && cmake .. && make`.

# Documentation
None. The code documentes itself.

## fifo usage
- `u <x,0<x<=100>`  
Makes lcd x% brighter.
- `d <x,0<x<=100>`  
Makes lcd x% darker.
- `s <x,-100<=x<=100>`  
Set relative brightness of lcd.
- `r`  
Reset relative brightness of lcd, equivalent to `s 0`.

The fifo is owned by `root:video` and has permission `0620` so that
everyone in the video group could potentially mess with your brightness.
Surprise!
