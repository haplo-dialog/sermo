<div align="right"><sub><code>maj :     2026-08-20 20:34:10 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# Packaging Arch Linux — gtk4sermo

## Installation depuis AUR (future soumission)

```bash
# Avec yay
yay -S gtk4sermo

# Ou manuellement
git clone https://aur.archlinux.org/gtk4sermo.git
cd gtk4sermo
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
sudo pacman -S gtk4 vte4 flex bison autoconf automake pkgconfig
```

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) — mai 2026."*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
