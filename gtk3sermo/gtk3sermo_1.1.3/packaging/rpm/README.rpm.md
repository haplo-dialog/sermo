# Packaging RPM — gtk3sermo

## Construction du paquet

```bash
# Installer les outils
sudo dnf install rpm-build rpmdevtools

# Créer l'arbre rpmbuild
rpmdev-setuptree

# Copier le spec
cp packaging/rpm/gtk3sermo.spec ~/rpmbuild/SPECS/

# Créer l'archive source
tar czf ~/rpmbuild/SOURCES/gtk3sermo-1.0.0.tar.gz --transform 's,^,gtk3sermo-1.0.0/,' .

# Builder le RPM
rpmbuild -ba ~/rpmbuild/SPECS/gtk3sermo.spec

# Résultat
ls ~/rpmbuild/RPMS/x86_64/gtk3sermo-1.0.0-*.rpm
```

## Dépendances de build

```bash
# Fedora / RHEL
sudo dnf install gtk3-devel vte291-devel flex bison autoconf automake pkgconfig

# openSUSE
sudo zypper install gtk3-devel vte291-devel flex bison autoconf automake pkgconfig
```

## Installation du paquet

```bash
sudo rpm -ivh gtk3sermo-1.0.0-1.fc40.x86_64.rpm
# ou
sudo dnf install ./gtk3sermo-1.0.0-1.fc40.x86_64.rpm
```

---

