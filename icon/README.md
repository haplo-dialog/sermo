# Marque sermo

Marque maître du projet **sermo** (haplo-dialog).

- `haplo-dialog-hd.svg` — le logo **HD**, source vectorielle des icônes
  d'application. C'est lui que portent les barres de titre des fenêtres.

Les icônes d'application des six ports graphiques
(`sermo-backend-*/data/icons/hicolor/<taille>/apps/<port>sermo.png`) sont
engendrées depuis `haplo-dialog-hd.svg`, aux huit tailles du thème hicolor
(16, 22, 24, 32, 48, 64, 128, 256), et installées par `debian/rules`.

Le port **ncurses** n'a pas d'icône : un terminal ne peut pas en porter. La
marque y est écrite en toutes lettres — « HD » dans le filet du haut de la
fenêtre (`sermo-backend-ncurses/src/render_ncurses.c`).

Seules des **images matricielles** sont installées : le chercheur d'icônes du
cœur préfère un SVG quand il en trouve un, et tous les ports ne savent pas le
décoder.

© 2026 haplo-dialog &lt;devel@haplo-dialog.fr&gt; — **GPL-2.0-or-later**, comme le
reste du dépôt.
