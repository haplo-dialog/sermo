# Licences - haplo-dialog

## Récapitulatif

| Composant | Fichiers | Licence | Raison |
|-----------|----------|---------|--------|
| Cœur et widgets | `src/*.c`, `src/*.h`, `src/safe_exec.c`, `src/variables.c`, `src/stringman.c`, `src/stack.c` | **GPL-2.0-or-later** | Héritage direct gtkdialog (László Pere), copyleft fort |
| Parser XML | `src/gtkdialog_lexer.l`, `src/gtkdialog_parser.y` | **GPL-2.0-or-later** | Héritage direct gtkdialog original |
| Documentation | `doc/*.texi`, `doc/*.info`, `man/`, `*-site/*.html` | **CC-BY-SA 4.0** | Standard documentation libre |
| Exemples | `examples/`, `tests/xml/*.xml` | **CC0-1.0** | Domaine public, utilisables sans restriction |
| Packaging | `packaging/` | **GPL-2.0-or-later** | Scripts de build liés au projet |

> **État vérifié sur la source.** Les fichiers source portent la
> clause « either version 2 of the License, or (at your option) any later
> version » ; aucun ne référence la version 3. Les en-têtes SPDX et les
> `debian/copyright` confirment **GPL-2.0-or-later** partout, hormis les
> exemples (CC0-1.0).

## Textes complets

- **GPL-2.0-or-later** : https://www.gnu.org/licenses/old-licenses/gpl-2.0.html
- **CC-BY-SA 4.0** : https://creativecommons.org/licenses/by-sa/4.0/
- **CC0-1.0** : https://creativecommons.org/publicdomain/zero/1.0/

## Héritage

haplo-dialog est un fork de **gtkdialog 0.8.3** par László Pere et Thunor,
distribué sous GPL-2.0+. haplo-dialog conserve **GPL-2.0-or-later** (continuité
directe avec l'amont).

Upstream original : https://github.com/01micko/gtkdialog

## Identification SPDX dans les sources

Chaque fichier source contient un identifiant SPDX en en-tête :

```c
/* SPDX-License-Identifier: GPL-2.0-or-later */  // cœur, widgets, compat
```

```sh
# SPDX-License-Identifier: CC0-1.0               // exemples
```

---

