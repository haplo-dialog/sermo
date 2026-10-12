# Job scripts

Sixty ready-to-run scripts, five per job, that show what sermo is for in real work:
backing up a folder, filing an expense claim, entering measurements in a workshop,
converting a video, configuring a server over SSH…

They have a second role: they are the **acceptance test of every port**.
`verifie.sh` plays each of their dialogs on the nine ports and checks that every
port returns the same values as gtk3sermo, the reference.

The scripts' interface and comments are in French (the folder names too); this file
describes them in English.

## The twelve jobs

| Folder | Job | Suggested port |
|---|---|---|
| `01-admin-systeme` | system administrator | gtk3sermo; ncursessermo over SSH |
| `02-developpeur-devops` | developer, DevOps | gtk3sermo, qt6sermo |
| `03-support-informatique` | IT support | gtk3sermo |
| `04-distribution-integrateur` | distribution, integrator | gtk3sermo, gtk4sermo |
| `05-bornes-ecrans` | kiosks and dedicated screens | lvglsermo |
| `06-atelier-industrie-labo` | workshop, industry, laboratory | lvglsermo, fltk1sermo |
| `07-pme-association` | small business, non-profit, office | gtk3sermo, htmlsermo |
| `08-enseignement-formation` | teaching, training | gtk3sermo, htmlsermo |
| `09-multimedia` | multimedia | qt6sermo, gtk3sermo |
| `10-securite-informatique` | IT security | gtk3sermo, ncursessermo |
| `11-serveur-sans-ecran` | headless server | ncursessermo |
| `12-web-local-intranet` | local web, intranet | htmlsermo |

Each script states at its top its job, its need, the command it wraps, the suggested
port and what is lost in a terminal.

## Running them safely

```sh
examples/metiers/01-admin-systeme/1-sauvegarde.sh gtk3sermo
```

The first argument is the port (default: `sermo`). **By default nothing is
changed**: the command the script would run is printed (`à blanc : rsync …`). To act
for real, after reading that command:

```sh
SERMO_A_BLANC=0 examples/metiers/01-admin-systeme/1-sauvegarde.sh gtk3sermo
```

A destructive action also asks for confirmation in a second dialog. No script calls
`sudo` or `pkexec`: when rights are needed, it says so.

## How they are written

The rules are in [CONVENTION.md](CONVENTION.md) (French only). In short:
- each dialog is a `dialogue_<name>` function that prints its XML;
- values come back through `--do` (in the environment), never through `eval`;
- `EXIT` tells which button was clicked: only the action button means consent;
- every action goes through `agir`, which prints it in dry-run mode.

They are public domain (CC0-1.0): copy and adapt them.

## The test

```sh
examples/metiers/verifie.sh                       # every script, nine ports
examples/metiers/verifie.sh --images folder/      # plus one picture per dialog
examples/metiers/verifie.sh --ports "gtk3 lvgl" examples/metiers/05-bornes-ecrans/*.sh
```

It checks each script's header and structure, has each dialog parsed by the nine
ports, then **plays** each dialog on each port. An invisible timer closes the dialog,
and the XML's commands are blocked. The values returned must be gtk3sermo's.
`ci/bancs.sh` runs it.

These dialogs are also served by `sermoia-mcp` (`sermo_examples`), under one topic per
job.

See also the [toolboxes](../boites-a-outils/README.en.md) (system, server,
administrator, user), written to the same rules and tested by the same bench.
