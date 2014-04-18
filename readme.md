This is a small app for interfacing an [Adonit Jot Touch 4](http://www.adonit.net/jot/touch) device with the Linux `uinput`.

The main goal of this app is to enable a touchscreen-enabled laptop to work as a full drawing tablet using a pressure-sensitive Bluetooth pen.

The app is based on code from [wdaemon](http://sourceforge.net/apps/mediawiki/linuxwacom/index.php?title=Wdaemon) and [Gato library](https://gitorious.org/gato) by [Javier S. Pedro](http://javispedro.com).

## Build requirements

To build you will need **[libgato](https://gitorious.org/gato/libgato)** source in a folder named `libgato` inside the parent folder of this repository.

This project also require `libqt4` and `libbluetooth`.

Build using `make` command.

## Running

After building the project, run `gato-test` with a user with privileges to access `/dev/uinput` files.
