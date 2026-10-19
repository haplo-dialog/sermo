# History of gtkdialog — from László Pere to today

[Français](HISTOIRE.md)

sermo descends from **gtkdialog**. This document traces who wrote gtkdialog, who
maintained it, when, and where its different lines stand today. Each fact
points to its source; what could not be established is said so.

The sources were read on 10 October 2026: GitHub repositories (commit history,
`AUTHORS`, `ChangeLog`, `NEWS` files), the Google Code archive, Debian and
Launchpad. Three sources did not answer that day: the old Puppy Linux forum
(access denied), the author's FTP server and the Wayback Machine.

## In short

| Period | Who | Where | Versions |
|---|---|---|---|
| 2003/2004 – 2007 | **László Pere** (program), Kiss Gábor (documentation) | personal site, University of Pécs | 0.58 → **0.7.20** |
| 2005 – 2009 | Debian and Ubuntu packaging, then removal | Debian, Ubuntu | up to 0.7.20 |
| 2011 – 2013 | **Thunor** | Google Code (svn) | 0.8.0, 0.8.2, **0.8.3** |
| 2018 – 2024 | **woof-CE contributors (Puppy Linux)** | `puppylinux-woof-CE/gtkdialog` | 0.8.4b → 0.8.5e |
| 2024 – 2025 | **Mick Amadio (01micko)**: gtk3dialog | `01micko/gtk3dialog` | 0.1.0 → 0.1.2 |
| 2025 | **BunsenLabs**: packaging of gtk3dialog | `BunsenLabs/gtk3dialog` | 0.1.2 (Debian package) |
| 2026 | **gtkdialog3**, published under László Pere's name | `laszlopere/gtkdialog3` | 0.9.0 announced |
| 2026 | **sermo** (this project), fork of 0.8.3 | `haplo-dialog/sermo` | 2.x then 3.x |

```
László Pere 0.58 … 0.7.20 (2004-2007)
   └─ Thunor 0.8.0 … 0.8.3 (2011-2013, Google Code)
        ├─ woof-CE / Puppy Linux 0.8.4b … 0.8.5e (2018-2024)
        │    └─ 01micko: gtk3dialog 0.1.x (2024-2025)
        │         └─ BunsenLabs: Debian package of gtk3dialog (2025)
        ├─ gtkdialog3 "GtkDialog2/3" 0.9.0 (2026, laszlopere account)
        └─ sermo 2.x then 3.x (2026, haplo-dialog)
```

## 1. László Pere, the author (2003/2004 – 2007)

- **Author**: the upstream `AUTHORS` file names "Program: Pere László" and
  "Documentation: Kiss Gábor".
  [woof-CE `AUTHORS`](https://github.com/puppylinux-woof-CE/gtkdialog/blob/master/AUTHORS)
- **Dates**: source headers read "Copyright (C) 2003-2007 László Pere". The
  oldest `ChangeLog` entry is **0.58.6, 22 April 2004**. The very first version
  and its date were not found.
- **Last version**: **0.7.20, 9 April 2007** (`ChangeLog`).
- **License**: GPL version 2 "or (at your option) any later version", i.e.
  **GPL-2.0-or-later**.
- **Hosting**: the author's personal site at the University of Pécs
  (`linux.pte.hu/~pipas/gtkdialog/`). The page still exists but only links to an
  FTP server that is unreachable today.
- **Numbering**: the 0.59.x series (2004-2005) was the "unstable" branch;
  numerically higher than 0.6 and 0.7, it forced Debian to add *epochs* (`1:`,
  then `2:`) to its package versions.

**Debian** packaged gtkdialog from 2005 (0.59.4, Fabio Tranchitella) to 2008
(2:0.7.20-4), then **removed it in 2009** for lack of a maintained upstream (bug
[#543945](https://bugs.debian.org/543945); Debian 5 "lenny" is the last release
to ship it). Ubuntu kept it until 10.04.
[tracker.debian.org/pkg/gtkdialog](https://tracker.debian.org/pkg/gtkdialog)

## 2. Thunor and the 0.8 series (2011 – 2013)

- On **16 June 2011**, Thunor created the `gtkdialog` project on Google Code and
  imported László Pere's **0.7.20** archive into it, with Debian's patches. The
  project page presented it as the continuation of László Pere's project, which
  he no longer had time to maintain.
  [Google Code archive](https://code.google.com/archive/p/gtkdialog)
- Released versions: **0.8.0** (26 September 2011), **0.8.2** (21 September
  2012), **0.8.3** (**6 January 2013**, last released archive). 0.8.1 was never
  released.
- In **March-April 2013**, Thunor added **experimental GTK 3** support to svn
  (`--enable-gtk3`). It was never released as an archive. His last commit is
  dated **9 April 2013**.
- Thunor's copyright notices read **2011-2012** (code) and 2011-2013 (examples).

The 0.8 series, by far the most widespread (Puppy Linux and its derivatives), is
why Thunor is sometimes taken for gtkdialog's author. The author is László Pere;
Thunor is its second maintainer.

## 3. woof-CE / Puppy Linux (2015 – 2024)

- The [`puppylinux-woof-CE/gtkdialog`](https://github.com/puppylinux-woof-CE/gtkdialog)
  repository was created on **14 March 2015** and carries **all of Thunor's svn
  history** (same commits, from the 0.7.20 import on).
- No commit between April 2013 and **March 2018**. The `AUTHORS` file then
  notes: "Continued by the woof-CE gtkdialog contributors 2018-03-19". Hence
  the two dates: 2015 (heading) is when the repository was created, 2018 (table)
  when the woof-CE contributions began.
- Main contributors: woodenshoe-wi (GTK 3 and VTE 2.91, 2018),
  **Mick Amadio (01micko)**, **Dima Krasner** (Meson build and **Wayland
  layer-shell anchoring**, November 2021), step- (`chooser` widget, XML
  comments, GTK 3 terminal, 2022), peabee (merges).
- GTK 2 and GTK 3 live side by side (Meson option `gtkver`).
- Versions: 0.8.4b (March 2021, first release published on GitHub), 0.8.5
  (March 2022), … **0.8.5e (13 September 2024)**, which is also the last commit.
- **Status on 10 October 2026**: not archived, 35 open issues, no commit since
  September 2024. It is the **direct continuation** of the Pere → Thunor line.

## 4. gtk3dialog: 01micko, then BunsenLabs (2024 – 2025)

- [`01micko/gtk3dialog`](https://github.com/01micko/gtk3dialog) is a GitHub fork
  of woof-CE (created in 2020). On **25 November 2024**, Mick Amadio renamed
  everything to **gtk3dialog** there, starting from woof-CE's last commit. He
  removed GTK 2 and autotools, and added manual pages and Wayland examples.
  Versions **0.1.x**; **0.1.2** is dated **11 May 2025**.
- [`BunsenLabs/gtk3dialog`](https://github.com/BunsenLabs/gtk3dialog) is a fork
  of `01micko/gtk3dialog`, created on **4 June 2025**. Its main branch is
  **identical** to 01micko's: BunsenLabs only adds the **Debian packaging**
  (`carbon` branch, John Crawley, June-July 2025) for its BunsenLabs "Carbon"
  distribution.
- **Layer-shell support was therefore not written at BunsenLabs**: it was
  written in woof-CE in 2021 (Dima Krasner, then Mick Amadio), and it reached
  BunsenLabs through gtk3dialog.
- **Status on 10 October 2026**: no commit since May 2025 at 01micko, July 2025
  at BunsenLabs.

## 5. gtkdialog3, published under László Pere's name (2026)

- [`laszlopere/gtkdialog3`](https://github.com/laszlopere/gtkdialog3) is a **new
  repository with no earlier history**: its first commit (14 March 2026) adds
  all 300 files of the project at once, ported from GTK 2 to GTK 3. The
  repository itself was created on GitHub on 22 May 2026.
- It starts from **gtkdialog 0.8.3** (Thunor), not from woof-CE nor gtk3dialog:
  its `ChangeLog` jumps straight from March 2026 to the January 2013 0.8.3.
- The project was first called "GtkDialog2", then "GtkDialog3"; it announces a
  **0.9.0** version, but no version is released (no tag, no archive) as of
  10 October 2026.
- Its `AUTHORS` and `NEWS` files present it as continued "by the original
  author". Some clues point that way (an e-mail address on the University of
  Pécs domain in two commits); this document does not hold it as proven and
  therefore writes "published under László Pere's name".
- License: GPL-2.0-or-later (SPDX headers).
- Last commit: 14 June 2026.

## 6. And sermo

- sermo starts from **gtkdialog 0.8.3** — the archive released by Thunor, which
  only knows GTK 2. It keeps the XML language and the parser, and rewrites the
  rest as a shared core and several rendering layers.
- `gtk3sermo`'s **Wayland anchoring** (attributes `layer`, `edge`, `dist`,
  `reserve`) was ported from gtk3dialog (BunsenLabs repository); it was written
  in woof-CE by **Dima Krasner** (2021) and extended by **Mick Amadio**
  (2021-2024). Their notices are kept.
- The documentation of sermo's MCP server follows the structure of gtkdialog3's
  `for-claude` documentation.
- What comes from upstream is detailed, file by file, in
  [LICENCES.en.md](LICENCES.en.md).

## What remains uncertain

- The date and number of gtkdialog's very first version.
- The content of the old Puppy Linux forum and of the author's FTP server
  (unreachable on 10 October 2026).
- The identity of the person behind the `laszlopere` account: plausible, not
  proven.
- The actual writing date of gtkdialog3's commits (dated March 2026, but
  published in May 2026).

## Sources

- <https://github.com/puppylinux-woof-CE/gtkdialog> (`AUTHORS`, `ChangeLog`, `NEWS`, commits, releases)
- <https://github.com/01micko/gtk3dialog> (commits, tags)
- <https://github.com/BunsenLabs/gtk3dialog> (`README.md`, `carbon` branch, `debian/changelog`)
- <https://github.com/laszlopere/gtkdialog3> (`AUTHORS`, `NEWS`, `ChangeLog`, commits)
- <https://code.google.com/archive/p/gtkdialog> (Thunor's project and downloads)
- <https://tracker.debian.org/pkg/gtkdialog>, <https://snapshot.debian.org/package/gtkdialog/>, <https://bugs.debian.org/543945>
- <https://launchpad.net/ubuntu/+source/gtkdialog>
- <https://aur.archlinux.org/packages/gtkdialog>
