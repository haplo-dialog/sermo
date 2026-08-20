
# gtk3dialog.texi — documentation GTK3 complète.
#
# La génération automatique reste désactivée pour éviter la dépendance
# makeinfo au moment du build Debian. Pour générer manuellement :
#   makeinfo --html gtk3dialog.texi      → HTML
#   makeinfo --pdf  gtk3dialog.texi      → PDF
#   makeinfo        gtk3dialog.texi      → gtk3dialog.info
#
# To re-enable automated generation:
#   pdf_TEXINFOS = gtk3dialog.texi
#   info_TEXINFOS = gtk3dialog.texi

# version.texi, stamp-vti et gtk3dialog.info sont produits par automake quand
# info_TEXINFOS est actif. Il ne l'est pas ici, donc ils n'existent jamais et
# les exiger faisait echouer « make dist ». Seule la source est distribuee.
EXTRA_DIST = gtk3dialog.texi

