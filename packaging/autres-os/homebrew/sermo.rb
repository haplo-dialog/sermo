# SPDX-License-Identifier: GPL-2.0-or-later
#
# Formule Homebrew de sermo : le port ncurses seul (ncurses de Homebrew).
# La construction est celle de packaging/autres-os/construire-os.sh.
# La somme (sha256) se pose à la publication de l'étiquette.
class Sermo < Formula
  desc "Dialog boxes for shell scripts, described in XML (ncurses port)"
  homepage "https://gitlab.com/haplo-dialog/sermo"
  url "https://gitlab.com/haplo-dialog/sermo/-/archive/v3.0.0/sermo-v3.0.0.tar.gz"
  sha256 "0000000000000000000000000000000000000000000000000000000000000000"
  license "GPL-2.0-or-later"

  depends_on "bison" => :build
  depends_on "cmake" => :build
  depends_on "flex" => :build
  depends_on "pkgconf" => :build
  depends_on "ncurses"

  def install
    # bison et flex de macOS sont trop anciens : ceux de Homebrew d'abord.
    ENV.prepend_path "PATH", Formula["bison"].opt_bin
    ENV.prepend_path "PATH", Formula["flex"].opt_bin
    ENV.prepend_path "PKG_CONFIG_PATH", Formula["ncurses"].opt_lib/"pkgconfig"
    system "sh", "packaging/autres-os/construire-os.sh", "construire", ".", "construit", "ncurses"
    system "sh", "packaging/autres-os/construire-os.sh", "installer", ".", "construit", "ncurses", "/", prefix
    bin.install_symlink "ncursessermo" => "sermo"
  end

  test do
    assert_match version.to_s, shell_output("#{bin}/ncursessermo --version")
    (testpath/"essai.xml").write <<~XML
      <window><vbox>
        <entry><default>essai</default><variable>V</variable></entry>
        <entry><variable>T</variable><input>echo aaa | tr a b</input></entry>
        <timer interval="1"><action>exit:OK</action></timer>
      </vbox></window>
    XML
    sortie = shell_output("SERMO_NCURSES_BATCH=1 #{bin}/sermo --file=#{testpath}/essai.xml < /dev/null")
    assert_match 'V="essai"', sortie
    assert_match 'T="bbb"', sortie
  end
end
