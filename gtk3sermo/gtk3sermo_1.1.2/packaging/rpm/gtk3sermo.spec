%global name gtk3sermo
%global version 1.1.2
%global release 1

Name:           gtk3sermo
Version:        %{version}
Release:        %{release}%{?dist}
Summary:        GTK3 dialog window creator for shell scripts

License:        GPL-2.0-or-later
URL:            https://haplo-dialog.fr
Source0:        https://gitlab.com/haplo-dialog/sermo/-/archive/v1.1.0/sermo-v1.1.0.tar.gz
Requires: gtk3-devel
BuildRequires: vte291-devel
BuildRequires: flex
BuildRequires: bison
BuildRequires: autoconf
BuildRequires: automake
BuildRequires: pkgconfig
BuildRequires: texinfo

Requires: gtk3
Requires: vte291

%description
gtk3sermo builds GTK3 dialog windows from an XML-like description language, allowing shell scripts to display interactive GUI windows without knowledge of any graphical toolkit. This is the haplo-dialog fork (1.0.0) with security hardening (safe_exec, FORTIFY_SOURCE=3, PIE, Full RELRO, stack-protector-strong).

Toolkit: GTK3.
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
%{_bindir}/gtk3sermo
%{_bindir}/gtkdialog
%{_mandir}/man1/gtk3sermo.1*
%{_mandir}/man1/gtkdialog.1*
%dir %{_docdir}/%{name}/examples

%changelog
* Thu May 28 2026 haplo-dialog <devel@haplo-dialog.fr> - 1.0.0-1
- Initial RPM packaging for gtk3sermo (haplo-dialog fork)
- GTK3 port — GTK3 dialog window creator for shell scripts
- Security: safe_exec, FORTIFY_SOURCE=3, PIE, Full RELRO
