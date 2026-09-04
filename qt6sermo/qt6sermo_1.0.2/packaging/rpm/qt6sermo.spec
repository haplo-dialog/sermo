%global name qt6sermo
%global version 1.0.0
%global release 1

Name:           qt6sermo
Version:        %{version}
Release:        %{release}%{?dist}
Summary:        Qt6 dialog window creator for shell scripts

License:        GPL-2.0-or-later
URL:            https://haplo-dialog.fr
Source0:        %{name}-%{version}.tar.gz
Requires: qt6-qtbase-devel >= 6.2
BuildRequires: qt6-qttools-devel
BuildRequires: flex
BuildRequires: bison
BuildRequires: cmake >= 3.20
BuildRequires: gcc-c++

Requires: qt6-qtbase >= 6.2

%description
qt6sermo builds Qt6-based dialog windows from an XML-like description language. Most complete port: 41/41 widgets including QMenuBar, QTableWidget, QFontDialog. Targets KDE Plasma 6, Qt-based desktops. Security: safe_exec, PIE, RELRO.

Toolkit: Qt6.
Security hardening: safe_exec, FORTIFY_SOURCE=3, PIE, Full RELRO, stack-protector-strong.

%build
%cmake -DCMAKE_BUILD_TYPE=Release
%cmake_build

%install
%cmake_install


# Install examples
install -d %{buildroot}%{_docdir}/%{name}/examples
cp -r examples/* %{buildroot}%{_docdir}/%{name}/examples/ 2>/dev/null || true

%files
%license COPYING
%doc README NEWS AUTHORS
%{_bindir}/qt6sermo
%{_mandir}/man1/qt6sermo.1*
%dir %{_docdir}/%{name}/examples

%changelog
* Thu May 28 2026 haplo-dialog <devel@haplo-dialog.fr> - 1.0.0-1
- Initial RPM packaging for qt6sermo (haplo-dialog fork)
- Qt6 port — Qt6 dialog window creator for shell scripts
- Security: safe_exec, FORTIFY_SOURCE=3, PIE, Full RELRO
