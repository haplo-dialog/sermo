%global name gtk4sermo
%global version 1.0.0
%global release 1

Name:           gtk4sermo
Version:        %{version}
Release:        %{release}%{?dist}
Summary:        GTK4 dialog window creator for shell scripts

License:        GPL-2.0-or-later
URL:            https://haplo-dialog.fr
Source0:        https://gitlab.com/haplo-dialog/sermo/-/archive/v1.0.0/sermo-v1.0.0.tar.gz
Requires: gtk4-devel
BuildRequires: vte291-gtk4-devel
BuildRequires: flex
BuildRequires: bison
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: pkgconfig

Requires: gtk4
Requires: vte291-gtk4

%description
gtk4sermo builds GTK4 dialog windows from an XML-like description language. Port GTK4 of gtk3d 1.0.0 — installs alongside gtk3d without conflict. 50 widgets including GTK4-native: stack, revealer, flowbox, overlay, drawingarea. Security: safe_exec, FORTIFY_SOURCE=3, PIE, Full RELRO.

Toolkit: GTK4.
Security hardening: safe_exec, FORTIFY_SOURCE=3, PIE, Full RELRO, stack-protector-strong.

%build
NOCONFIGURE=1 ./autogen.sh
%configure 
%make_build

%install
%make_install


# Install examples
install -d %{buildroot}%{_docdir}/%{name}/examples
cp -r examples/* %{buildroot}%{_docdir}/%{name}/examples/ 2>/dev/null || true

%files
%license COPYING
%doc README NEWS AUTHORS
%{_bindir}/gtk4sermo
%{_mandir}/man1/gtk4sermo.1*
%dir %{_docdir}/%{name}/examples

%changelog
* Thu May 28 2026 haplo-dialog <devel@haplo-dialog.fr> - 1.0.0-1
- Initial RPM packaging for gtk4sermo (haplo-dialog fork)
- GTK4 port — GTK4 dialog window creator for shell scripts
- Security: safe_exec, FORTIFY_SOURCE=3, PIE, Full RELRO
