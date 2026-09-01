# Changelog - haplo-dialog

Toutes les modifications notables de ce projet sont documentées ici.  
Format : [Keep a Changelog](https://keepachangelog.com/fr/1.0.0/)  
Versionning : [Semantic Versioning](https://semver.org/lang/fr/) à partir de 1.0.0.

---

## [Unreleased] - v1.2.0 (en cours)

### Ajouté

- **`qt6sermo` — troisième port, Qt 6.** La même syntaxe XML que gtk3sermo et
  gtk4sermo, rendue avec Qt 6 (Qt Widgets). « Écrit une fois, tourne en GTK 3,
  GTK 4 ou Qt 6. » Version 1.0.0 (port le plus jeune ; versionné indépendamment
  des autres). Sa parité de valeur avec le port GTK 3 de référence est vérifiée
  par un banc de comportement de 23 cas, livré dans son arbre
  (`qt6sermo_1.0.0/tests/comportement/`). Empaquetage `.deb` fourni. La sortie
  au shell passe par le même durcissement que les autres ports : échappement
  complet (`\ " $ \``), liste blanche `HAPLO_ALLOWED_CMDS` opt-in, `--do`.

### Corrigé

- **Port GTK 4 — `<tree>` :** poser la propriété `stock-id` sur le rendu de
  cellule émettait des `GLib-GObject-CRITICAL` à chaque ligne — cette propriété a
  été supprimée de `GtkCellRendererPixbuf` en GTK 4. Un `stock-id` s'y lit
  désormais comme un `icon-name`. La valeur exportée par `<tree>` était déjà
  correcte ; les avertissements disparaissent.
- **Port GTK 4 — `<table>` :** deux défauts. (1) L'en-tête était lu depuis le
  premier `<item>`, qui consommait la seule ligne de données — il est désormais
  lu depuis `<label>`, comme dans le port GTK 3, et **tous** les `<item>`
  deviennent des lignes. (2) Les cellules étaient découpées sur tabulation ; un
  `<item>col1|col2</item>` restait une cellule unique. Le découpage se fait sur
  `|` pour les `<item>`/`<label>` (la tabulation reste pour l'entrée par
  commande). Une `<table>` restitue enfin la valeur de sa colonne.

Ces deux défauts ont été trouvés en comparant, widget par widget, la valeur que
le port GTK 4 restitue au shell à celle du port GTK 3 de référence.

---

## [1.1.3] - 2026-08-29

### Corrigé

- ⚠️ **Un plantage qui touchait les DEUX ports.** Cliquer le bouton de fermeture
  d'un `<infobar>` tuait le programme par SIGSEGV. `widget_infobar.c` branchait
  `on_any_widget_changed_event(GtkWidget *, AttributeSet *)` sur le signal
  `response`, dont l'arité réelle est `(GtkInfoBar *, gint, gpointer)` —
  `g_signal_query` rend `n_params=1, param[0]=gint`. Le `response_id` arrivait
  donc **à la place du pointeur** `AttributeSet`. Reproduit par sonde compilée :
  en passant `0x55aa55aa0000` à `g_signal_connect`, le gestionnaire reçoit
  `0xfffffff9`, c'est-à-dire `-7`, `GTK_RESPONSE_CLOSE`. Un trampoline rétablit
  l'arité. **Toutes les versions antérieures portent ce défaut**, gtk3sermo
  compris.
- **Un fichier `.ui` contenant un `<menu>` faisait planter le port GTK 4**
  (SIGABRT). En GTK 4, `gtk_builder_get_objects()` rend aussi des objets qui ne
  sont pas des widgets ; `gtk_widget_get_name()` y rendait `NULL`, et
  l'assertion de `new_tag_attributeset` sautait.
- `<calendar>` rendait la **date du jour** au lieu de la date demandée : les
  macros qui la posent valaient `((void)0)`. En GTK 4 on ne peut pas régler mois
  et jour séparément — `set_month` sur un 31 décembre rend
  `assertion 'date != NULL' failed` et ne change rien. Une date complète et
  valide est désormais posée en un seul appel.
- `<infobar>` était un `GtkBox` déguisé : le type de message était ignoré.
  `GtkInfoBar` existe toujours en GTK 4.22, déprécié mais présent ; seule
  `get_content_area()` a disparu.
- `<pixmap>` n'a **jamais** chargé une icône de thème : `gtk_icon_theme_load_icon`
  était remplacée par un stub rendant `NULL`.
- L'icône de fenêtre **annonçait un succès sans rien faire** : la compatibilité
  valait `(TRUE)`. Mesuré par `xprop _NET_WM_ICON` — la référence rend
  `Icon (64 x 64)`, le port rendait `Icon (32 x 32)` toute blanche.
- `--geometry=+X+Y` était avalé sans un mot. Implémenté sur X11 par
  `XMoveWindow` ; sur Wayland le protocole l'interdit, et un avertissement le
  dit au lieu de faire croire que l'option a été honorée.
- `disable:` sur un `<menu>` entier n'avait aucun effet.
- Les menus d'un fichier `.ui` n'étaient pas reconnus : cinq macros
  `GTK_IS_MENU_*` valaient `(FALSE)`.

### Supprimé

- Les accélérateurs, **supprimés plutôt qu'implémentés** : la liste
  `accel_groups` n'était jamais remplie. Ce n'était pas un portage incomplet,
  c'était du code inatteignable — un shim qui protège du code mort donne
  l'illusion d'un chantier en cours.

### Ajouté

- **`tests/comportement/`** : un banc qui **lance** le dialogue sous Xvfb et
  compare les variables rendues, le port GTK 3 servant d'oracle en mode
  `--diff`. La suite `tests/xml/` lance `--print-ir` : elle **analyse** le XML
  sans construire un seul widget, et ne dit donc rien du comportement. C'est ce
  malentendu qui a laissé douze fonctions mortes en place. Plus
  `geometrie.sh`, pour ce qu'aucune variable ne trahit (`border-width`, taille
  d'icône de thème).

---

## [1.1.2] - 2026-08-29

### Corrigé

- ⚠️ **Une perte de données.** `<filechooser>` avec `<output file:…>` faisait
  `fopen("w")` — qui **tronque** — puis n'écrivait rien, la valeur étant
  toujours vide. Mesuré : un fichier de 26 octets tombait à **0**.
- **`<checkbox>` et `<radiobutton>` rendaient une valeur FAUSSE en silence.**
  GTK 4 a cassé l'héritage `GtkCheckButton` → `GtkToggleButton` : mesuré,
  `gtk_check_button_new()` donne `GTK_IS_TOGGLE_BUTTON=0`. Appeler
  `gtk_toggle_button_get_active()` dessus rendait **toujours FALSE**.
  `<checkbox><default>true</default>` sortait `CB="false"` : un script testant
  `[ "$CB" = "true" ]` prenait systématiquement la mauvaise branche.
- **`border-width` était avalé sans le moindre avertissement.** L'attribut ne
  passe pas par la macro de compatibilité mais par le chemin générique des
  propriétés GObject ; en GTK 4, `GtkContainer` a disparu et la propriété
  n'existe plus. 47 des exemples livrés l'utilisent. Mesuré : une même fenêtre
  passe de 56×16 quelle que soit la valeur, à 56×16 / 96×56 / 136×96 pour
  0 / 20 / 40 — soit **+2N exact**.
- **Les entrées de menu cochables n'existaient pas.** GTK 4 a supprimé
  `GtkCheckMenuItem` ; l'état vit désormais dans une `GSimpleAction` étatique.
  Les radios d'un même `<menu>` s'excluent, et `toggled` n'est émis que sur
  celles qui **changent réellement**.
- **`<filechooser>` était un bouton ordinaire.**
  `gtk_file_chooser_button_new(title, action)` valait
  `gtk_button_new_with_label(title)` : l'attribut `action="select-folder"` était
  jeté, le signal `"file-set"` n'existe pas sur un `GtkButton`. Réimplémenté sur
  `GtkFileDialog`, asynchrone.

---

## [1.1.1-3] - 2026-08-28

### Corrigé

- `glade_support.c` : un `return TRUE;` était **hors du `if`**, malgré son
  indentation. La fonction renvoyait vrai dès la première itération, sans égard
  au résultat de la comparaison : seul `signals[0]` pouvait être connecté, et un
  échec de connexion était annoncé comme un succès. **Les deux ports.**
- `widget_tree.c` : dans `widget_tree_save()`, `text` n'est affectée qu'à
  l'intérieur de la boucle sur les colonnes. Si elle ne tourne pas, `text` était
  lue par `fprintf` puis passée à `g_free()` **sans avoir jamais été
  initialisée**. **Les deux ports.**
- Port GTK 4 : `if (!paramspec->flags & G_PARAM_WRITABLE)` — le `!` lie plus
  fort que le `&`, l'expression valait `(!flags) & 2`, donc **toujours 0**. Le
  garde-fou « propriété non modifiable » ne s'est jamais déclenché. Le port
  GTK 3 portait ce correctif ; le port GTK 4 ne l'avait jamais reçu.
- Port GTK 4 : avertissements de compilation ramenés de 36 à 17 — quatre `else`
  ambigus accolés sans changer leur sens, un paramètre qui masquait un global
  (renommé **avec ses trois usages**, sans quoi la comparaison qui décide quels
  widgets sont détruits serait passée du paramètre au global), six comparaisons
  signe/non-signe, et `%option noinput` dans le lexer.

---

## [1.1.1-2] - 2026-08-28

### Corrigé

- **`_FORTIFY_SOURCE` était défini deux fois à la compilation** : `=2` par les
  drapeaux Debian dans `CPPFLAGS`, puis `=3` par `src/Makefile.am`. GCC retenait
  bien la seconde — le durcissement annoncé était celui appliqué, vérifié par
  une sonde et par les binaires — mais il avertissait à **chaque fichier
  compilé** : 125 lignes de bruit sur les deux ports, qui noyaient les
  60 avertissements de code réels. `-U_FORTIFY_SOURCE` désarme la première.
- **`HARDENING_CFLAGS` et `HARDENING_LDFLAGS` étaient déclarés puis ignorés.**
  `configure.ac` les définissait, aucun `Makefile.am` ne les référençait : le
  durcissement venait de listes recopiées en dur, et les deux listes avaient
  **divergé** — `-fPIE` manquait à la première, `-Wl,-z,ibt -Wl,-z,shstk` à la
  seconde. Les câbler telles quelles aurait donc **affaibli** les binaires ;
  elles ont été complétées d'abord. Preuve que rien n'a bougé : les jeux de
  drapeaux de compilation et de lien, extraits et triés avant et après, sont
  identiques.

### Modifié

- Documentation : le manuel développeur GTK 4 annonçait **30 widgets**, le port
  en a **56** ; celui de GTK 3 disait « 43 fichiers, un par widget », alors que
  43 fichiers portent **52 widgets**. Les deux manuels se présentaient en 1.0.0
  dans des dossiers 1.1.1.

---

## [1.1.1] - 2026-08-27

### Supprimé
- **Les paquets de la release `v1.0.0` sont retirés (2026-08-27)** : les cinq
  `.deb` (`gtk3sermo 1.0.0-10`, `gtk4sermo 1.0.0-11`, `gtksermo 1.0.0-10` et les
  deux paquets de symboles) portaient les trois défauts corrigés depuis — la
  sortie rendue au shell exécutable par `eval` si l'utilisateur du dialogue tape
  `$(commande)` dans un champ, le `<switch>` qui tue le programme au clic, et le
  nombre à point qui vaut zéro en silence sous locale française. Les laisser
  téléchargeables « pour l'historique » n'était pas défendable : un moteur de
  recherche ou un vieux signet y mène aussi bien qu'à la version courante.
  La page de release est retirée avec eux.
  Ce que l'opération a appris : **une release et un paquet de registre sont deux
  objets distincts.** Les fichiers vivent dans le registre générique
  (`packages/generic/sermo/1.0.0/`) et la release n'en tient que des liens —
  supprimer la release seule les aurait laissés accessibles par leur URL directe.
  Le registre est donc supprimé **en premier**, pour qu'un échec en cours de route
  laisse une page visible aux liens morts plutôt que des paquets dangereux
  accessibles en silence. Et le rôle compte autant que la portée du jeton :
  d'après la documentation GitLab, **créer** une release demande *Developer*,
  mais **supprimer** un paquet ou une release demande *Maintainer*.
  L'**étiquette git `v1.0.0` est conservée** : elle ne distribue rien et date le
  premier commit public. Les paquets retirés sont archivés hors dépôt, sommes
  vérifiées, pour que l'opération reste rattrapable.

### Corrigé
- **Les deux ports ne pouvaient pas s'installer ensemble (2026-08-27)** : `gtk3sermo` et `gtk4sermo` livraient tous deux `/usr/share/man/man5/haplo-dialog-xml.5.gz`, sans aucune relation déclarée entre eux. `dpkg` refusait le second — dans les deux ordres, et même donnés ensemble en une seule transaction. Or la description des deux paquets, le `README` et la note de la release affirmaient qu'ils « s'installent sans conflit ». Une seule collision sur 219 et 214 fichiers, mais elle frappait exactement le scénario promis. Le port GTK 4 livre désormais sa propre page, `gtk4sermo-xml.5` ; le port de référence garde le nom canonique. Mesuré après correctif dans une racine `dpkg` vierge : **0 fichier en commun**, dépaquetage conjoint `rc=0`, les deux binaires et les deux pages présents.
- **La page de manuel promettait l'ancrage Wayland dans les deux ports (2026-08-27)** : elle décrivait `layer`/`edge`/`dist`/`reserve` comme fonctionnels, et elle était livrée **à l'identique** par les deux ports. Mesuré sur les binaires livrés : `nm -D` rend **8** symboles `gtk_layer_*` côté `gtk3sermo` et **0** côté `gtk4sermo`, qui ne lie aucune bibliothèque layer-shell ; `git log -S "gtk_layer_init_for_window"` ne rend aucun commit côté GTK 4 — ce code n'a jamais existé là. Ce qui trompe : `libgtk-layer-shell0` est bien installé sur la machine de développement, mais c'est la bibliothèque **GTK 3** ; celle de GTK 4 est un paquet distinct. La page du port GTK 4 dit maintenant ce qu'il fait vraiment, et le comportement a été vérifié : les quatre attributs y sont ignorés en silence et la fenêtre s'ouvre normalement.
- **L'origine du code Wayland n'était pas dans l'en-tête ni dans le manuel (2026-08-27)** : l'ancrage est porté du fork **BunsenLabs gtk3dialog**, où il a été écrit par Dima Krasner (2021) puis étendu par Mick Amadio (2021-2024), sous GPL-2.0-or-later. L'attribution existait dans `AUTHORS`, `LICENCES.md`, `debian/copyright` et en commentaire de milieu de fichier — mais pas dans les lignes `Copyright` de `widget_window.c`, ni dans la page de manuel que lit l'utilisateur. Les deux sont posées.
- **Les pages de manuel ne suivaient pas la version (2026-08-27)** : les six portaient « 1.0.0 » **en dur** dans leur ligne `.TH`, si bien que le pied de page rendu par `man` mentait à chaque montée — dans une version dont l'argument est « il n'y a plus qu'une chose à lire ». Elles deviennent des gabarits `.in` remplis par `configure`, même patron que les `.spec.in`. Vérifié sur les pages **livrées dans les `.deb`** : les quatre disent la version courante.
- **`make install` écrasait en silence un `gtkdialog` existant (2026-08-27)** : le hook autotools posait le lien par `ln -sf … || true` — écrasement muet d'un `gtkdialog` de distribution, et le `|| true` masquait jusqu'à l'échec. C'est le contraire de ce que promettent le `README` et les descriptions de paquet. L'alias passe derrière `--enable-gtkdialog-alias`, **éteint par défaut**, et le hook refuse d'écraser une cible qui n'est pas un lien. Mesuré : 0 fichier `gtkdialog*` posé par défaut, 2 avec l'option. Le paquet `gtksermo` reste la voie propre — il déclare le conflit au vu et au su de `dpkg` — et devient un paquet de liens purs.
- **Les exemples livrés appelaient des commandes que les paquets ne fournissent pas (2026-08-27)** : sur 239 exemples, **215** appelaient `gtkdialog`, fourni par le seul paquet `gtksermo` que rien ne rend obligatoire — et côté GTK 4 cela lançait donc le port **GTK 3** ; **17** appelaient `gtkdialog4`, qu'**aucun** des cinq paquets ne fournit. Tous passent à `GTKDIALOG=${GTKDIALOG:-<port>}` : chacun appelle le binaire de son propre port, et reste surchargeable. Vérifié en lançant un exemple avec un `PATH` ne contenant **que** le binaire du port : la fenêtre s'ouvre, aucune commande introuvable.
- **`<spinbutton>` du port GTK 4 imprimait deux avertissements GTK à chaque ouverture (2026-08-27)** : `icon-press`/`icon-release` étaient branchés sur un `GtkSpinButton`. En GTK 3 celui-ci dérive de `GtkEntry` et hérite de ces signaux ; en GTK 4 non — c'est un `GtkWidget` qui implémente `GtkEditable`. Bloc recopié du port GTK 3 sans revérification, son commentaire disait encore « GTK3: always available ». Mesure par widget : `<spinbutton>` **2** avertissements, `<entry>` **0**, `<entry password="true">` **0**. Après correctif : **0**.
- **`--version` affichait deux fois la version (2026-08-27)** : « gtk3sermo version 1.1.0 gtk3sermo 1.1.0 » d'un côté, « … 1.1.0 1.1.0-gtk4 » de l'autre — soit deux numéros à lire, ce que la 1.1.0 disait justement avoir supprimé. `BUILD_DETAILS` était écrit en dur dans `configure.ac` et répétait ce que `PACKAGE_NAME`/`PACKAGE_VERSION` impriment déjà.
- **La suite XML rendait un succès muet (2026-08-27)** : `./tests/xml/run_tests.sh all` sortait `rc=0` avec une **sortie vide** et zéro test exécuté pour qui avait suivi la recette « cloner puis construire » sans installer — le script ne cherchait que dans le `PATH`. Le lecteur croyait avoir vérifié 55/55. Elle cherche maintenant aussi dans l'arbre construit et sort en `rc=2` si elle ne trouve rien. Éprouvé dans les deux sens.
- **`make check` restait vert avec un binaire entièrement cassé (2026-08-27)** : c'est le contrôle que `CONTRIBUTING.md` exige avant fusion. Deux causes cumulées. Le wrapper engendré lançait `run_tests.sh` **sans argument**, si bien que le script cherchait le binaire dans le `PATH`, ne trouvait pas celui du build, et **sautait** les sections qui l'éprouvent — saut qu'automake résumait en `PASS`. Et les épreuves elles-mêmes ne pouvaient pas échouer : chacune ne cherchait qu'un mot dans la sortie, de sorte qu'un binaire n'imprimant **rien** les passait toutes. Sabotage mesuré avant correctif : un faux binaire réduit à `exit 1` donnait **49 passés / 0 échoué**. Le wrapper passe désormais le binaire construit, un binaire introuvable est un échec et non un saut, et les épreuves exigent une preuve **positive**. Éprouvé par deux sabotages : binaire muet et binaire trop permissif, tous deux attrapés.
- **L'avertissement de `--do` criait « injection risk » sur son propre exemple (2026-08-27)** : l'exemple donné par la page de manuel déclenchait `safe_system: shell fallback (injection risk)` à chaque exécution, alors que la note de release présente `--do` comme la voie sûre. La commande de `--do` vient de l'**auteur du script**, qui a déjà le droit de lancer des commandes — c'est le modèle de confiance assumé. Le message devient factuel ; **le refus reste une alarme**, vérifié sous `HAPLO_NO_SHELL_FALLBACK=1`.
- **Empaquetage (2026-08-27)** : `libgtk-3-0 (>= 3.22)` et `libvte-2.91-0` étaient écrits à la main **en double** de `${shlibs:Depends}`, qui les calcule avec les bons noms `t64` — le nom écrit à la main n'existe plus comme paquet réel dans testing, il n'est satisfait que par un `Provides` de transition. Retirés des deux ports. Les overrides lintian `initial-upload-closes-no-bugs` sont morts et signalés comme inutilisés : supprimés. Le `.SRCINFO` avait de nouveau divergé de son `PKGBUILD` (`texinfo` manquant), or c'est lui que les outils Arch lisent en premier. La page de manuel renvoyait au suivi d'incidents du projet, qui répond **404**. `src/gtkdialog.1` est supprimé : installé par rien, il affirmait pourtant « This file is installed as /usr/share/man/man1/gtkdialog.1 ».

---

## [1.1.0] - 2026-08-26

### Ajouté
- **Les manuels info sont livrés (2026-08-26)** : `info_TEXINFOS` était commenté dans les deux `doc/Makefile.am`, et aucun `.install` n'emportait de manuel. Une fois le paquet installé, `info gtk3sermo` ne trouvait **rien** — les manuels n'existaient que dans l'archive source. La raison invoquée en commentaire était d'éviter la dépendance `makeinfo` au moment du build : `texinfo` entre donc dans les `Build-Depends`, dans les recettes Arch, RPM et Gentoo, et dans les trois pipelines.
  Deux défauts sont sortis en cours de route, et ils sont instructifs :
  - **`version.texi` portait des variables qu'automake écrase.** Il contenait `@set PORT`, `@set BINARY` et `@set TOOLKIT`. Or automake **régénère** ce fichier dès que `info_TEXINFOS` est actif, et n'y met que `UPDATED`, `UPDATED-MONTH`, `EDITION` et `VERSION`. Le manuel installé affichait donc, en toutes lettres, `{No value for 'TOOLKIT'}`. Ces variables vivent maintenant dans le `.texi`, qu'automake ne touche pas.
  - **Sans `@dircategory` ni `@direntry`, le manuel s'installe mais reste introuvable.** Il n'apparaissait pas dans le menu de l'index Info : on ne pouvait l'ouvrir qu'en connaissant déjà son nom de fichier. Mesuré : le menu du fichier `dir` était vide. Les deux directives sont posées, et les manuels apparaissent sous « Interfaces graphiques ».
  Vérifié en extrayant les paquets construits et en lançant `info` pour de vrai : `info gtk3sermo` et `info gtk4sermo` ouvrent le manuel, la navigation par nœud fonctionne (`info gtk3sermo Security` rend bien le chapitre Sécurité, accents compris), et `lintian` ne signale rien sur le manuel — ses seules remarques restantes sont le correcteur orthographique anglais sur du changelog français.
- **Dix faits extraits du manuel amont orphelin (2026-08-25)** : `doc/gtkdialog.texi` est le manuel de László Pere (2003-2007), présent dans git mais absent de l'archive publiée, et périmé de son propre aveu. En le dépouillant, dix comportements que le programme a **depuis toujours** et que rien ne documentait sont remontés. Chacun a été éprouvé dans le code **et** rejoué en exécution, puis soumis à un vérificateur chargé de le démolir : trois formulations sur dix ont été corrigées à ce stade — dont deux qui affirmaient « documenté nulle part » alors que ça l'était.
  - **La table complète des actions.** Le programme reconnaît **22** préfixes ; la page de manuel en documentait **7**. Les quinze autres sont désormais décrits : `command:`, `launch:`, `closewindow:`, `activate:`, `grabfocus:`, `presentwindow:`, `save:`, `fileselect:`, `removeselected:`, `break:`, `insert:`, `append:`, `pulse:`, `start:`, `stop:`. Avec le point qui explique le reste : **un préfixe inconnu n'est jamais signalé**, la chaîne entière part au shell. C'est ce qui rendait `CLOSE:` silencieux.
  - **Portée réelle de `closewindow:`** : n'importe quel widget nommé, pas seulement les fenêtres ouvertes par `launch:` — et sur la dernière fenêtre, il imprime la seule ligne `EXIT="closewindow"`, **sans** les valeurs des widgets, contrairement à `exit:`.
  - **Où écrit `save:`** — la directive `<output file>` — et le **piège de locale** : sous `fr_FR` les valeurs numériques sortent avec une virgule (`0,4`), donc illisibles pour `bc` ou `awk` sans `LC_NUMERIC=C`.
  - **Les préfixes conditionnels `if true ` et `if false `**, utilisés par **11 exemples livrés** et documentés nulle part. L'espace après est obligatoire, et il en faut exactement un.
  - **L'action par défaut d'un bouton** : sans `<action>`, il sort en imprimant `EXIT="<sa première étiquette>"` ; sans étiquette du tout, « OK » lui est imposé. C'est le mécanisme qui fait fonctionner **tous** les exemples du dépôt, et aucun document ne l'expliquait. `<togglebutton>` en est exclu.
  - **`<visible>` est un synonyme de `<sensitive>`** : il **grise**, il ne cache pas — c'est l'attribut `visible="false"` qui retire de l'affichage. Sur une `<entry>`, la valeur `password` masque la saisie, orthographe héritée qui explique de vieux scripts. **Le masquage est purement visuel** : la valeur ressort en clair sur la sortie standard.
  - **Le mode fichier auto-exécutable** (`#!/usr/bin/gtk3sermo -f`), avec ses deux limites mesurées : une seule ligne est jetée, et seulement en lecture de fichier — avec `--stdin` elle ne l'est pas.
  - **Dans un `<menu>`, le `<label>` se déclare APRÈS les `<menuitem>`.** En tête, c'est une erreur de syntaxe, dans les deux ports. La page de manuel affirmait l'ordre inverse.
- **`--do=CMD` (2026-08-24)** : l'option existait dans les pages de manuel installées et dans le manuel info depuis toujours, sans exister dans aucun binaire — qui recopiait l'exemple officiel obtenait un abandon. Elle exécute la commande après la fermeture de la fenêtre, valeurs des widgets exportées dans l'environnement, via `safe_system()` : `exec()` direct hors métacaractères, repli `/bin/sh -c` journalisé sinon, refus net sous `HAPLO_NO_SHELL_FALLBACK=1`. Branchée par `atexit()`, parce que deux sorties directes ne repassent pas par la fin de `main()`.
- **`HAPLO_ALLOWED_CMDS` (2026-08-24)** : borne facultative des commandes lançables (`HAPLO_ALLOWED_CMDS=ls,cat,date`). **Éteinte par défaut**, à dessein : le langage sert à lancer des commandes et 14 des exemples livrés appellent `bash` ou `sh`. Elle vise celui qui *déploie* un dialogue dans un contexte moins fiable. Le nom comparé est le nom de base, et tant qu'elle est posée le repli shell est refusé — sinon `sh -c` la contournerait.
- **Bancs de non-régression (2026-08-22 → 25)** : `tests/run_examples.sh` lance réellement les exemples sous Xvfb au lieu de vérifier qu'ils existent ; `garde_progressbar_thread.sh`, `garde_maxwidgets.sh`, `garde_option_do.sh`, `garde_allowed_cmds.sh`, `garde_durcissement.sh`, `garde_spdx.sh` et `garde_fonctions_interdites.sh` tiennent chacun une promesse du dépôt. Tous branchés dans la CI, sur les deux ports.
- **`garde_fonctions_interdites.sh` (2026-08-25)** : `SECURITY.md` affirme qu'aucune fonction interdite n'est appelée dans `src/`. Une affirmation pareille est vraie le jour où on l'écrit et plus personne ne la revérifie — c'est ainsi que trois `strtod()` ont survécu quatre mois. Le banc la rejoue à chaque poussée sur les deux `src/`, pour `strcpy`, `strcat`, `sprintf`, `vsprintf`, `gets`, `system`, `popen`, `atof` et `strtod`. Deux pièges traités : les **commentaires** sont retirés avant la recherche (`safe_exec.c` cite légitimement `system()` et `popen()` pour dire qu'il les remplace), et une frontière de mot est exigée **à gauche**, faute de quoi `safe_system` compterait pour `system` et `g_ascii_strtod` pour `strtod`. Éprouvé par six sabotages, dont un appel réel, un appel en commentaire simple, un en commentaire multiligne, et un répertoire vide qui doit échouer au lieu de passer en silence.
- **Port `gtk4sermo` publié (2026-08-21)** : second backend, même grammaire, quatre balises de plus (`flowbox`, `overlay`, `revealer`, `stack`). La CI le construit et l'éprouve désormais comme l'autre.
- **Ancrage Wayland de `<window>` (2026-08-20)** : quatre attributs, `layer` (`background`/`bottom`/`top`/`overlay`), `edge` (les 4 bords, les 4 coins, et 6 formes « bande » qui arriment deux bords opposés), `dist` (marge 0-200 px, 20 par défaut) et `reserve` (`yes`/`no`, `no` par défaut : la surface flotte au-dessus des fenêtres ordinaires, ou bien le compositeur lui réserve sa place et dispose les autres à côté), transforment le dialogue en surface *wlr-layer-shell* : barre, dock, widget de bureau. Portage du fork gtk3sermo de BunsenLabs (GPL-2.0+), avec deux corrections par rapport à l'original : la marge, lue dans un type non signé puis contrôlée par un `< 0` qui ne peut jamais se déclencher, est désormais analysée par `strtol()`, refuse une fin de chaîne parasite et est bornée à [0, 200] ; et la détection de Wayland, qui lisait la variable d'environnement `GDK_BACKEND` et continuait quand elle n'était pas définie (le cas normal sous X11), interroge maintenant la bibliothèque elle-même (`gtk_layer_is_supported()`). Hors Wayland, ou sur un compositeur sans le protocole (GNOME), les quatre attributs sont ignorés et une fenêtre ordinaire s'ouvre. Dépendance optionnelle `gtk-layer-shell ≥ 0.8.0` ; `./configure --without-layer-shell` s'en passe. Documenté dans `haplo-dialog-xml(5)`, exemple dans `examples/layer-shell`. **Ancrage éprouvé sous sway 1.12 (wlroots 0.20) et mesuré au pixel** : barre `topstride` collée en haut sur toute la largeur, dock `bottom` `dist="24"` à exactement 24 px du bord, bande `background` intégralement recouverte par une fenêtre ordinaire, `dist="0"` et `dist="60"` séparés de 60 px exactement. `reserve="yes"` mesuré par `swaymsg` : une barre de 48 px repousse la fenêtre ordinaire à `y=48` avec `dist="0"`, et à `y=88` avec `dist="20"`, la marge comptant des deux côtés ; `reserve="no"` et l'absence d'attribut laissent la fenêtre occuper toute la sortie. Non couvert : le multi-écran, le matériel réel, et les compositeurs hors wlroots comme Hyprland.
- **Compatibilité ascendante gtkdialog (2026-06-06)** : `make install` pose un **symlink `gtkdialog` → `gtk3sermo`** (et `gtkdialog.1` → `gtk3sermo.1`) via le hook autotools ; un dialogue d'époque (`export MAIN_DIALOG='<window …>'; gtkdialog --program=MAIN_DIALOG`) parse, s'exécute et rend sa sortie au format historique (`VAR="valeur"`). La cohérence du symlink est répercutée dans chaque recette de paquet (Debian `.links`, RPM `%files`, etc.).
- **`detect_terminal()` / `detect_editor()` (2026-08-24)**, dans `examples/system-tools` : auto-détection du terminal et de l'éditeur graphiques installés (`x-terminal-emulator` en premier, puis xfce4-terminal, gnome-terminal, konsole, mate-terminal, lxterminal, xterm ; mousepad, gedit, kate, pluma, gnome-text-editor, xed, leafpad). La détection existait déjà en boucle en ligne ; cette entrée la nommait par des fonctions qui n'existaient pas — elles existent maintenant, et rendent le premier outil trouvé.
- Suite XML étendue à **55 cas de test** (ancrage Wayland, étiquettes et valeurs par défaut vides, searchentry, levelbar, drawingarea, colorbutton, fontbutton, aspectframe, tree, table, menubar, statusbar, togglebutton, timer, edit, list, separators, infobar types, notebook 3 pages, actions REFRESH/ENABLE/DISABLE/SHOW/HIDE/CLEAR, formulaire complexe)
- `AUTHORS` et `NEWS` à la racine (standard GNU)

### Modifié
- **Toute la documentation passe en GPL-2.0-or-later (2026-08-25)** : il y avait **trois papiers pour les mêmes fichiers** — `LICENCES.md` annonçait CC-BY-SA 4.0, les deux manuels texinfo portaient une **GFDL** dans leur propre `@copying`, et `debian/copyright` disait GPL. Et le texte de la GFDL n'était nulle part dans le dépôt, alors qu'elle demande de l'y joindre. Un seul texte couvre maintenant l'ensemble, celui du programme.
  La relicence est légitime, et c'est **mesuré** et non supposé : les manuels texinfo ont été écrits par le projet et ne partagent **aucune phrase** avec le manuel d'origine de László Pere, qui était sous GFDL. Zéro séquence de six mots en commun côté `gtk3sermo` ; côté `gtk4sermo`, les 31 séquences communes venaient **toutes** de la section « Remerciements — Assistance IA » que le projet avait lui-même ajoutée à ce manuel-là — le texte allait donc du projet vers lui, pas l'inverse.
  L'attribution est corrigée du même coup : les manuels ne portent plus le copyright de Pere et Thunor, qui ont écrit le **programme** et non ces pages. Le programme, lui, reste GPL-2.0-or-later, en héritage direct de leur travail.
- **La marque « Haplo-Linux » quitte le dépôt public (2026-08-25)** : 310 occurrences dans 262 fichiers. Le remplacement n'a pas été aveugle, parce qu'il ne pouvait pas l'être — le mot avait deux sens. Là où il nommait l'**auteur** ou le détenteur du copyright, il devient `haplo-dialog`. Là où il nommait un **système d'exploitation** (« installé sur », « distribution cible », « disponible sur », « Debian / Ubuntu / … »), il devient `Debian Testing` ou disparaît : écrire qu'un programme de dialogues est une distribution aurait produit des phrases fausses. L'opération s'est faite au niveau **octet** et non caractère, parce qu'une bonne partie des sources est en ISO-8859-1 : les décoder en UTF-8 les aurait cassées, et `grep` en mode texte les saute en les prenant pour du binaire — c'est ce piège qui avait fait sous-estimer le compte à 251.
- **Résidus de la même marque, hors de portée d'un remplacement littéral (2026-08-25)** : « Paquet de la distribution *haplo-histrio* » dans les `lintian-overrides` du port GTK 4 — alors que le port GTK 3 disait déjà « Paquet tiers » ; « Haplo ships XFCE » dans les deux `widget_window.c` ; « Release Haplo » et « dépôt Haplo » dans une feuille de route ; le thème « Haplo-Dark » ; l'overlay Gentoo `/var/db/repos/haplo/` (devenu `local/`, l'overlay standard) ; l'étiquette Slackware `_haplo` (devenue `_sermo`).
- **Les cahiers des charges publiaient une empreinte de la machine de construction (2026-08-25)** : version exacte du noyau (`Linux 7.0.4+deb14-amd64`) et bureau (`Xfce4 4.20 / Xfwm4`). Remplacés par les prérequis réels du port, qui est ce que le lecteur cherchait.
- `LOGO_TMP` utilise un nom fixe par UID (`/tmp/haplo-logo-UID.png`), évite la fuite en cas de `SIGKILL`
- **`ALLOWED_CMDS` élargi (2026-08-24)**, dans `examples/system-tools` : `dnf`, `zypper`, `pacman`, `emerge`, `slackpkg`, `xbps-install`, `xbps-query`, `apk` s'ajoutent à la famille `apt`, pour que l'aide en ligne de l'exemple réponde ailleurs que sur Debian. La liste continue de refuser tout le reste (`rm` reste refusé, vérifié).
- Licence **uniformisée à GPL-2.0-or-later** sur tout le dépôt (en-têtes source, packaging, `LICENCES.md`, `CONTRIBUTING.md`), l'essai GPL-3.0+ a été annulé, conformément à la clause « either version 2 … any later version » des sources et à l'amont gtkdialog

### Sécurité
- **La sortie rendue au shell pouvait être exécutée par `eval` (2026-08-25)** : le programme écrit des lignes `NOM="valeur"` et `EXIT="valeur"`, et toute la documentation du projet — l'exemple vitrine du README, les deux pages de manuel, six exemples du manuel utilisateur — invitait à les passer à `eval`. Or, dans un contexte entre guillemets doubles, le shell POSIX développe **quatre** caractères : `\`, `"`, `$` et l'accent grave. `shell_escape_value()` n'en échappait que deux, tout en affirmant dans son propre commentaire que sa sortie était *« safely eval'd »*. Et `gtk4sermo` n'avait pas la fonction du tout — il n'échappait rien.
  Ce qui rend le défaut réel, c'est **d'où vient la valeur**. Ce n'est pas l'auteur du script XML — lui a déjà le droit de lancer des commandes, c'est le modèle de confiance assumé. C'est la personne qui **se sert** du dialogue, et qui n'est pas forcément la même. Mesuré avant correctif, sans jamais toucher au script : ouvrir une fenêtre avec un simple champ, **taper au clavier** `$(touch /tmp/preuve)`, cliquer, puis évaluer la ligne rendue — le fichier était créé. Identique dans les deux ports.
  Les quatre caractères sont désormais échappés, aux **deux** points d'émission (`variables.c` pour les variables, `actions.c` pour `EXIT`) et dans **les deux** ports. La valeur n'est pas abîmée pour autant : « Ada Lovelace » traverse intacte, et la charge utile ressort comme du texte littéral.
  Verrouillé par `tests/garde_echappement_sortie.sh`, branché dans les trois pipelines. Ce banc ouvre une **vraie fenêtre**, clique, et passe la ligne réellement produite à `eval` sous surveillance d'un fichier témoin — car `--print-ir` n'imprime pas ces lignes. Une première version se rabattait sur une chaîne fabriquée quand le binaire ne rendait rien : elle se validait donc sur son propre bruit. Le repli a été supprimé, et un **témoin de sensibilité** vérifie qu'une ligne non échappée déclenche bien la détection. Éprouvé par deux sabotages : revenir aux deux caractères d'origine (les quatre cas échouent), et retirer l'appel du seul chemin `EXIT` (seul ce cas échoue).
  **La voie sûre reste `--do`** : les valeurs arrivent par l'environnement et ne sont jamais relues comme du code. Vérifié en rejouant la même charge utile : elle n'est pas exécutée. `README`, `SECURITY.md` et les pages de manuel le disent désormais, et le premier exemple du manuel utilisateur est passé sur `--do`.

### Supprimé
- **Le manuel amont orphelin (2026-08-25)** : `doc/gtkdialog.texi`, sa forme engendrée `doc/gtkdialog.info`, et deux artefacts que le port GTK 3 n'avait pas — `gtk4sermo.info` et `stamp-vti`. Les dossiers `doc/` des deux ports ont désormais la même forme. Le port GTK 3 avait déjà déprécié ce fichier au patch 13 ; seul le port GTK 4 en avait gardé une copie. Les dix faits qui valaient d'être repris en avaient été extraits juste avant. `make dist` et le build passent, et l'archive produite ne le contient plus.

### Corrigé
- **La montée de version avait laissé sept recettes de paquet en 1.0.0 (2026-08-26)** : renommer les dossiers et bumper `AC_INIT` ne suffisait pas. Un audit contradictoire du diff — cinq lectures indépendantes, puis un réfutateur par constat chargé de les démolir — en a confirmé douze, dont neuf qui **cassent** vraiment. Les deux ebuilds Gentoo étaient l'exemple parfait du défaut invisible : renommés en `-1.1.0.ebuild`, donc `PV=1.1.0`, ils téléchargeaient toujours l'archive du tag `v1.0.0` tout en cherchant `${WORKDIR}/sermo-v1.1.0/<port>/<port>_1.1.0` — un chemin que cette archive ne contient pas. `emerge` aurait échoué au dépaquetage. Vérifié en déballant pour de vrai l'archive publiée : sa racine est bien `sermo-v<étiquette>/`, et son dossier de port `<port>_1.0.0`. Les deux `SRC_URI` sont maintenant écrits en `${PV}` : le prochain `git mv` suffira. Même traitement pour les deux `Manifest`, les deux `README.gentoo.md` et les deux `README.slackware.md`, qui citaient des fichiers supprimés.
  Le plus instructif est ailleurs : j'avais bien monté les deux `.spec` — sauf que ce sont des fichiers **produits**, ignorés par git. La vraie source, `<port>.spec.in`, était restée en `%define version 1.0.0`, et `configure` réécrivait le `.spec` en 1.0.0 à chaque passage. Le modèle dit désormais `%define version @VERSION@`, que `configure` remplit depuis `AC_INIT` : il suit tout seul, et ce piège-là est fermé pour de bon. La check-list de release de `VERSIONING.md`, qui avait laissé passer ces sept emplacements, en compte maintenant six de plus.
- **Le port GTK 4 n'avait presque pas de `.gitignore` (2026-08-26)** : 3 lignes contre 60 pour le port GTK 3. Son binaire compilé (1,5 Mo), son manuel `.info` engendré et ses fichiers objets n'étaient protégés par rien — un `git add -A` les aurait emportés. Le fichier du port GTK 3 lui a été transposé. Au passage, un motif qui ne mordait dans aucun des deux : `debian/` est un **lien** vers `packaging/debian/`, et git n'applique les règles qu'au chemin réel — les dossiers d'assemblage de debhelper restaient donc visibles. Les deux chemins sont désormais couverts, et l'arbre revient propre après une construction complète.
- **`ci/build.sh` ne connaissait pas le port GTK 4 (2026-08-26)** : `VERSIONING.md` prescrit `./ci/build.sh gtk4sermo --test` en première étape de publication, et la commande répondait « Argument inconnu » — puis construisait `gtk3sermo` en silence, parce qu'un argument inconnu n'était qu'un avertissement. Les deux branches manquantes sont posées, `all` construit désormais les deux ports, et un argument inconnu **arrête** le script au lieu de faire autre chose sans le dire. Le rapport de tests, du même coup, ne ment plus : `make check` traverse les sous-répertoires et rend 0 même sans une seule épreuve, si bien que le script annonçait « Tests gtk4sermo » pour zéro test exécuté. Il compte maintenant les épreuves et dit explicitement quand un port n'en a aucune de câblée — c'est le cas du port GTK 4, dont les bancs vivent à la racine.
- **Un fichier engendré était suivi par git (2026-08-26)** : `gtk4sermo/gtkdialog.spec` est produit par `configure` depuis `gtkdialog.spec.in`, et se trouvait pourtant commité — le port GTK 3 ne suit que son modèle, lui. Retiré et ignoré, les deux ports ont maintenant la même forme. Son modèle nommait encore le paquet **`gtk3d`**, pointait une URL FTP de 2004 et donnait l'amont comme empaqueteur ; la vraie recette RPM et le `slack-desc` décrivaient eux aussi un « gtk3d » qui n'existe nulle part. Corrigés, en respectant les longueurs de ligne calibrées du `slack-desc`.
- **Une ligne de changelog dépassait la limite Debian (2026-08-26)** : `lintian` la signalait (`debian-changelog-line-too-long`). Repliée. Les seules remarques restantes sont le correcteur orthographique anglais sur du texte français.
- **La page de manuel XML disait la racine `<window>` obligatoire (2026-08-25)** : « Chaque dialogue doit avoir un unique élément racine `<window>` ». Faux, et éprouvé faux dans les deux ports avec contre-cas : une description sans racine — un `<vbox>` seul, ou même un widget nu — est enveloppée d'office par la grammaire (règle de départ de `gtkdialog_parser.y`), et une vraie fenêtre s'ouvre. C'est le motif de la plupart des dialogues écrits pour l'ancêtre gtkdialog, donc de la compatibilité que le projet revendique. Corrigé en disant aussi à quoi sert quand même un `<window>` explicite : le titre, la taille, les décorations, l'ancrage Wayland — et le fait qu'une fenêtre ne porte **qu'un seul enfant**, les widgets suivants étant perdus sans avertissement.
- **`--include` était décrit à l'envers dans les trois pages de manuel (2026-08-25)** : « Include and execute FILE **before running** the main GUI description ». En réalité le fichier n'est pas exécuté une fois au démarrage — chaque commande d'action est transformée en `. FICHIER ; commande`, donc le fichier est relu dans le sous-shell de **chaque** action. Avec une exception mesurée que personne n'aurait devinée : `--do` n'est **pas** préfixé, et ne voit donc pas le fichier inclus.
- **L'exemple `examples/miscellaneous/standalone_file` ne pouvait pas s'exécuter (2026-08-25)** : il portait encore le shebang d'amont `#! /usr/local/bin/gtkdialog`, un chemin qu'aucun paquet ne pose (`gtk3sermo.install` pose `usr/bin/gtk3sermo`, `gtksermo.install` pose `usr/bin/gtkdialog`). Lancé directement, il échouait sur « mauvais interpréteur ». Corrigé par port, et le mode shebang est désormais documenté — avec ses limites, rejouées.
- **Cliquer un `<switch>` tuait le programme (2026-08-25)** : SIGSEGV au premier clic, mesuré **6 fois sur 6**, dans les deux ports. La pile désigne notre code — `attributeset_get_first()` appelé depuis `widget_signal_executor()` sur le signal « changed ». La cause : `GObject::notify` passe **trois** arguments — `(objet, pspec, data)` — et `on_any_widget_changed_event()` n'en attend que deux, `(widget, Attr)`. Le `GParamSpec` arrivait donc à la place du jeu d'attributs, et était déréférencé. C'est **exactement** la faute de `GtkListBox::row-selected` corrigée trois jours plus tôt : un signal dont l'arité ne correspond pas au rappel branché. Adaptateur de signature posé, sur le même patron.
  Ce que ce défaut apprend surtout, c'est que `tests/run_examples.sh` **ouvre** les fenêtres mais ne **clique** jamais : les 55 et 58 exemples étaient comptés OK et le pipeline restait vert pendant que l'interrupteur tuait le programme. D'où `tests/garde_clic_widgets.sh` : un dialogue minimal par widget interactif, deux clics, et le processus doit survivre. 17 widgets, les deux ports, dans les trois pipelines. Un widget qui n'ouvre aucune fenêtre y est un échec, pas un cas ignoré. Balayage complet des 17 : `<switch>` était le seul à tomber. Éprouvé par sabotage — rebrancher le rappel en direct fait ressortir le `MORT rc=139`.
- **L'action `CLOSE:` n'existe pas (2026-08-25)** : les deux `MANUEL_UTILISATEUR.md` la documentaient dans un tableau de référence **et** dans un exemple complet à recopier. Le préfixe réel est `closewindow:`. Le point qui rend l'erreur silencieuse : un préfixe inconnu n'est pas signalé, il retombe en commande shell ordinaire — `CLOSE:MAFENETRE` ne fermait donc rien, et personne ne voyait d'erreur. Vérifié par un harnais compilé sur `stringman.c` : `command_get_prefix("CLOSE:essai")` rend `prefixe=command, commande=CLOSE:essai`, quand `closewindow:essai` rend `prefixe=closewindow, commande=essai`. Trouvé en dépouillant le manuel amont orphelin, seul document du dépôt à donner l'orthographe qui marche.
- **`packaging/arch/.SRCINFO` avait divergé de son `PKGBUILD` (2026-08-25)** : le correctif du 2026-08-24 sur les quatre recettes non-Debian n'avait touché que le `PKGBUILD`. Le `.SRCINFO`, que les outils Arch lisent en premier, pointait toujours une URL qui n'existe pas (`https://haplo-dialog.fr/pub/…`) et déclarait moins de dépendances (`gtk-layer-shell` manquant côté GTK 3). Les deux fichiers sont régénérés depuis le `PKGBUILD` et contrôlés champ par champ ; l'URL répond **HTTP 200**.
- **Le manuel info du port GTK 4 était un talon (2026-08-25)** : `doc/gtk4sermo.info` faisait 442 octets et annonçait « produced by makeinfo » sans l'avoir été. Régénéré pour de bon : 8 372 octets. `doc/gtkdialog.info` régénéré aussi, ses 17 renvois de table vérifiés un à un.
- **Les deux ports plantaient dès qu'on s'en servait (2026-08-22)** : une fenêtre contenant une `<list>` mourait à l'ouverture (segfault) — le rappel de `row-selected` n'attendait que deux paramètres sur les trois que GTK passe ; et tout nombre à virgule tuait le programme en locale française (abandon) — `atof()` lit `0.5` comme `0` sous `fr_FR`. 52 appels à `atof()` remplacés par `g_ascii_strtod`. La passe était incomplète : voir l'entrée du 2026-08-25 ci-dessous. Mesuré en locale `fr_FR` : les exemples passent de 26/54 à 55/55 pour gtk3sermo, et 58/58 pour gtk4sermo.
- **La barre de progression faisait tourner GTK depuis son thread de lecture (2026-08-23)** : `gtk_main_iteration_do()` appelé hors du thread principal, sous un `gdk_threads_enter()` qui ne protège plus rien depuis GTK 3.6. Segfault ou abandon environ une fois sur dix. Le thread ne fait plus que lire son tuyau ; tout ce qui touche GTK repart vers la boucle principale par `g_idle_add()`.
- **`make check` ne compilait pas sous `--enable-unit-tests` (2026-08-25)** : l'option est documentée dans `COMPILE.md` et dans la check-list de release, et elle échouait à l'édition de liens avec une trentaine de « undefined reference ». `test_stringman.c` était écrit contre une vingtaine de fonctions `command_is_*()` / `command_get_*()` qui n'existent dans aucun des deux binaires : leurs définitions sont enfermées dans un bloc `/* Redundant: Not being used */` de `stringman.c`, hérité de l'amont gtkdialog, ouvert ligne 76 et refermé ligne 303. Le fichier de test l'avait même écrit noir sur blanc — « ces fonctions existent dans stringman.c » — sans que ce soit vrai. Il porte désormais sur l'API qui les a remplacées (la table de préfixes, lue par `command_get_prefix()` et `command_prefix_get_type()`), plus `strnatcmp`, `input_is_shell_command`, `linecutter` et `str_default_name` : 15 vérifications, dont l'alignement de la table sur l'énumération `CommandType` et le cas d'une commande shell contenant un deux-points. Éprouvé par deux sabotages, tous deux attrapés et nommés. **La CI rejoue maintenant `make check` à chaque poussée** : son absence est précisément ce qui a laissé la casse passer inaperçue.
- **`linecutter()` mélangeait deux allocateurs (2026-08-25)** : les champs après le premier étaient alloués par `strdup()` (malloc) et libérés par `g_free()`. Sans conséquence visible sur glibc, mais formellement indéfini. Passés à `g_strdup()`. Au passage, le commentaire d'origine affirmait que la fonction « ne détruit pas la chaîne d'origine, elle en fait des copies » : c'est faux deux fois — elle écrit des `\0` dedans, et `line[0]` **est** la chaîne d'origine, que `list_t_free()` libère ensuite. Le contrat d'appartenance réel est maintenant écrit au-dessus de la fonction ; s'y tromper, c'est passer un littéral et faire libérer une adresse que GLib n'a jamais allouée.
- **Trois `strtod()` avaient survécu à la passe sur les locales (2026-08-25)** : la campagne du 2026-08-22 avait bien retiré les 52 `atof()`, mais laissé trois `strtod()` par port, qui souffrent exactement du même mal — c'est la locale qui décide où se trouve la virgule. Mesuré sous `fr_FR.UTF-8` : `strtod("0.9")` rend **0,000**, quand `g_ascii_strtod("0.9")` rend **0,900**. Les trois sites : un attribut XML flottant dans `tag_attributes.c` (par exemple `xalign="0.9"`), et deux colonnes numériques de `<tree>` dans `widget_tree.c`. Autrement dit, un nombre écrit avec un point dans un fichier XML valait zéro **en silence** sur toute machine française. Corrigés dans les deux ports, et désormais interdits par un banc.
- **Sûreté mémoire du port GTK 4 (2026-08-24)** : `strncpy` dans un tampon non mis à zéro (un nom de variable de 512 caractères ou plus n'était pas terminé), recopie des widgets d'un conteneur sans borne `MAXWIDGETS` (300 enfants directs étaient acceptés en silence), et `action_append()` qui recopiait son premier paramètre depuis le début de la chaîne. Le port GTK 3 avait déjà les trois garde-fous ; les deux ports sont alignés.
- **Filtrage de l'environnement enfant, port GTK 4 (2026-08-24)** : `_build_child_env()` manquait entièrement — 60 lignes d'écart avec le port GTK 3 — alors que `SECURITY.md` annonçait le filtrage pour le produit.
- **Élément à contenu textuel vide (2026-08-20)** : `<label></label>`, ou une étiquette faite uniquement d'espaces, cassait l'analyse avec un « syntax error » pointant la balise fermante, alors que poser un espaceur en écrivant `<text><label>   </label></text>` est le réflexe naturel. `<label>` et `<default>` acceptent désormais le vide et valent `""` (comme `<item>` le faisait déjà) ; `<sensitive>`, `<width>`, `<height>`, `<input>`, `<output>`, `<variable>` et `<action>` restent une erreur, mais avec un message qui nomme la cause : « the <variable> element is empty; it requires a name. »
- **Troncature des noms de widgets auto-générés (2026-06-06)** : `g_snprintf(name, sizeof(name), …)` où `name` est un `char*`, `sizeof` valait donc 8 octets, tronquant les noms, corrigé en passant la taille du tampon à 64 ; restaure le comportement gtkdialog d'origine
- **Lecture de variable non initialisée (2026-06-06)** : `instruction inst;` dont `inst.ival` était lu → `instruction inst = {0};`
- **Compilation propre (2026-06-06)** : 0 erreur ; les warnings résiduels relèvent d'idiomes amont gtkdialog et de 2 conflits shift/reduce bison intouchables

---

## Révisions d'empaquetage de la 1.0.0

Le **logiciel** reste en version 1.0.0. Les révisions -2 et -3 ne changent que le
paquet Debian et livrent un binaire identique ; à partir de la -4 elles changent
aussi le binaire.

Les deux ports ont chacun son `debian/changelog`, qui fait foi pour lui, et leurs
révisions peuvent diverger : un correctif ne touche pas toujours les deux. État au
2026-08-24 : **gtk3sermo 1.0.0-10**, **gtk4sermo 1.0.0-11**.

### [1.0.0-11] - 2026-08-22 — gtk4sermo seulement
- Page de manuel : la section SEE ALSO renvoyait vers `fltk1d`, `efl1d`, `qt6d` et
  d'autres ports jamais publiés.

### [1.0.0-10] - 2026-08-22 — les deux ports
- Menus GTK 4 : `<menuitem>` rendait un `GtkPopoverMenu` autonome, empilé dans la
  fenêtre au lieu d'être attaché à sa barre.

### [1.0.0-9] - 2026-08-21 — les deux ports
- **Renommage.** Le paquet s'appelait `gtk3dialog`, nom déjà porté par un paquet de
  BunsenLabs, puis `haplo-dialog` — qui est le nom du produit, pas celui d'un port.
  Les ports s'appellent désormais `gtk3sermo` et `gtk4sermo`, et le lien de
  compatibilité `gtkdialog` vit dans un paquet séparé, `gtksermo`, en conflit
  assumé. Les deux ports s'installent donc à côté de n'importe quelle autre
  implémentation.

### [1.0.0-5] - 2026-08-20
- **Analyseur** : une étiquette vide est acceptée là où le vide est une valeur
  (`<label>`, `<default>`, `<item>` sous sa forme attribuée) ; ailleurs, l'erreur
  nomme enfin sa cause au lieu de désigner la balise fermante. Voir la section
  1.1.0 ci-dessus.
- **Ancrage Wayland éprouvé** sous sway 1.12 (wlroots 0.20), sortie headless
  1280x720, mesuré au pixel : barre `topstride` pleine largeur, dock `bottom` à
  exactement `dist` px du bord, bande `background` intégralement recouverte par
  une fenêtre ordinaire, `dist="0"` et `dist="60"` séparés de 60 px. Le paquet
  -4 est antérieur à cette vérification. Non couvert : le multi-écran, le
  matériel réel, et les compositeurs hors wlroots comme Hyprland.
- Une barre ne réserve **pas** d'espace : la zone exclusive reste à zéro, donc
  une surface ancrée flotte au-dessus des fenêtres ordinaires au lieu de les
  repousser.
- Comme la -4, cette révision change le binaire.

### [1.0.0-4] - 2026-08-20
- Premier paquet embarquant l'**ancrage Wayland** (`layer`/`edge`/`dist` sur
  `<window>`, voir la section 1.1.0 ci-dessus). Contrairement aux deux révisions
  précédentes, le binaire n'est **pas** identique au précédent.
- Nouvelle dépendance optionnelle `libgtk-layer-shell0` (build :
  `libgtk-layer-shell-dev ≥ 0.8.0`).
- Construit avant que l'ancrage soit éprouvé sur un compositeur réel ; c'est la
  -5 qui apporte cette vérification.

### [1.0.0-3] - 2026-08-16
- Identité du mainteneur du paquet : le champ `Maintainer` portait un nom
  personnel et un domaine privé, visibles par `apt show gtk3sermo` chez chaque
  utilisateur. Il porte désormais l'identité du projet.
- `Vcs-Git` et `Vcs-Browser` pointent sur le dépôt réel ; ils désignaient
  auparavant une forge qui n'a jamais existé.
- Aucun changement fonctionnel.

### [1.0.0-2] - 2026-08-11
- Première construction propre du paquet, après la passe de sécurité
  d'avant-publication (voir la 1.0.0 ci-dessous) : `lintian` sans signalement,
  durcissement PIE / RELRO / stack-protector / FORTIFY vérifié.

---

## [1.0.0] - 2026-05-29

Première version publique stable, refonte complète de gtkdialog 0.8.3.

### Ajouté
- **gtk3sermo** : port de référence (backend GTK 3), fournissant l'alias rétro-compatible `gtkdialog`
- **43 widgets** implémentés
- Nouveaux widgets : `<switch>`, `<password>`, `<searchentry>`, `<calendar>`, `<infobar>`, `<levelbar>`, `<spinner>`, `<aspectframe>`, `<drawingarea>`
- `safe_system()` / `safe_popen()`, remplacement sécurisé de `system()` / `popen()`
- Durcissement : `FORTIFY_SOURCE=3`, PIE, Full RELRO, NX stack, stack canary (`-fstack-protector-strong`), CFI (`-fcf-protection`)
- Manpage roff `gtk3sermo(1)`
- `haplo-dialog-xml(5)`, manpage de référence de la syntaxe XML
- Documentation Texinfo (`.texi` → `.info`)
- Site web de documentation (HTML statique)
- Recettes d'empaquetage : `.deb` (Debian, seule construite par le projet), `.rpm` (Fedora/SUSE), `PKGBUILD` (Arch), `.ebuild` (Gentoo), `.SlackBuild` (Slackware)
- Scripts d'exemple couvrant tous les widgets (`examples/`, 53 répertoires)
- Suite de régression XML (`tests/xml/`)
- `SECURITY.md`, `.clang-format`, `.editorconfig`
- `CONTRIBUTING.md`, `CODE_OF_CONDUCT.md`, `ROADMAP.md`

### Modifié
- Renommage du binaire : `gtkdialog` → `gtk3sermo`
- `gtkdialog_parser.y` / `gtkdialog_lexer.l`, conservés
- Élévation via `pkexec` (PolicyKit), `sudo` GUI supprimé
- `fclose()` sur tout `FILE*` issu de `safe_popen()`, `pclose()` banni

### Supprimé

- Dépendances GTK2 résiduelles
- `system()` et `popen()` directs dans les widgets
- Code mort et commentaires parasites

### Sécurité
- CVE-like : aucune vulnérabilité connue au 2026-05-29
- Durcissement `safe_exec.c`, exécution sans shell quand possible, repli `/bin/sh -c` journalisé et désactivable (`HAPLO_NO_SHELL_FALLBACK`)
- Variables exportées par l'environnement, jamais évaluées par l'outil

---

## [0.9.0-haplo1] - 2026-03-01 (interne)

Version de travail initiale, portage depuis gtkdialog 0.8.3.

### Ajouté
- Structure initiale du port gtk3sermo
- Migration du core (parser XML, variables, actions, signaux, pile)
- Implémentation initiale des widgets
- Système de build : autotools

---

## [gtkdialog-0.8.3] - 2013-xx-xx (upstream original)

Dernière version de l'upstream original par László Pere et Thunor.  
Référence : https://github.com/01micko/gtkdialog

---

*haplo-dialog est un fork de gtkdialog 0.8.3 (GPL-2.0+),  
modernisé et étendu par le projet haplo-dialog (GPL-2.0-or-later).*

---

