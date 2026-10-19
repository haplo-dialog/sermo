# Toolboxes

[Français](README.md)

Four boxes of settings and tools, each with a **launcher** and **ten tools**. The
launcher shows the tools in a single window; one click opens a tool, then you come
back to the launcher.

Like the [job scripts](../metiers/README.en.md), they are also the **acceptance test
of the ports**: `examples/metiers/verifie.sh` plays each of their dialogs on the nine
ports and compares the values with gtk3sermo.

The scripts' interface and comments are in French (the folder names too); this file
describes them in English.

## The four boxes

| Folder | For whom | Suggested port |
|---|---|---|
| `systeme` (system) | the person at their desktop, adjusting it | gtk3sermo, qt6sermo |
| `serveur` (server) | the person running a server, often over SSH | ncursessermo |
| `administrateur` (administrator) | the person administering a Debian machine | gtk3sermo; ncursessermo over SSH |
| `utilisateur` (user) | everyone, no technical knowledge needed | gtk3sermo, qt6sermo, htmlsermo |

**System**: display, sound, Wi-Fi, date and time, keyboard and language, mouse and
touchpad, power, startup applications, default applications, Bluetooth.

**Server**: nginx site, TLS certificates, listening ports, SSH hardening, fail2ban,
database, log rotation, NFS and Samba shares, e-mail alerts, systemd timers.

**Administrator**: groups, permissions on a folder, packages, apt sources, mounts
(fstab), formatting, kernel modules, sudo rules, host name, scheduled reboot.

**User**: find files, archives, password, reminder, print, convert a document to
PDF, tidy a folder, copy to a USB stick, lend a file on the local network, disk
space.

## Running them safely

```sh
examples/boites-a-outils/utilisateur/lanceur.sh gtk3sermo
```

The first argument is the port (default: `sermo`). **By default nothing is
changed**: the command the tool would run is printed (`à blanc : …`). To act for
real, after reading that command:

```sh
SERMO_A_BLANC=0 examples/boites-a-outils/utilisateur/lanceur.sh gtk3sermo
```

- A destructive action asks for confirmation in a second dialog (formatting asks you
  to type the disk name again).
- No tool calls `sudo` or `pkexec`. When root rights are needed, the tool says so
  and prints the command to run as root.
- Input is checked before acting: allowed values, normalised paths, system folders
  refused.

## How they are written

The rules are those of the job scripts: [CONVENTION.md](../metiers/CONVENTION.md) (French only).
Public domain (CC0-1.0): copy and adapt them.

These dialogs are also served by `sermoia-mcp` (`sermo_examples`), under one topic
per box.
