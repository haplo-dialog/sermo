# Packaging RPM — qt6sermo

## Construction du paquet

```bash
# Installer les outils
sudo dnf install rpm-build rpmdevtools

# Créer l'arbre rpmbuild
rpmdev-setuptree

# Copier le spec
cp packaging/rpm/qt6sermo.spec ~/rpmbuild/SPECS/

# Créer l'archive source
tar czf ~/rpmbuild/SOURCES/qt6sermo-1.0.0.tar.gz --transform 's,^,qt6sermo-1.0.0/,' .

# Builder le RPM
rpmbuild -ba ~/rpmbuild/SPECS/qt6sermo.spec

# Résultat
ls ~/rpmbuild/RPMS/x86_64/qt6sermo-1.0.0-*.rpm
```

## Dépendances de build

```bash
# Fedora / RHEL
sudo dnf install qt6-qtbase-devel >= 6.2 qt6-qttools-devel flex bison cmake >= 3.20 gcc-c++

# openSUSE
sudo zypper install qt6-qtbase-devel >= 6.2 qt6-qttools-devel flex bison cmake >= 3.20 gcc-c++
```

## Installation du paquet

```bash
sudo rpm -ivh qt6sermo-1.0.0-1.fc40.x86_64.rpm
# ou
sudo dnf install ./qt6sermo-1.0.0-1.fc40.x86_64.rpm
```

---

## Assistance IA

Ces fichiers de packaging ont été créés avec l'assistance de **Claude** (Anthropic).

> *"Ce projet a bénéficié de l'assistance de Claude (Anthropic) — mai 2026."*

---

> **TODO :** installer aussi `data/qt6sermo.desktop` et `data/qt6sermo.metainfo.xml` (fournis depuis la 1.0.0 ; déjà intégrés au build amont et au paquet Debian).

*Document horodaté — dernière mise à jour : 2026-06-10 (audit #6 — haplo-dialog v1.0.0).*