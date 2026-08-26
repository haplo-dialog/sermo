# gtk3sermo.texi — manuel du port, construit et installé.
#
# info_TEXINFOS demande makeinfo au moment du build : le paquet texinfo est donc
# dans les Build-Depends. La génération était restée désactivée pour éviter cette
# dépendance, au prix d'un manuel qui n'existait que dans l'archive source —
# `info gtk3sermo` ne trouvait rien sur une machine où le paquet était installé.
# Activée le 2026-08-26.
#
# À la main, pour d'autres formats :
#   makeinfo --html gtk3sermo.texi    → HTML
#   makeinfo --pdf  gtk3sermo.texi    → PDF

info_TEXINFOS = gtk3sermo.texi

EXTRA_DIST = version.texi
