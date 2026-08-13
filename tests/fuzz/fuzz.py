#!/usr/bin/env python3
"""Fuzzer de repli (sans afl) du parser XML haplo-dialog.

Le parser (lexer flex + grammaire bison) traite de l'XML potentiellement non
fiable : c'est une surface d'attaque. Ce fuzzer mute des graines XML et les
passe au binaire en mode **parse-only headless** (`--file <f> --print-ir`,
ni widgets ni serveur X), puis détecte :
  - les crashs   : processus tué par un signal (SIGSEGV/SIGABRT…) ;
  - les hangs    : dépassement du délai (boucle infinie).
Les entrées fautives sont sauvegardées dans tests/fuzz/findings/.

Pour du fuzzing sérieux, préférer afl++ (voir run_fuzz.sh). Ce repli sert
quand afl n'est pas disponible (CI légère, poste sans instrumentation).

Usage : fuzz.py <binaire> [secondes] [corpus_dir]
Sortie : exit 1 si au moins un crash/hang est trouvé, 0 sinon.
"""
import os, sys, random, subprocess, time, glob, tempfile, shutil

META = [b"<", b">", b"&", b'"', b"'", b"</window>", b"<window ", b"<vbox>",
        b"<variable>", b"%s", b"%n", b"\x00", b"\xff\xfe", b"=", b"<!--", b"-->"]


def mutate(data: bytes) -> bytes:
    d = bytearray(data) if data else bytearray(b"<window></window>")
    for _ in range(random.randint(1, 8)):
        if not d:
            d = bytearray(b"<window></window>")
        r = random.random()
        if r < 0.40:                                   # flip de bit
            i = random.randrange(len(d)); d[i] ^= 1 << random.randint(0, 7)
        elif r < 0.58:                                 # insertion d'octet
            i = random.randrange(len(d) + 1); d.insert(i, random.randint(0, 255))
        elif r < 0.72:                                 # suppression
            del d[random.randrange(len(d))]
        elif r < 0.86:                                 # duplication de bloc
            i = random.randrange(len(d)); n = random.randint(1, max(1, len(d) // 4))
            d[i:i] = d[i:i + n]
        else:                                          # injection d'un token/metachar
            tok = random.choice(META)
            i = random.randrange(len(d) + 1); d[i:i] = tok
    return bytes(d)


def main():
    if len(sys.argv) < 2:
        sys.exit("usage: fuzz.py <binaire> [secondes] [corpus_dir]")
    binary = sys.argv[1]
    dur = int(sys.argv[2]) if len(sys.argv) > 2 else 30
    here = os.path.dirname(os.path.abspath(__file__))
    corpus = sys.argv[3] if len(sys.argv) > 3 else os.path.join(here, "..", "xml")
    findings = os.path.join(here, "findings")
    os.makedirs(findings, exist_ok=True)

    seeds = []
    for f in glob.glob(os.path.join(corpus, "*.xml")):
        try:
            seeds.append(open(f, "rb").read())
        except OSError:
            pass
    if not seeds:
        seeds = [b"<window><vbox><button ok></button></vbox></window>"]

    print(f"Fuzzing {binary} — {dur}s, {len(seeds)} graines (corpus: {corpus})")
    env = {**os.environ, "DISPLAY": ""}
    execs = crashes = 0
    t0 = time.time()
    while time.time() - t0 < dur:
        data = mutate(random.choice(seeds))
        with tempfile.NamedTemporaryFile(suffix=".xml", delete=False) as tf:
            tf.write(data); path = tf.name
        rc = None
        try:
            rc = subprocess.run([binary, "--file", path, "--print-ir"],
                                stdout=subprocess.DEVNULL, stderr=subprocess.DEVNULL,
                                timeout=5, env=env).returncode
            crash = rc < 0 or rc >= 128            # tué par un signal
        except subprocess.TimeoutExpired:
            crash, rc = True, "timeout"
        execs += 1
        if crash:
            crashes += 1
            dst = os.path.join(findings, f"crash_{crashes:04d}_{rc}.xml")
            shutil.copy(path, dst)
            print(f"  !! CRASH rc={rc} → {dst}")
        os.unlink(path)

    dt = time.time() - t0 or 1
    print(f"\n{execs} exécutions en {dt:.0f}s ({execs / dt:.0f}/s) — {crashes} crash(s)/hang(s)")
    sys.exit(1 if crashes else 0)


if __name__ == "__main__":
    main()
