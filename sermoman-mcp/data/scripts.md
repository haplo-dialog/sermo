# Scripts prêts à l'emploi : scripts métier et boîtes à outils

Inventaire écrit par `sermoman-mcp/outils/inventaire-scripts.py` à partir de
l'en-tête de chaque script : ne pas le modifier à la main.

Ce sont des exemples complets (domaine public, CC0-1.0) ET le test de réussite
des neuf ports : `examples/metiers/verifie.sh` joue chacun de leurs dialogues
sur chaque port et compare les valeurs à gtk3sermo. Règles d'écriture :
`examples/metiers/CONVENTION.md`. Installés par le paquet `sermo-doc` dans
`/usr/share/doc/sermo-doc/examples/`. Leurs dialogues sont servis par
sermoia-mcp (`sermo_examples`, sujets `metier-…` et `boite-…`).

Lancer sans risque : `<script> gtk3sermo` (le 1er argument est le port). Par
défaut rien n'est modifié : la commande est affichée (`à blanc : …`) ;
`SERMO_A_BLANC=0` pour agir. Aucun script n'appelle `sudo` ni `pkexec`.

## Scripts métier (examples/metiers)

### Administrateur système (`01-admin-systeme`)

- **`examples/metiers/01-admin-systeme/1-sauvegarde.sh`** — sauvegarder un dossier vers un disque, en voyant d'abord ce qui partira
  - enrobe : rsync
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/01-admin-systeme/2-comptes.sh`** — créer un compte utilisateur (identifiant, groupes, shell, mot de passe initial)
  - enrobe : useradd, chpasswd, passwd --expire
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/01-admin-systeme/3-services.sh`** — voir l'état d'un service systemd, le démarrer, l'arrêter ou le redémarrer
  - enrobe : systemctl
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/01-admin-systeme/4-journaux.sh`** — lire le journal système filtré par service, gravité et période
  - enrobe : journalctl
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu (le calendrier devient une saisie de date)

- **`examples/metiers/01-admin-systeme/5-espace-disque.sh`** — voir où part l'espace disque et nettoyer les caches, avec confirmation
  - enrobe : df, du, apt-get clean, journalctl --vacuum-time
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu


### Développeur / DevOps (`02-developpeur-devops`)

- **`examples/metiers/02-developpeur-devops/1-compiler.sh`** — compiler un projet : cible, nombre de tâches, mode, nettoyage éventuel
  - enrobe : make, cmake --build
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/02-developpeur-devops/2-git.sh`** — changer de branche ou valider (commit) les modifications d'un dépôt git
  - enrobe : git switch, git commit
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/02-developpeur-devops/3-deployer.sh`** — déployer une version sur un serveur, avec le nom de l'environnement à taper
  - enrobe : rsync, ssh (ln -sfn, systemctl reload-or-restart)
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/02-developpeur-devops/4-conteneurs.sh`** — lister les conteneurs, lire leurs journaux, les démarrer, arrêter ou redémarrer
  - enrobe : podman (sinon docker) ps, logs, start, stop, restart
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/02-developpeur-devops/5-nouveau-projet.sh`** — créer l'arborescence d'un nouveau projet (langage, licence, dépôt git)
  - enrobe : mkdir, install, cp, git init
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu


### Support informatique (`03-support-informatique`)

- **`examples/metiers/03-support-informatique/1-releve-poste.sh`** — relever l'état d'un poste (système, matériel, mémoire, disques, réseau) en texte à copier
  - enrobe : uname, lscpu, free, df, lsblk, ip, dpkg-query
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/03-support-informatique/2-ticket.sh`** — saisir un ticket d'incident (catégorie, urgence, description) et l'enregistrer ou l'envoyer
  - enrobe : install (fichier texte), xdg-email
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : xdg-email ouvre un logiciel de courriel graphique : choisir le fichier en SSH

- **`examples/metiers/03-support-informatique/3-reseau.sh`** — voir les connexions réseau d'un poste, en afficher le détail, les activer ou les relancer
  - enrobe : nmcli
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu (attention : relancer la connexion qui porte la session SSH la coupe)

- **`examples/metiers/03-support-informatique/4-imprimantes.sh`** — voir les files d'impression, annuler un travail bloqué, imprimer une page de test
  - enrobe : lpstat, cancel, lp
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/03-support-informatique/5-aide-a-distance.sh`** — ouvrir une aide à distance : choix de l'outil, consignes à lire, accord de la personne
  - enrobe : rustdesk, x11vnc (par tunnel SSH), tmate, timeout
  - ports : gtk3sermo conseillé (la personne aidée est devant l'écran) ; terminal : tmate seul a du sens par SSH ; RustDesk et x11vnc partagent l'écran graphique


### Intégrateur de distribution (`04-distribution-integrateur`)

- **`examples/metiers/04-distribution-integrateur/1-premier-demarrage.sh`** — au premier démarrage, régler la langue, le clavier et le fuseau horaire
  - enrobe : localectl, timedatectl
  - ports : gtk3sermo conseillé ; ncursessermo sur une console sans bureau ; terminal : rien de perdu

- **`examples/metiers/04-distribution-integrateur/2-choix-logiciels.sh`** — choisir un profil de poste et des logiciels, voir ce qu'apt installera, puis installer
  - enrobe : apt-get (simulation -s, puis install)
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/04-distribution-integrateur/3-utilisateur-initial.sh`** — créer le premier compte du poste (nom, identifiant, mot de passe saisi deux fois, administrateur ou non)
  - enrobe : useradd, chpasswd
  - ports : gtk3sermo conseillé ; ncursessermo sur une console sans bureau ; terminal : rien de perdu (le mot de passe reste masqué)

- **`examples/metiers/04-distribution-integrateur/4-apparence.sh`** — régler l'apparence du bureau : thème, icônes, police, taille et échelle des textes
  - enrobe : xfconf-query (XFCE), sinon gsettings
  - ports : gtk3sermo conseillé ; terminal : sans objet en ncurses (réglage d'une session graphique)

- **`examples/metiers/04-distribution-integrateur/5-mises-a-jour.sh`** — régler la fréquence des mises à jour et leur installation automatique, ou voir celles en attente
  - enrobe : unattended-upgrades (fichiers de /etc/apt/apt.conf.d), apt list, apt-get
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu


### Bornes et écrans d'accueil (`05-bornes-ecrans`)

- **`examples/metiers/05-bornes-ecrans/1-accueil-visiteurs.sh`** — tenir le registre des visiteurs d'un site (arrivée, départ) sur une borne
  - enrobe : tee -a (registre CSV, séparateur « ; »)
  - ports : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/05-bornes-ecrans/2-rendez-vous.sh`** — prendre un rendez-vous à la borne : service, jour, créneau libre, coordonnées
  - enrobe : tee -a (agenda CSV, séparateur « ; »), date
  - ports : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/05-bornes-ecrans/3-avis-satisfaction.sh`** — recueillir l'avis des visiteurs (note de 1 à 5 et commentaire) sur une borne
  - enrobe : tee -a (avis en CSV, séparateur « ; »)
  - ports : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/05-bornes-ecrans/4-information.sh`** — écran d'information en onglets (horaires, plan, contacts, actualités), avec impression du plan
  - enrobe : lp (impression du plan) ; textes lus dans des fichiers simples
  - ports : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/05-bornes-ecrans/5-inscription-evenement.sh`** — inscrire le public à un atelier ou un événement, avec consentement explicite (RGPD)
  - enrobe : tee -a (inscriptions en CSV, séparateur « ; »)
  - ports : lvglsermo conseillé (écran tactile) ; gtk3sermo sur un poste ; terminal : rien de perdu


### Atelier, industrie, laboratoire (`06-atelier-industrie-labo`)

- **`examples/metiers/06-atelier-industrie-labo/1-saisie-mesures.sh`** — saisir les mesures d'une pièce (opérateur, lot, poste), les comparer aux tolérances, les garder en CSV
  - enrobe : awk (contrôle des tolérances), tee -a (fichier de mesures CSV)
  - ports : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier ; ncursessermo sur un terminal de poste ; terminal : rien de perdu

- **`examples/metiers/06-atelier-industrie-labo/2-controle-qualite.sh`** — contrôle qualité d'un ordre de fabrication : chaque point conforme ou non, remarque, rapport daté
  - enrobe : tee (rapport texte par OF, et ligne CSV de synthèse)
  - ports : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier ; terminal : rien de perdu

- **`examples/metiers/06-atelier-industrie-labo/3-etiquetage.sh`** — préparer et imprimer les étiquettes de colis d'un lot (référence, quantité, dates), aperçu d'abord
  - enrobe : lp (impression), lpstat (liste des imprimantes, en lecture)
  - ports : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier ; terminal : rien de perdu

- **`examples/metiers/06-atelier-industrie-labo/4-banc-essai.sh`** — régler et lancer un essai sur banc (profil, tension, température, durée), puis suivre son avancement
  - enrobe : le pilote du banc ($BANC_CMD, fourni avec le banc) ; tail -F pour suivre l'avancement
  - ports : gtk3sermo conseillé ; lvglsermo sur l'écran du banc ; terminal : rien de perdu (la barre de progression existe aussi en ncurses)

- **`examples/metiers/06-atelier-industrie-labo/5-capteurs.sh`** — relever les températures des capteurs de la machine, les comparer à un seuil, garder le relevé
  - enrobe : lecture de /sys/class/thermal (et /sys/class/hwmon à défaut) ; tee -a (relevés CSV)
  - ports : gtk3sermo conseillé ; lvglsermo sur un écran d'atelier ; terminal : rien de perdu (la jauge devient une barre de caractères)


### PME, association (trésorerie) (`07-pme-association`)

- **`examples/metiers/07-pme-association/1-note-de-frais.sh`** — saisir une dépense avancée (date, montant, catégorie) dans le registre CSV des notes de frais
  - enrobe : tee -a (registre CSV, séparateur « ; », ouvrable dans un tableur)
  - ports : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; ncursessermo par SSH ; terminal : rien de perdu (le calendrier devient une saisie de date)

- **`examples/metiers/07-pme-association/2-adherent.sh`** — inscrire un adhérent (identité, formule, règlement, consentements) dans le registre de la saison
  - enrobe : grep (doublon, lecture seule) ; tee -a (registre CSV)
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo pour une permanence sur tablette ; terminal : rien de perdu (les onglets deviennent des pages)

- **`examples/metiers/07-pme-association/3-contact.sh`** — saisir la fiche d'un contact et l'enregistrer en vCard, importable dans tout carnet d'adresses
  - enrobe : tee (fichier .vcf, vCard 3.0)
  - ports : gtk3sermo conseillé ; qt6sermo, efl1sermo ; terminal : rien de perdu

- **`examples/metiers/07-pme-association/4-facture.sh`** — établir une facture (client, lignes, TVA), la relire calculée, l'enregistrer en texte et l'imprimer
  - enrobe : awk (calcul) ; tee (fichier de facture) ; lp (impression)
  - ports : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; terminal : rien de perdu (le tableau d'aperçu défile en ncurses)

- **`examples/metiers/07-pme-association/5-reservation-salle.sh`** — réserver une salle (salle, date, créneau, effectif, matériel) en voyant les conflits au planning
  - enrobe : grep (conflits, lecture seule) ; tee -a (planning CSV)
  - ports : gtk3sermo conseillé ; lvglsermo sur un écran d'accueil ; htmlsermo sur l'intranet ; terminal : rien de perdu (le calendrier devient une saisie de date)


### Enseignement, formation (`08-enseignement-formation`)

- **`examples/metiers/08-enseignement-formation/1-qcm.sh`** — faire passer un QCM (une question par onglet), afficher le score et la correction, garder le résultat
  - enrobe : tee -a (résultats CSV) ; questions lues dans un fichier texte (QCM_FICHIER)
  - ports : gtk3sermo conseillé ; qt6sermo ; lvglsermo sur tablette ; terminal : rien de perdu (les onglets deviennent des pages, la barre un pourcentage)

- **`examples/metiers/08-enseignement-formation/2-lanceur-tp.sh`** — choisir un TP dans la liste, copier son dossier dans l'espace de l'élève et l'ouvrir
  - enrobe : cp -r --no-clobber ; xdg-open
  - ports : gtk3sermo conseillé ; qt6sermo, efl1sermo ; lvglsermo sur poste élève tactile ; terminal : xdg-open ouvre un gestionnaire de fichiers graphique : sans écran, seule la copie a un sens

- **`examples/metiers/08-enseignement-formation/3-fiche-eleve.sh`** — remplir la fiche de renseignements d'un élève (identité, responsables, scolarité) et la classer par classe
  - enrobe : tee (fiche texte, une par élève, rangée dans le dossier de la classe)
  - ports : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; terminal : rien de perdu (les onglets deviennent des pages, l'interrupteur une case)

- **`examples/metiers/08-enseignement-formation/4-minuteur-examen.sh`** — chronométrer une épreuve (durée, tiers-temps, alerte avant la fin) et noter début et fin au procès-verbal
  - enrobe : date (temps restant, lu par <timer>) ; tee -a (procès-verbal CSV)
  - ports : gtk3sermo conseillé (vidéoprojecteur) ; qt6sermo ; htmlsermo ne joue pas les minuteurs ; terminal : le temps restant se met à jour ; la barre devient un pourcentage

- **`examples/metiers/08-enseignement-formation/5-releve-notes.sh`** — saisir les notes d'un devoir, les ramener sur 20, voir moyenne, plus basse et plus haute, exporter le relevé
  - enrobe : awk (calculs) ; tee (relevé CSV pour le tableur ou le logiciel de vie scolaire)
  - ports : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; ncursessermo par SSH ; terminal : rien de perdu


### Multimédia (vidéo) (`09-multimedia`)

- **`examples/metiers/09-multimedia/1-convertir-video.sh`** — convertir une vidéo (format, définition, qualité, son) après avoir vu ce que contient la source
  - enrobe : ffprobe (lecture seule) ; ffmpeg
  - ports : gtk3sermo conseillé ; qt6sermo, efl1sermo ; ncursessermo par SSH sur un serveur d'encodage ; terminal : rien de perdu

- **`examples/metiers/09-multimedia/2-extraire-audio.sh`** — extraire la bande son d'une vidéo (format, débit, extrait, volume normalisé) pour un podcast ou une transcription
  - enrobe : ffmpeg
  - ports : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/09-multimedia/3-redimensionner-images.sh`** — réduire un dossier de photos pour le web ou l'envoi (taille, qualité, métadonnées), en copie ou sur place
  - enrobe : convert (copies, ImageMagick) ; mogrify (sur place, avec confirmation)
  - ports : gtk3sermo conseillé ; qt6sermo, sdl3sermo ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/09-multimedia/4-renommer-photos.sh`** — renommer les photos d'un dossier d'après leur date de prise de vue, en voyant d'abord les nouveaux noms
  - enrobe : exiftool (-testname pour l'aperçu, en lecture seule ; -FileName pour renommer)
  - ports : gtk3sermo conseillé ; qt6sermo, efl1sermo ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/09-multimedia/5-capture-ecran.sh`** — enregistrer l'écran ou une zone (images par seconde, son, souris, durée) dans un fichier vidéo
  - enrobe : ffmpeg -f x11grab (et -f pulse pour le son) ; xdpyinfo (taille de l'écran, lecture seule)
  - ports : gtk3sermo conseillé ; qt6sermo, fltk1sermo ; terminal : sans serveur X il n'y a rien à filmer : le script affiche la commande, à lancer sur le poste graphique


### Sécurité informatique (`10-securite-informatique`)

- **`examples/metiers/10-securite-informatique/1-perimetre-analyse.sh`** — préparer une analyse réseau (cible, type, ports, rythme) et relire la commande avant de la lancer
  - enrobe : nmap
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/10-securite-informatique/2-integrite.sh`** — relever l'empreinte SHA-256 des fichiers d'un dossier, puis vérifier plus tard que rien n'a changé
  - enrobe : sha256sum
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/10-securite-informatique/3-audit-comptes.sh`** — passer en revue les comptes qui peuvent ouvrir une session (dernière connexion, état du mot de passe) et verrouiller un compte dormant
  - enrobe : getent, lastlog (ou lastlog2), passwd -S en lecture ; passwd -l pour verrouiller
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/10-securite-informatique/4-pare-feu.sh`** — relire les règles du pare-feu et ouvrir ou fermer un port en entrée, sans retenir la syntaxe
  - enrobe : ufw (status, allow, deny) s'il est installé, sinon nft (list ruleset, add rule)
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/metiers/10-securite-informatique/5-chiffrer.sh`** — chiffrer un fichier avec une phrase secrète avant de l'envoyer ou de le stocker, et effacer l'original si on le veut
  - enrobe : gpg --symmetric ; shred pour effacer l'original
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu


### Serveur sans écran (`11-serveur-sans-ecran`)

- **`examples/metiers/11-serveur-sans-ecran/1-reseau.sh`** — passer une connexion réseau du serveur en adresse fixe (ou revenir en DHCP) sans se tromper de syntaxe
  - enrobe : nmcli (connection show en lecture ; connection modify et up pour appliquer)
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/11-serveur-sans-ecran/2-mises-a-jour.sh`** — voir les paquets à mettre à jour et choisir entre tout mettre à jour ou un seul paquet
  - enrobe : apt list --upgradable (lecture) ; apt-get upgrade ou apt-get install --only-upgrade
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/11-serveur-sans-ecran/3-sauvegarde-base.sh`** — sauvegarder une base PostgreSQL ou MariaDB/MySQL dans un fichier daté, avec les bonnes options
  - enrobe : pg_dump ou mysqldump
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/11-serveur-sans-ecran/4-taches-cron.sh`** — relire ses tâches planifiées et en ajouter une sans écrire la ligne cron à la main
  - enrobe : crontab -l (lecture) ; crontab - pour installer la table complétée
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu

- **`examples/metiers/11-serveur-sans-ecran/5-etat-serveur.sh`** — un tableau de bord d'un coup d'œil (durée de marche, charge, mémoire, disques, services en échec) et relancer un service tombé
  - enrobe : uptime, /proc/loadavg, free, df, systemctl --failed (lecture) ; systemctl restart
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu


### Web local / intranet (`12-web-local-intranet`)

- **`examples/metiers/12-web-local-intranet/1-demande-materiel.sh`** — recueillir une demande de matériel dans un navigateur et l'ajouter au registre des demandes (CSV)
  - enrobe : htmlsermo en mode serve (page sur 127.0.0.1) ; ajout d'une ligne au registre CSV
  - ports : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre ; terminal : rien de perdu

- **`examples/metiers/12-web-local-intranet/2-rapport.sh`** — publier sur l'intranet une page de rapport (erreurs du journal et occupation des disques) sur une période choisie
  - enrobe : journalctl et df (lecture) ; htmlsermo en mode export pour écrire la page
  - ports : gtk3sermo (ou tout port) pour les réglages ; htmlsermo en mode export pour la page ; terminal : rien de perdu

- **`examples/metiers/12-web-local-intranet/3-sondage-interne.sh`** — faire remplir un sondage interne anonyme dans un navigateur et cumuler les réponses dans un CSV
  - enrobe : htmlsermo en mode serve (page sur 127.0.0.1) ; ajout d'une ligne au fichier des réponses
  - ports : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre ; terminal : rien de perdu

- **`examples/metiers/12-web-local-intranet/4-reservation-ressource.sh`** — réserver une ressource partagée (salle, vidéoprojecteur, véhicule) en voyant les réservations déjà prises, sans double réservation
  - enrobe : htmlsermo en mode serve (page sur 127.0.0.1) ; lecture et ajout au registre CSV des réservations
  - ports : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre ; terminal : rien de perdu

- **`examples/metiers/12-web-local-intranet/5-annuaire.sh`** — chercher un collègue dans l'annuaire interne (nom, service, poste, courriel) et lui écrire
  - enrobe : grep sur l'annuaire CSV (lecture) ; xdg-email pour préparer le courriel
  - ports : htmlsermo conseillé (SERMO_HTML_MODE=serve, posé par défaut) ; tout autre port en fenêtre ; terminal : rien de perdu


## Boîtes à outils (examples/boites-a-outils)

Chaque boîte : un `lanceur.sh` qui présente les dix outils, puis rouvre le menu.

### Système — régler son poste (`systeme`)

- **`examples/boites-a-outils/systeme/lanceur.sh`** — régler son poste Linux (écran, son, Wi-Fi, heure, clavier, souris, énergie, démarrage, applications par défaut, Bluetooth) depuis une seule fenêtre
  - enrobe : les dix outils de ce dossier (01-ecran.sh … 10-bluetooth.sh)
  - ports : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible (mais la plupart des réglages visent une session graphique) ; terminal : rien de perdu dans le lanceur lui-même

- **`examples/boites-a-outils/systeme/01-ecran.sh`** — régler un écran (résolution, fréquence, échelle, place par rapport à un autre écran) et la lisibilité (police, DPI, couleur de fond)
  - enrobe : xrandr ; xfconf-query (police et DPI sous XFCE) ; xsetroot (couleur de fond sans gestionnaire de bureau)
  - ports : gtk3sermo ou qt6sermo conseillés ; terminal : sans objet : xrandr règle une session X, pas un terminal

- **`examples/boites-a-outils/systeme/02-son.sh`** — choisir la sortie et l'entrée du son, régler leur volume, couper ou rétablir le son
  - enrobe : pactl (PipeWire ou PulseAudio) ; wpctl (WirePlumber) en repli pour le volume et le muet
  - ports : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible ; terminal : rien de perdu

- **`examples/boites-a-outils/systeme/03-wifi.sh`** — se connecter à un réseau Wi-Fi, revenir sur un réseau connu, en oublier un, allumer ou éteindre le Wi-Fi
  - enrobe : nmcli (NetworkManager)
  - ports : gtk3sermo ou qt6sermo conseillés ; ncursessermo par SSH (attention : couper le Wi-Fi coupe la session) ; terminal : rien de perdu

- **`examples/boites-a-outils/systeme/04-date-heure.sh`** — choisir son fuseau horaire, la mise à l'heure automatique (NTP), ou régler l'heure à la main
  - enrobe : timedatectl (systemd)
  - ports : gtk3sermo ou qt6sermo conseillés ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/systeme/05-clavier-langue.sh`** — changer la disposition du clavier (AZERTY, BÉPO, QWERTY…), sa variante et ses options, et la langue du poste
  - enrobe : setxkbmap (la session en cours) ; localectl (tout le poste, droits d'administration)
  - ports : gtk3sermo ou qt6sermo conseillés ; terminal : sans objet pour setxkbmap (session X) ; la partie localectl marche aussi par SSH

- **`examples/boites-a-outils/systeme/06-souris-pave.sh`** — régler la souris ou le pavé tactile : vitesse du pointeur, défilement naturel, tap pour cliquer, gaucher
  - enrobe : xinput (pilote libinput)
  - ports : gtk3sermo ou qt6sermo conseillés ; terminal : sans objet : xinput règle une session X, pas un terminal

- **`examples/boites-a-outils/systeme/07-energie.sh`** — choisir au bout de combien de minutes l'écran s'assombrit, se met en veille puis s'éteint
  - enrobe : xfconf-query (gestionnaire d'énergie XFCE, durable) ; xset dpms (session X en cours)
  - ports : gtk3sermo ou qt6sermo conseillés ; terminal : sans objet : ces réglages visent l'écran d'une session graphique

- **`examples/boites-a-outils/systeme/08-applications-demarrage.sh`** — choisir les applications lancées à l'ouverture de session : en désactiver, en réactiver, en ajouter, en retirer
  - enrobe : fichiers .desktop de ~/.config/autostart (norme XDG Autostart) ; cp, sed, rm
  - ports : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible ; terminal : rien de perdu

- **`examples/boites-a-outils/systeme/09-applications-par-defaut.sh`** — choisir l'application qui ouvre les liens web, les PDF, les textes et les images
  - enrobe : xdg-settings (navigateur) ; xdg-mime (types de fichiers)
  - ports : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible ; terminal : rien de perdu

- **`examples/boites-a-outils/systeme/10-bluetooth.sh`** — voir ses appareils Bluetooth, en chercher de nouveaux, les appairer, leur faire confiance, les connecter ou les oublier
  - enrobe : bluetoothctl (BlueZ)
  - ports : gtk3sermo ou qt6sermo conseillés ; ncursessermo possible ; terminal : rien de perdu


### Serveur — tenir un serveur, souvent par SSH (`serveur`)

- **`examples/boites-a-outils/serveur/lanceur.sh`** — choisir l'un des dix outils de la boîte serveur, puis revenir au menu après chaque outil
  - enrobe : les dix scripts 01-…sh à 10-…sh posés à côté de ce lanceur
  - ports : ncursessermo conseillé (par SSH, sans écran) ; gtk3sermo sur un poste ; terminal : rien de perdu (les onglets deviennent des pages ; le menu attend Entrée avant de revenir, pour lire ce que l'outil a écrit)

- **`examples/boites-a-outils/serveur/01-site-web.sh`** — publier un site nginx (statique, proxy vers une appli, ou PHP) sans écrire le bloc server à la main, et ne recharger que si nginx -t passe
  - enrobe : tee /etc/nginx/sites-available/<domaine> ; ln -s vers sites-enabled ; nginx -t ; systemctl reload nginx
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : la couleur et la police de la page d'attente se saisissent comme du texte (pas de nuancier ni de sélecteur de police) ; l'aperçu du fichier tient sur une ligne

- **`examples/boites-a-outils/serveur/02-certificats.sh`** — obtenir un certificat TLS Let's Encrypt, vérifier que le renouvellement marchera, voir ce qui expire bientôt
  - enrobe : certbot (--nginx, --webroot ou --standalone) ; certbot renew --dry-run ; certbot certificates ; certbot delete
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (les onglets deviennent des pages ; le tableau montre six certificats à la fois)

- **`examples/boites-a-outils/serveur/03-ports-ecoute.sh`** — voir quels programmes écoutent sur le réseau, et arrêter le service qui n'a rien à y faire
  - enrobe : ss -tulnp (lecture) ; ps -o unit= pour trouver l'unité ; systemctl stop / disable / mask
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (le tableau montre six lignes à la fois) ; couper ssh demande de retaper son nom, et le masquer est refusé

- **`examples/boites-a-outils/serveur/04-ssh-durcissement.sh`** — durcir le serveur SSH (mot de passe, root, port, comptes permis) sans toucher à sshd_config, et ne recharger que si sshd -t valide
  - enrobe : tee /etc/ssh/sshd_config.d/10-durcissement.conf ; sshd -t ; systemctl reload ssh
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (l'interrupteur et le bouton à bascule deviennent des cases ; l'aperçu tient sur une ligne) ; gardez une seconde session SSH ouverte pendant l'essai

- **`examples/boites-a-outils/serveur/05-fail2ban.sh`** — voir les prisons fail2ban et les adresses bannies, débannir une adresse légitime (ou en bannir une à la main)
  - enrobe : fail2ban-client status (lecture) ; fail2ban-client set <prison> unbanip / banip ; fail2ban-client unban
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (les listes montrent six lignes à la fois)

- **`examples/boites-a-outils/serveur/06-base-de-donnees.sh`** — créer une base et le compte d'une application (PostgreSQL ou MariaDB) sans taper le SQL, mot de passe compris
  - enrobe : runuser -u postgres -- psql (SQL lu sur l'entrée) ; mariadb (SQL lu sur l'entrée, root par le socket)
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (le mot de passe reste masqué)

- **`examples/boites-a-outils/serveur/07-rotation-journaux.sh`** — empêcher le dossier de journaux d'une application de remplir le disque, avec une règle logrotate essayée avant d'être laissée en place
  - enrobe : tee /etc/logrotate.d/<nom> ; logrotate --debug (essai sans rien tourner)
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (la glissière devient une barre de caractères ; l'aperçu tient sur une ligne)

- **`examples/boites-a-outils/serveur/08-partages.sh`** — partager un dossier du serveur, en NFS pour des machines Linux ou en Samba pour des postes Windows et Mac
  - enrobe : tee /etc/exports.d/<nom>.exports + exportfs -ra ; tee -a /etc/samba/smb.conf + testparm -s + systemctl reload smbd
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (les onglets deviennent des pages, l'interrupteur une case)

- **`examples/boites-a-outils/serveur/09-alertes.sh`** — être prévenu par courriel avant que le disque, la mémoire ou la charge ne fassent tomber le serveur, sans installer d'outil de supervision
  - enrobe : écrit /usr/local/sbin/alertes-serveur + une unité .service et .timer ; systemctl daemon-reload ; systemctl enable --now ; mail (bsd-mailx) pour l'envoi
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (les glissières deviennent des barres de caractères)

- **`examples/boites-a-outils/serveur/10-minuteries-systemd.sh`** — voir les tâches planifiées par systemd, en activer ou en couper une, et en créer une nouvelle sans écrire les deux unités à la main
  - enrobe : systemctl list-timers et systemctl show (lecture) ; systemd-analyze calendar (contrôle) ; tee des unités .service/.timer ; systemctl daemon-reload, enable/disable --now, start
  - ports : ncursessermo conseillé (par SSH) ; gtk3sermo sur un poste ; terminal : rien de perdu (les onglets deviennent des pages ; le calendrier devient une saisie de date ; le tableau montre six minuteries à la fois)


### Administrateur — administrer une machine Debian (`administrateur`)

- **`examples/boites-a-outils/administrateur/lanceur.sh`** — ouvrir l'un des dix outils d'administration d'une machine Debian depuis une seule fenêtre
  - enrobe : les dix scripts 01-… à 10-… de ce dossier
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/01-groupes.sh`** — créer un groupe, y ajouter ou en retirer un membre
  - enrobe : groupadd ; gpasswd -a / gpasswd -d ; getent group pour la liste
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/02-droits-fichiers.sh`** — régler le propriétaire, le mode et une ACL d'un dossier partagé, en confirmant si c'est récursif
  - enrobe : chown ; chmod ; setfacl (paquet acl) ; getfacl pour l'état actuel
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/03-paquets.sh`** — chercher un paquet Debian, puis l'installer ou le retirer ; la purge (configuration effacée) se confirme
  - enrobe : apt-cache search (lecture) ; apt install / apt remove / apt purge
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/04-depots-apt.sh`** — ajouter un dépôt apt au format deb822, ou en désactiver un, en voyant le fichier avant de l'écrire
  - enrobe : fichiers /etc/apt/sources.list.d/*.sources (install -m 0644) ; apt update ensuite
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/05-montages.sh`** — monter un disque à demeure : ligne fstab par UUID, vérifiée à blanc avant de monter
  - enrobe : lsblk (lecture) ; /etc/fstab (ajout) ; findmnt --verify ; mount --fake -a ; mount
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/06-formatage.sh`** — formater un disque ou une partition avec un système de fichiers et une étiquette, sans se tromper de périphérique
  - enrobe : lsblk et findmnt (lecture) ; mkfs.ext4, mkfs.xfs, mkfs.btrfs, mkfs.vfat, mkfs.exfat
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/07-modules-noyau.sh`** — charger ou décharger un module du noyau, ou le mettre sur liste noire pour les prochains démarrages
  - enrobe : /proc/modules et modinfo (lecture) ; modprobe ; modprobe -r ; fichier /etc/modprobe.d/*.conf ; update-initramfs
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/08-regles-sudo.sh`** — donner à une personne ou à un groupe le droit de lancer quelques commandes précises en root, par une règle vérifiée
  - enrobe : fichier /etc/sudoers.d/<nom> (install -m 0440) vérifié par visudo -c ; ce script ne lance jamais sudo
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/09-nom-machine.sh`** — renommer la machine, et garder /etc/hosts d'accord avec le nouveau nom
  - enrobe : hostnamectl (hostname, pretty, chassis, location) ; sed sur la ligne 127.0.1.1 de /etc/hosts
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu

- **`examples/boites-a-outils/administrateur/10-redemarrage-programme.sh`** — programmer un redémarrage ou un arrêt à une heure donnée (ou dans N minutes), prévenir les personnes connectées, ou annuler
  - enrobe : shutdown -r / shutdown -h HH:MM ou +N, avec message ; shutdown -c pour annuler
  - ports : gtk3sermo conseillé ; ncursessermo par SSH ; terminal : rien de perdu


### Utilisateur — outils du quotidien, sans connaissances techniques (`utilisateur`)

- **`examples/boites-a-outils/utilisateur/lanceur.sh`** — trouver d'un coup d'œil les dix petits outils du quotidien, et en ouvrir un d'un clic
  - enrobe : rien (ouvre les outils de ce dossier, un par un)
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : rien de perdu (mais la boîte vise un bureau graphique)

- **`examples/boites-a-outils/utilisateur/01-rechercher-fichiers.sh`** — retrouver un fichier perdu par son nom, son genre, sa taille ou sa date, puis ouvrir son dossier
  - enrobe : find (lecture seule) ; xdg-open pour ouvrir le dossier trouvé
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : en ncurses, « Ouvrir le dossier » n'a pas d'effet sans bureau graphique

- **`examples/boites-a-outils/utilisateur/02-archives.sh`** — regrouper un dossier dans un seul fichier zip ou tar.gz, ou sortir les fichiers d'une archive reçue
  - enrobe : zip, unzip, tar
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : rien de perdu

- **`examples/boites-a-outils/utilisateur/03-mot-de-passe.sh`** — inventer un mot de passe solide (ou une phrase de passe facile à retenir), le voir en grand, sans qu'il soit enregistré nulle part
  - enrobe : /dev/urandom (via tr et od) ; rien n'est écrit sur le disque
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : en ncurses, la police et la couleur choisies ne s'appliquent pas

- **`examples/boites-a-outils/utilisateur/04-rappel.sh`** — se faire un rappel qui s'affichera sur le bureau à la date et à l'heure choisies ; voir et annuler ses rappels
  - enrobe : at, atq, atrm ; notify-send pour la bulle à l'heure dite
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : en ncurses, la bulle s'affiche quand même sur le bureau s'il y en a un

- **`examples/boites-a-outils/utilisateur/05-imprimer.sh`** — imprimer un fichier en choisissant l'imprimante, le nombre de copies, le recto verso et les pages
  - enrobe : lp ; lpstat pour lire les imprimantes et la file d'attente
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : rien de perdu

- **`examples/boites-a-outils/utilisateur/06-convertir-documents.sh`** — transformer un document texte (ODT, DOCX, DOC, RTF) en PDF pour l'envoyer ou l'imprimer
  - enrobe : libreoffice --headless --convert-to pdf
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : rien de perdu (LibreOffice travaille sans fenêtre)

- **`examples/boites-a-outils/utilisateur/07-ranger-dossier.sh`** — mettre de l'ordre dans un dossier en vrac (Téléchargements…) : trier les fichiers en sous-dossiers par genre ou par mois, et mettre les très vieux à la corbeille
  - enrobe : mkdir, mv -n ; gio trash pour la corbeille (rien n'est effacé pour de bon)
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : rien de perdu

- **`examples/boites-a-outils/utilisateur/08-copie-cle-usb.sh`** — copier un dossier sur une clé USB branchée, en voyant d'abord combien partira et la place libre ; rien n'est effacé sur la clé par défaut
  - enrobe : rsync (sans --delete par défaut) ; df pour la place libre
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : rien de perdu

- **`examples/boites-a-outils/utilisateur/09-partager-sur-le-reseau.sh`** — prêter un fichier à un appareil de la maison (téléphone, autre ordinateur) pendant quelques minutes, sans clé USB ni compte en ligne
  - enrobe : python3 -m http.server, dans un dossier dédié qui ne contient QUE la copie du fichier, arrêté par timeout
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : rien de perdu

- **`examples/boites-a-outils/utilisateur/10-place-disque.sh`** — savoir quels dossiers prennent le plus de place dans son dossier personnel, descendre dedans, et ouvrir celui qu'on veut trier soi-même
  - enrobe : du (lecture seule) ; df pour la place libre ; xdg-open pour ouvrir le dossier
  - ports : gtk3sermo conseillé ; qt6sermo ; htmlsermo ; terminal : en ncurses, « Ouvrir ce dossier » n'a pas d'effet sans bureau graphique


(104 scripts.)
