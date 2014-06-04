This is a small app for interfacing an [Adonit Jot Touch 4](http://www.adonit.net/jot/touch) device with the Linux `uinput`.

The main goal of this app is to enable a touchscreen-enabled laptop to work as a full drawing tablet using a pressure-sensitive Bluetooth pen.

The app uses code from [BlueZ](http://www.bluez.org).

## Build requirements

To build you will need **[glib](https://developer.gnome.org/glib/stable)** and **[DBus](http://www.linuxfromscratch.org/blfs/view/svn/general/dbus.html)** >= 1.6.

Build using `make` command.

## Running

After building the project, run `adonit` with a user with privileges to access `/dev/uinput` and `/dev/input/event*` (i.e. Use `sudo adonit`).
