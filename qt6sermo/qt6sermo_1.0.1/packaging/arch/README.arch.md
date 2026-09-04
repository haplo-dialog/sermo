# Packaging Arch Linux — qt6sermo

## Installation depuis AUR (future soumission)

```bash
# Avec yay
yay -S qt6sermo

# Ou manuellement
git clone https://aur.archlinux.org/qt6sermo.git
cd qt6sermo
makepkg -si
```

## Build local

```bash
cd packaging/arch/
# Mettre à jour le sha256sum
updpkgsums  # ou calculer manuellement
makepkg -si
```

## Dépendances

```bash
sudo pacman -S qt6-base qt6-tools flex bison cmake pkgconfig
```

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) — mai 2026."*

---

> **TODO :** installer aussi `data/qt6sermo.desktop` et `data/qt6sermo.metainfo.xml` (fournis depuis la 1.0.0 ; déjà intégrés au build amont et au paquet Debian).

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*