# Packaging Arch Linux — gtk3sermo

## Installation depuis AUR (future soumission)

```bash
# Avec yay
yay -S gtk3sermo

# Ou manuellement
git clone https://aur.archlinux.org/gtk3sermo.git
cd gtk3sermo
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

