# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 S. Cage
# haplo-dialog <devel@haplo-dialog.fr>
#
# sermo — un paquet source, onze paquets binaires : le cœur (sermo-core-devel),
# le manuel (sermo-doc) et un paquet par backend. La commande « sermo » passe par
# le mécanisme alternatives, comme update-alternatives sous Debian.
#
# ⚠️ Écrit sans rpmbuild sous la main : la suite des gestes de construction est
# rejouée (packaging/autres-os/README.md), pas les noms de paquets Fedora.

%global _ports gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses html
# Le projet pose lui-même -D_FORTIFY_SOURCE=3, PIE, RELRO, CET.
%global _lto_cflags %{nil}

Name:           sermo
Version:        2.7.7
Release:        1%{?dist}
Summary:        Dialog boxes for shell scripts, described in XML (gtkdialog successor)
License:        GPL-2.0-or-later
URL:            https://gitlab.com/haplo-dialog/sermo
Source0:        https://gitlab.com/haplo-dialog/sermo/-/archive/v%{version}/sermo-v%{version}.tar.gz

BuildRequires:  gcc gcc-c++ cmake flex bison pkgconf-pkg-config texinfo
BuildRequires:  SDL3-devel
BuildRequires:  efl-devel
BuildRequires:  fltk-devel
BuildRequires:  gtk-layer-shell-devel
BuildRequires:  gtk3-devel
BuildRequires:  gtk4-devel
BuildRequires:  libX11-devel
BuildRequires:  libpng-devel
BuildRequires:  libxkbcommon-devel
BuildRequires:  ncurses-devel
BuildRequires:  qt6-qtbase-devel
BuildRequires:  vte291-devel
BuildRequires:  vte291-gtk4-devel
BuildRequires:  wayland-devel
BuildRequires:  wayland-protocols-devel

%description
sermo builds native windows from an XML description, so that a shell script can
show a real graphical interface without any toolkit knowledge. It succeeds
gtkdialog and keeps its XML language. One hardened core parses the description
and runs the actions; each backend draws it with one toolkit.

%package -n sermo-core-devel
Summary:        sermo dialog engine - static core library and headers
%description -n sermo-core-devel
The sermo core as a static library, with its headers and sermocore.pc.

%package -n sermo-doc
Summary:        sermo manual (Texinfo) and common documentation
BuildArch:      noarch
%description -n sermo-doc
The sermo manual ("info sermo-doc").

%package -n sermo-backend-gtk3
Summary:        sermo dialog boxes for shell scripts - GTK 3 backend (gtkdialog-compatible command)
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
Requires:       hicolor-icon-theme
%description -n sermo-backend-gtk3
GTK 3 backend (gtkdialog-compatible command). It also offers the sermo command through the
alternatives system (priority 50).

%package -n sermo-backend-gtk4
Summary:        sermo dialog boxes for shell scripts - GTK 4 backend
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
Requires:       hicolor-icon-theme
%description -n sermo-backend-gtk4
GTK 4 backend. It also offers the sermo command through the
alternatives system (priority 45).

%package -n sermo-backend-qt6
Summary:        sermo dialog boxes for shell scripts - Qt 6 backend
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
Requires:       hicolor-icon-theme
%description -n sermo-backend-qt6
Qt 6 backend. It also offers the sermo command through the
alternatives system (priority 40).

%package -n sermo-backend-fltk1
Summary:        sermo dialog boxes for shell scripts - FLTK 1.4 backend
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
Requires:       hicolor-icon-theme
%description -n sermo-backend-fltk1
FLTK 1.4 backend. It also offers the sermo command through the
alternatives system (priority 30).

%package -n sermo-backend-efl1
Summary:        sermo dialog boxes for shell scripts - EFL (Elementary) backend
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
Requires:       hicolor-icon-theme
%description -n sermo-backend-efl1
EFL (Elementary) backend. It also offers the sermo command through the
alternatives system (priority 20).

%package -n sermo-backend-sdl3
Summary:        sermo dialog boxes for shell scripts - SDL 3 backend, drawn with Dear ImGui
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
Requires:       hicolor-icon-theme
%description -n sermo-backend-sdl3
SDL 3 backend, drawn with Dear ImGui. It also offers the sermo command through the
alternatives system (priority 10).

%package -n sermo-backend-lvgl
Summary:        sermo dialog boxes for shell scripts - LVGL backend for kiosks and embedded screens (LVGL embedded)
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
%description -n sermo-backend-lvgl
LVGL backend for kiosks and embedded screens (LVGL embedded). It also offers the sermo command through the
alternatives system (priority 8).

%package -n sermo-backend-ncurses
Summary:        sermo dialog boxes for shell scripts - terminal (ncurses) backend, for machines without a display
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
%description -n sermo-backend-ncurses
Terminal (ncurses) backend, for machines without a display. It also offers the sermo command through the
alternatives system (priority 5).

%package -n sermo-backend-html
Summary:        sermo dialog boxes for shell scripts - static HTML export backend (writes a page instead of a window)
Provides:       sermo-backend
Requires(post):   %{_sbindir}/alternatives
Requires(postun): %{_sbindir}/alternatives
%description -n sermo-backend-html
Static HTML export backend (writes a page instead of a window). It also offers the sermo command through the
alternatives system (priority 1).

%prep
%autosetup -n sermo-v%{version}

%build
%set_build_flags
sh packaging/autres-os/construire-os.sh construire . "$PWD/construit" %{_ports}

%install
for p in %{_ports}; do
    sh packaging/autres-os/construire-os.sh installer . "$PWD/construit" "$p" %{buildroot} %{_prefix}
done
sh packaging/autres-os/construire-os.sh coeur-dev . "$PWD/construit" %{buildroot} %{_prefix} %{_lib}
sh packaging/autres-os/construire-os.sh manuel . "$PWD/construit" %{buildroot} %{_prefix}
# La commande générique et ses liens : posés par alternatives, pas par le paquet.
mkdir -p %{buildroot}%{_sysconfdir}/alternatives
touch %{buildroot}%{_bindir}/sermo %{buildroot}%{_mandir}/man1/sermo.1.gz \
      %{buildroot}%{_sysconfdir}/alternatives/sermo

%files -n sermo-core-devel
%license COPYING
%{_includedir}/sermo
%{_libdir}/libsermocore.a
%{_libdir}/pkgconfig/sermocore.pc

%files -n sermo-doc
%license COPYING
%{_infodir}/sermo-doc.info*

%files -n sermo-backend-gtk3
%license COPYING
%{_bindir}/gtk3sermo
%{_mandir}/man1/gtk3sermo.1*
%{_mandir}/man5/haplo-dialog-xml.5*
%{_datadir}/icons/hicolor/*/apps/gtk3sermo.png
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-gtk3
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/gtk3sermo 50 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/gtk3sermo.1.gz || :

%postun -n sermo-backend-gtk3
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/gtk3sermo || :
fi

%files -n sermo-backend-gtk4
%license COPYING
%{_bindir}/gtk4sermo
%{_mandir}/man1/gtk4sermo.1*
%{_mandir}/man5/gtk4sermo-xml.5*
%{_datadir}/icons/hicolor/*/apps/gtk4sermo.png
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-gtk4
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/gtk4sermo 45 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/gtk4sermo.1.gz || :

%postun -n sermo-backend-gtk4
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/gtk4sermo || :
fi

%files -n sermo-backend-qt6
%license COPYING
%{_bindir}/qt6sermo
%{_mandir}/man1/qt6sermo.1*
%{_datadir}/applications/qt6sermo.desktop
%{_datadir}/metainfo/fr.haplo_dialog.qt6sermo.metainfo.xml
%{_datadir}/icons/hicolor/*/apps/qt6sermo.png
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-qt6
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/qt6sermo 40 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/qt6sermo.1.gz || :

%postun -n sermo-backend-qt6
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/qt6sermo || :
fi

%files -n sermo-backend-fltk1
%license COPYING
%{_bindir}/fltk1sermo
%{_mandir}/man1/fltk1sermo.1*
%{_datadir}/icons/hicolor/*/apps/fltk1sermo.png
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-fltk1
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/fltk1sermo 30 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/fltk1sermo.1.gz || :

%postun -n sermo-backend-fltk1
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/fltk1sermo || :
fi

%files -n sermo-backend-efl1
%license COPYING
%{_bindir}/efl1sermo
%{_mandir}/man1/efl1sermo.1*
%{_datadir}/icons/hicolor/*/apps/efl1sermo.png
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-efl1
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/efl1sermo 20 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/efl1sermo.1.gz || :

%postun -n sermo-backend-efl1
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/efl1sermo || :
fi

%files -n sermo-backend-sdl3
%license COPYING
%{_bindir}/sdl3sermo
%{_mandir}/man1/sdl3sermo.1*
%{_datadir}/icons/hicolor/*/apps/sdl3sermo.png
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-sdl3
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/sdl3sermo 10 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/sdl3sermo.1.gz || :

%postun -n sermo-backend-sdl3
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/sdl3sermo || :
fi

%files -n sermo-backend-lvgl
%license COPYING
%{_bindir}/lvglsermo
%{_mandir}/man1/lvglsermo.1*
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-lvgl
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/lvglsermo 8 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/lvglsermo.1.gz || :

%postun -n sermo-backend-lvgl
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/lvglsermo || :
fi

%files -n sermo-backend-ncurses
%license COPYING
%{_bindir}/ncursessermo
%{_mandir}/man1/ncursessermo.1*
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-ncurses
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/ncursessermo 5 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/ncursessermo.1.gz || :

%postun -n sermo-backend-ncurses
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/ncursessermo || :
fi

%files -n sermo-backend-html
%license COPYING
%{_bindir}/htmlsermo
%{_mandir}/man1/htmlsermo.1*
%ghost %{_bindir}/sermo
%ghost %{_mandir}/man1/sermo.1.gz
%ghost %{_sysconfdir}/alternatives/sermo

%post -n sermo-backend-html
%{_sbindir}/alternatives --install %{_bindir}/sermo sermo %{_bindir}/htmlsermo 1 \
    --slave %{_mandir}/man1/sermo.1.gz sermo.1.gz %{_mandir}/man1/htmlsermo.1.gz || :

%postun -n sermo-backend-html
if [ $1 -eq 0 ]; then
    %{_sbindir}/alternatives --remove sermo %{_bindir}/htmlsermo || :
fi

%changelog
* Sat Oct 10 2026 s.cage <devel@haplo-dialog.fr> - %{version}-1
- Recette hors Debian : neuf backends (gtk3 gtk4 qt6 fltk1 efl1 sdl3 lvgl ncurses
  html), cœur et manuel en paquets séparés.
