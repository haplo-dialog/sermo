# SPDX-License-Identifier: GPL-2.0-or-later
# Copyright (C) 2026 S. Cage
# haplo-dialog <devel@haplo-dialog.fr>
#
# sermo sous Nix : une dérivation par backend (et une pour le cœur), construites
# par packaging/autres-os/construire-os.sh depuis l'arbre où se trouve ce fichier.
#
#   nix-build packaging/autres-os/nix -A sermo-backend-gtk3
#   nix-build packaging/autres-os/nix -A all
#
# ⚠️ Écrit sans Nix sous la main : la suite des gestes de construction est
# rejouée (packaging/autres-os/README.md), pas les noms d'attributs nixpkgs.
# Vérifié en conteneur nixos/nix (nixpkgs-unstable, 10/10/2026) : les neuf ports construits.
{ pkgs ? import <nixpkgs> { } }:

let
  inherit (pkgs) lib stdenv;
  version = "2.8.2";
  src = lib.cleanSource ../../..;

  # Un backend : son nom de port (celui de construire-os.sh), ses entrées, sa
  # description. -fortify est retiré : le projet pose lui-même _FORTIFY_SOURCE=3.
  mkBackend = { port, buildInputs ? [ ], nativeBuildInputs ? [ ], description }:
    stdenv.mkDerivation {
      pname = "sermo-backend-${port}";
      inherit version src buildInputs;
      nativeBuildInputs = with pkgs; [ cmake flex bison pkg-config ] ++ nativeBuildInputs;
      hardeningDisable = [ "fortify" ];
      dontUseCmakeConfigure = true;
      buildPhase = ''
        runHook preBuild
        sh packaging/autres-os/construire-os.sh construire . "$PWD/construit" ${port}
        runHook postBuild
      '';
      installPhase = ''
        runHook preInstall
        sh packaging/autres-os/construire-os.sh installer . "$PWD/construit" ${port} "$out" /
        runHook postInstall
      '';
      meta = {
        inherit description;
        homepage = "https://gitlab.com/haplo-dialog/sermo";
        license = lib.licenses.gpl2Plus;
        mainProgram = "${port}sermo";
        platforms = lib.platforms.linux;
      };
    };

  backends = {
    "sermo-backend-gtk3" = mkBackend {
      port = "gtk3";
      buildInputs = [ pkgs.gtk3 pkgs.vte pkgs.gtk-layer-shell ];
      nativeBuildInputs = [ pkgs.wrapGAppsHook3 ];
      description = "sermo dialog boxes for shell scripts - GTK 3 backend (gtkdialog-compatible command)";
    };
    "sermo-backend-gtk4" = mkBackend {
      port = "gtk4";
      buildInputs = [ pkgs.gtk4 pkgs.vte-gtk4 ];
      nativeBuildInputs = [ pkgs.wrapGAppsHook4 ];
      description = "sermo dialog boxes for shell scripts - GTK 4 backend";
    };
    "sermo-backend-qt6" = mkBackend {
      port = "qt6";
      buildInputs = [ pkgs.qt6.qtbase ];
      nativeBuildInputs = [ pkgs.qt6.wrapQtAppsHook ];
      description = "sermo dialog boxes for shell scripts - Qt 6 backend";
    };
    "sermo-backend-fltk1" = mkBackend {
      port = "fltk1";
      buildInputs = [ pkgs.fltk14 ];
      nativeBuildInputs = [  ];
      description = "sermo dialog boxes for shell scripts - FLTK 1.4 backend";
    };
    "sermo-backend-efl1" = mkBackend {
      port = "efl1";
      buildInputs = [ pkgs.enlightenment.efl pkgs.glib ];
      nativeBuildInputs = [  ];
      description = "sermo dialog boxes for shell scripts - EFL (Elementary) backend";
    };
    "sermo-backend-sdl3" = mkBackend {
      port = "sdl3";
      buildInputs = [ pkgs.sdl3 pkgs.libGL pkgs.xorg.libX11 pkgs.libpng pkgs.gdk-pixbuf ];
      nativeBuildInputs = [  ];
      description = "sermo dialog boxes for shell scripts - SDL 3 backend, drawn with Dear ImGui";
    };
    "sermo-backend-lvgl" = mkBackend {
      port = "lvgl";
      buildInputs = [ pkgs.xorg.libX11 pkgs.libpng pkgs.wayland pkgs.libxkbcommon ];
      nativeBuildInputs = [ pkgs.wayland-scanner pkgs.wayland-protocols ];
      description = "sermo dialog boxes for shell scripts - LVGL backend for kiosks and embedded screens (LVGL embedded)";
    };
    "sermo-backend-ncurses" = mkBackend {
      port = "ncurses";
      buildInputs = [ pkgs.ncurses ];
      nativeBuildInputs = [  ];
      description = "sermo dialog boxes for shell scripts - terminal (ncurses) backend, for machines without a display";
    };
    "sermo-backend-html" = mkBackend {
      port = "html";
      buildInputs = [  ];
      nativeBuildInputs = [  ];
      description = "sermo dialog boxes for shell scripts - static HTML export backend (writes a page instead of a window)";
    };
  };

  core-dev = stdenv.mkDerivation {
    pname = "sermo-core-dev";
    inherit version src;
    nativeBuildInputs = with pkgs; [ cmake flex bison pkg-config ];
    hardeningDisable = [ "fortify" ];
    dontUseCmakeConfigure = true;
    buildPhase = ''
      sh packaging/autres-os/construire-os.sh coeur-dev . "$PWD/construit" "$PWD/sortie" / lib
    '';
    installPhase = ''
      cp -r sortie "$out"
    '';
    meta = {
      description = "sermo dialog engine - static core library and headers";
      license = lib.licenses.gpl2Plus;
    };
  };
in
backends // {
  "sermo-core-dev" = core-dev;
  all = pkgs.symlinkJoin {
    name = "sermo-${version}";
    paths = lib.attrValues backends;
  };
}
