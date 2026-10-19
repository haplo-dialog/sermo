<!-- SPDX-License-Identifier: CC0-1.0 -->
# sermo examples

[Français](README.md)

The 76 folders of `examples/`, one line each, grouped by use. A link leads to the folder's README when it
has one.

## Where they are installed

The `sermo-doc` package installs part of these folders under `/usr/share/doc/sermo-doc/examples/`:

- `metiers/` and `boites-a-outils/`, in full;
- from `modbus/`, `opcua/` and `mavlink/`, the scripts and sources only (not a program compiled on the
  machine);
- the fifteen examples of 3.1.0 (`approbation/`, `bluetooth-scan/`, `bus-can/`, `camera-apercu/`,
  `certificat-tls/`, `dns-testeur/`, `domotique-mqtt/`, `gnss-moniteur/`, `heure-ntp/`, `journal-direct/`,
  `lecteur-media/`, `liaison-serie/`, `reseau-testeur/`, `usb-moniteur/`, `wifi-scan/`), all their files.

The other folders are in the sources only.

## Language

The examples' READMEs are in French, except those of `metiers/` and `boites-a-outils/`, which also exist in
English. Those of `pfeme/`, `pfontview/` and `playmusic/` are the original READMEs, in English.

## Licence

Most files written for sermo state their licence at the top (`SPDX-License-Identifier`): most often
CC0-1.0 (public domain), sometimes GPL-2.0 or later (`metiers/verifie.sh`, `mavlink/faux-drone.py`,
`glade/*-gtk4.ui`). Part of `examples/` comes from gtkdialog: those files, without that line, are under
GPL-2.0 or later (see `debian/copyright`); `c_embedded/example01.c` says so by its GPL notice at the top.
The READMEs of `metiers/`, `boites-a-outils/`, `showcase/` and `system-tools/` have no SPDX line:
`debian/copyright` files them, like the rest of `examples/`, under GPL-2.0 or later.

## Widgets: one folder per tag

Most come from gtkdialog. Each script picks its binary from `GTKDIALOG` (default `gtk3sermo`):
`GTKDIALOG=qt6sermo ./button`.

- `aspectframe/` — `<aspectframe>`: a container that keeps its child's width/height ratio.
- `button/` — `<button>`: label, image, icon, image position, signals.
- `c_embedded/` — driving a sermo dialog from a C program (`example01.c`).
- `calendar/` — `<calendar>`: pick a date.
- `checkbox/` — `<checkbox>`: attributes, conditional refresh.
- `colorbutton/` — `<colorbutton>`: pick a colour.
- `combobox/` — `<combobox>`: list, attributes, `<input>`.
- `comboboxentry/` — `<comboboxentry>`: list with free input.
- `comboboxtext/` — `<comboboxtext>`: simple drop-down list.
- `edit/` — `<edit>`: multi-line text.
- `entry/` — `<entry>`: one-line field, attributes, `<input>`.
- `eventbox/` — `<eventbox>`: an area that catches clicks.
- `expander/` — `<expander>`: a collapsible section.
- `filechooser/` — `<filechooser>`: pick a file or a folder.
- `fontbutton/` — `<fontbutton>`: pick a font.
- `frame/` — `<frame>`: a frame with a title.
- `glade/` — `--glade-xml`: a GtkBuilder interface (GTK 3 and GTK 4 ports only).
- `hbox/` — `<hbox>`: horizontal box, scrollable.
- `hscale/` — `<hscale>`: horizontal slider.
- `hseparator/` — `<hseparator>`: horizontal separator.
- `image/` — `<image>`: an image.
- `infobar/` — `<infobar>`: a notification bar.
- `layer-shell/` — `<window>` attributes to anchor a window under Wayland (bar, dock).
- `linkbutton/` — `<linkbutton>`: a link button.
- `list/` — `<list>`: list with selection, actions, signals.
- `menu/` — `<menu>`: a menu.
- `menubar/` — `<menubar>`: a menu bar.
- `menuitem/` — `<menuitem>`: menu entries.
- `menuitemseparator/` — `<menuitemseparator>`: menu separator.
- `miscellaneous/` — miscellaneous: conditional actions, signals, icons, input files, windows.
- `notebook/` — `<notebook>`: tabs.
- `password/` — `<password>`: masked field.
- `pixmap/` — `<pixmap>`: image, refreshed or watched on disk.
- `progressbar/` — `<progressbar>`: progress bar.
- `pulse/` — `<pulse>`: activity bar with no known end.
- `radiobutton/` — `<radiobutton>`: exclusive choice.
- `searchentry/` — `<searchentry>`: search field.
- `spinbutton/` — `<spinbutton>`: numeric selector.
- `spinner/` — `<spinner>`: waiting indicator.
- `statusbar/` — `<statusbar>`: status bar.
- `switch/` — `<switch>`: on/off switch.
- `table/` — `<table>`: table.
- `terminal/` — `<terminal>`: embedded terminal (gtk3sermo and gtk4sermo, with VTE).
- `text/` — `<text>`: text label.
- `timer/` — `<timer>`: timer.
- `togglebutton/` — `<togglebutton>`: toggle button.
- `tree/` — `<tree>`: table with columns, icons, insertion, selection.
- `vbox/` — `<vbox>`: vertical box, scrollable.
- `vscale/` — `<vscale>`: vertical slider.
- `vseparator/` — `<vseparator>`: vertical separator.
- `window/` — `<window>`: window attributes and signals.

## Thunor's applications

Three complete applications, written by Thunor for gtkdialog (GPL-2.0 or later).

- `pfeme/` — a front-end to launch emulators ([README](pfeme/README), in English).
- `pfontview/` — view a TrueType font ([README](pfontview/README), in English).
- `playmusic/` — a music player ([README](playmusic/README), in English).

## Showcases

- `showcase/` — five visual landmarks, to see at a glance what a port can display ([README](showcase/README.md) (French only)).
- `system-tools/` — a six-tab Debian administration tool, taken all the way ([README](system-tools/README.md) (French only)).

## Jobs and toolboxes

- `metiers/` — sixty ready-to-run scripts, five per job, and the acceptance test of the ports ([README](metiers/README.en.md)).
- `boites-a-outils/` — four boxes (system, server, administrator, user), a launcher and ten tools each ([README](boites-a-outils/README.en.md)).

## Reading from and writing to the outside

- `modbus/` — read a Modbus/TCP controller, read-only, with a fake controller ([README](modbus/README.md) (French only)).
- `opcua/` — read an OPC UA server, read-only, with a fake server and test certificates ([README](opcua/README.md) (French only)).
- `domotique-mqtt/` — read MQTT sensors and control with a click, in native tags ([README](domotique-mqtt/README.md) (French only)).
- `mavlink/` — a drone's telemetry, and safety orders from an allow-list ([README](mavlink/README.md) (French only)).
- `lecteur-media/` — play a sound or a video: play, pause, volume, state ([README](lecteur-media/README.md) (French only)).

## Network and hardware

- `approbation/` — a human approval gate in one window; also used for the sends of `bus-can` and `liaison-serie` ([README](approbation/README.md) (French only)).
- `bluetooth-scan/` — Bluetooth devices in range, read-only ([README](bluetooth-scan/README.md) (French only)).
- `bus-can/` — listen to a CAN bus, and send a frame only after confirmation ([README](bus-can/README.md) (French only)).
- `camera-apercu/` — see a camera on a click, recording nothing ([README](camera-apercu/README.md) (French only)).
- `certificat-tls/` — a server's TLS certificate: chain, name, days left ([README](certificat-tls/README.md) (French only)).
- `dns-testeur/` — resolve a name and compare resolvers ([README](dns-testeur/README.md) (French only)).
- `gnss-moniteur/` — the state of a GNSS/GPS receiver, with a demo mode ([README](gnss-moniteur/README.md) (French only)).
- `heure-ntp/` — is the machine's clock right? ([README](heure-ntp/README.md) (French only)).
- `journal-direct/` — follow the systemd journal live, filtered ([README](journal-direct/README.md) (French only)).
- `liaison-serie/` — listen to a serial port, and write to it only after confirmation ([README](liaison-serie/README.md) (French only)).
- `reseau-testeur/` — TCP connections, port test, ping, frame, UDP and HTTP ([README](reseau-testeur/README.md) (French only)).
- `usb-moniteur/` — the USB devices plugged in, and plug events live ([README](usb-moniteur/README.md) (French only)).
- `wifi-scan/` — Wi-Fi networks in range and channel use ([README](wifi-scan/README.md) (French only)).
