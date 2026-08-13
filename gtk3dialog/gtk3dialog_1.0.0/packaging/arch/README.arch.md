# Packaging Arch Linux — gtk3dialog

## Installation depuis AUR (future soumission)

```bash
# Avec yay
yay -S gtk3dialog

# Ou manuellement
git clone https://aur.archlinux.org/gtk3dialog.git
cd gtk3dialog
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
sudo pacman -S gtk3 vte3 flex bison autoconf automake pkgconfig
```

---

