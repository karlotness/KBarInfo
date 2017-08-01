# KBarInfo - i3bar status line in C

Pretty rough but provides information on the current status of:
- Connected WiFi network and VPN (NetworkManager)
- Audio volume (PulseAudio)
- Battery charge (UPower)
- Time

Build with `make` and configure i3 to run the `kbarinfo` executable. This
program subscribes to updates on the system state over DBus.

## Dependencies
Building this application depends on:
- glib2
- libpulse

Gathering status information requires that you use the above software to control
your system (NetworkManager, PulseAudio, UPower).
