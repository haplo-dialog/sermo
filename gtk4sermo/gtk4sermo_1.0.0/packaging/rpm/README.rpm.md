# Packaging RPM — gtk4sermo

## Construction du paquet

```bash
# Installer les outils
sudo dnf install rpm-build rpmdevtools

# Créer l'arbre rpmbuild
rpmdev-setuptree

# Copier le spec
cp packaging/rpm/gtk4sermo.spec ~/rpmbuild/SPECS/

# Créer l'archive source
tar czf ~/rpmbuild/SOURCES/gtk4sermo-1.0.0.tar.gz --transform 's,^,gtk4sermo-1.0.0/,' .

# Builder le RPM
rpmbuild -ba ~/rpmbuild/SPECS/gtk4sermo.spec

# Résultat
ls ~/rpmbuild/RPMS/x86_64/gtk4sermo-1.0.0-*.rpm
```

## Dépendances de build

```bash
# Fedora / RHEL
sudo dnf install gtk4-devel vte291-gtk4-devel flex bison autoconf automake pkgconfig

# openSUSE
sudo zypper install gtk4-devel vte291-gtk4-devel flex bison autoconf automake pkgconfig
```

## Installation du paquet

```bash
sudo rpm -ivh gtk4sermo-1.0.0-1.fc40.x86_64.rpm
# ou
sudo dnf install ./gtk4sermo-1.0.0-1.fc40.x86_64.rpm
```

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) — mai 2026."*

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
