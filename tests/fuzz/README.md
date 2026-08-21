# Fuzzing du parser XML — haplo-dialog

Le cœur parse de l'XML (lexer **flex** + grammaire **bison**) potentiellement
fourni par un tiers : c'est une **surface d'attaque**. On la fuzze en mode
**parse-only headless** — `<binaire> --file <f> --print-ir` (le parser tourne
sans construire de widget ni ouvrir de fenêtre).

## Lancer

```sh
# Fuzzer de repli (sans dépendance, runnable partout / CI)
./tests/fuzz/run_fuzz.sh gtk3sermo 60        # 60 s sur le binaire gtk3sermo
./tests/fuzz/run_fuzz.sh ./chemin/vers/binaire 120
```

- **Avec afl++** (recommandé pour une vraie campagne) : `run_fuzz.sh` bascule
  automatiquement sur `afl-fuzz -i tests/xml -o findings -- <bin> --file @@ --print-ir`.
  Pour l'instrumentation, recompiler le port avec `CC=afl-gcc CXX=afl-g++`.
- **Sans afl** : `fuzz.py` mute les 52 graines de `tests/xml/` (flips de bits,
  insertions, suppressions, injection de métacaractères/tokens) et détecte les
  **crashs** (signal) et **hangs** (timeout 5 s).

## Corpus & résultats

- **Graines** : `tests/xml/*.xml` (les cas de régression servent de corpus).
- **Trouvailles** : `tests/fuzz/findings/` (entrées fautives sauvegardées ;
  non versionnées). Chaque crash y est rejoué via
  `<bin> --file findings/crash_XXXX.xml --print-ir`.

## Intégration continue

Un job CI peut lancer une campagne courte de non-régression :
`./tests/fuzz/run_fuzz.sh <port> 120` (échoue si un crash est trouvé).
