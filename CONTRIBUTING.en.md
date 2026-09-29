# Contributing to sermo

[Français](CONTRIBUTING.md)

Thanks for your interest. sermo separates a hardened **core** (`libsermocore`)
and autonomous **rendering backends**, one per graphical library.

sermo has **a single maintainer**: a review may take time. For a significant
change, open an issue first to discuss it; that avoids work that could not be
merged.

## Reporting a bug

- Open an issue at <https://gitlab.com/haplo-dialog/sermo/-/issues> with a
  minimal XML script that reproduces the bug, the command you ran (`gtk3sermo`,
  `qt6sermo`…) and its `--version` output.
- Without a GitLab account, the same report is welcome at
  `devel@haplo-dialog.fr`.
- **A security vulnerability does not go into a public issue**: see
  [SECURITY.en.md](SECURITY.en.md).

## Before you start

- Read the [Developer manual](MANUEL_DEVELOPPEUR.en.md) — core/backend
  architecture, build variants, anatomy of a backend.
- Read [SECURITY.en.md](SECURITY.en.md) — the security rules are **not**
  negotiable.
- The [Code of conduct](CODE_OF_CONDUCT.en.md) applies to issues, merge requests
  and emails.

## Where to send a contribution

- **A merge request** on GitLab:
  <https://gitlab.com/haplo-dialog/sermo/-/merge_requests>. The CI there runs the
  two scripts described below.
- **Without a GitLab account**: patches produced by `git format-patch`, sent to
  `devel@haplo-dialog.fr`.
- The repository has a copy on GitHub; contributions go through GitLab or email.
  That copy runs the same benches on its own side
  (`.github/workflows/bancs.yml`, twin of `.gitlab-ci.yml`): a request landing
  there anyway is still tested, and the mirror cannot drift silently.

## Signing your commits (DCO)

Every commit carries a `Signed-off-by:` line with your name and an address where
you can be reached. `git commit -s` adds it. With that line you certify what the
**Developer Certificate of Origin 1.1** (<https://developercertificate.org/>)
says, in short:

- the contribution is yours; or it builds on work under a free license that
  allows you to submit it; or someone who certified the same passed it to you and
  you did not modify it;
- you submit it under the license of the file it touches (usually
  GPL-2.0-or-later, see [LICENCES.en.md](LICENCES.en.md));
- you accept that the contribution and your sign-off, name and address included,
  stay public in the project history.

The text on the website is the one that counts. A commit without a sign-off is
not merged. Your name stays the author of the commit.

## The golden rule: the benches

Every contribution must leave **the three gates green** for the affected
backend(s):

```sh
TIMEOUT=5 bash tests/xml/run_tests.sh <BIN>                       # 55 PASS
bash tests/comportement/run.sh <BIN>                              # 53 green
xvfb-run -a bash tests/garde_echappement_sortie.sh <BIN>          # OK — 4 cases
```

A merge request that turns a bench red is not merged. If you add a capability,
add the test case that checks it.

### Replay everything, like the CI

The repository CI does nothing but run two scripts. At home, on Debian testing
with the packages listed in [`ci/dependances.txt`](ci/dependances.txt):

```sh
bash ci/construire.sh      # the core and the seven backends (or a few: ci/construire.sh qt6 ncurses)
bash ci/bancs.sh           # every bench; one log per bench in _journaux/
```

`ci/bancs.sh` runs everything before concluding, prints one line per bench, and
returns 1 as soon as a bench is red, hung, or verified nothing. It also checks
that no personal path (`/home/<you>/`) or workstation timestamp header enters
the repository.

## Where code goes

- **Behaviour, grammar, execution, security** → in the **core**
  (`libsermocore`). It is shared by all backends: a fix there benefits everyone.
- **Widget rendering, toolkit glue** → in the relevant **backend**
  (`sermo-backend-<t>/src/widget_*.c`).
- When in doubt, prefer the core: parity between backends is the goal.
- **gtk3sermo is the reference**: another port that differs from it is fixed
  towards it. An `.attendu` file is never changed to match a port.

## Style and security

- C `-std=gnu11`, mandatory hardening (see [COMPILE.en.md](COMPILE.en.md) §4).
- **Never** `system`/`popen`/`strcpy`/`strcat`/`sprintf`/`gets` — use `safe_*` and
  bounds; nor `atof`/`strtod`, which depend on the system language
  (`g_ascii_strtod`). `tests/garde_fonctions_interdites.sh` checks it.
- SPDX header on every new file: `GPL-2.0-or-later` for code and documentation;
  `CC0-1.0` accepted for a test or an example written from scratch.
- Write code that looks like its neighbours (naming, comment density).

## Proposing a new backend

See the [Developer manual](MANUEL_DEVELOPPEUR.en.md) §5. In short: provide
the `widget_*.c`, `tag_set_property.c`, the `sermo_backend_toolkit_init` entry
point, a `CMakeLists.txt` consuming `sermocore` via pkg-config, then pass the
benches.

## Contact

`devel@haplo-dialog.fr`.
