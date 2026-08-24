<div align="right"><sub><code>maj :     2026-08-20 20:34:10 · par claude</code></sub></div>
<div align="right"><sub><code>réindex : —</code></sub></div>

# TODO Sécurité — gtk4sermo (gtk4sermo)

**Haplo-Linux — 2026**

## Points de sécurité résolus ✅

- [x] `safe_exec()` — remplace `system()`, parse argv sans shell
- [x] `safe_popen()` — pipe sécurisé, fermeture via `fclose()`
- [x] `-D_FORTIFY_SOURCE=3` — détection overflow compile+runtime
- [x] `-fstack-protector-strong` — canary stack
- [x] `-fstack-clash-protection` — protection stack clash
- [x] `-fcf-protection=full` — CFI hardware (x86 CET)
- [x] PIE (`-fPIE -pie`) — ASLR
- [x] Full RELRO (`-Wl,-z,relro -Wl,-z,now`) — GOT read-only
- [x] NX stack (`-Wl,-z,noexecstack`) — pile non exécutable
- [x] `-Wformat=2 -Wformat-overflow=2 -Werror=format-security`
- [x] `-Wshadow -Wnull-dereference -Wimplicit-fallthrough=3`
- [x] Validation longueur commande dans `safe_exec()`/`safe_popen()`

## Points restants à traiter ⏳

- [x] Sûreté mémoire de variables.c — **fait le 2026-08-24**. Il n'y avait ni
      `strcat()` ni `strcpy()` (l'item était périmé) mais un
      `strncpy(new->Name, name, NAMELEN)` dans un tampon `g_malloc` non mis à
      zéro : un nom de 512 caractères ou plus n'était pas terminé
      (CWE-170, puis CWE-125 à la première lecture). Remplacé par
      `g_strlcpy(new->Name, name, sizeof(new->Name))`, comme le port GTK 3.
- [x] Borne de `_sum()` — **fait le 2026-08-24**. La recopie des widgets d'un
      conteneur écrivait dans `widgets[]`/`widgettypes[]`, tableaux fixes de
      MAXWIDGETS, sans vérifier la place (CWE-787). Mesuré avant correctif :
      300 enfants directs étaient acceptés en silence. Désormais refus net,
      même message que le port GTK 3. Test : `tests/garde_maxwidgets.sh`.
- [x] `actions.c` — **fait le 2026-08-24** : `action_append()` recopiait le
      premier paramètre depuis le DÉBUT de la chaîne au lieu de la position
      de la correspondance (`string` au lieu de `string + matched[1].rm_so`).
      Le port GTK 3 n'avait pas ce défaut.
- [ ] Auditer `tag_attributes.c` — snprintf systématique (pas sprintf)
- [ ] Valider les chemins de fichiers passés via XML (traversal path)
- [ ] Ajouter fuzzing (AFL++ ou libFuzzer) sur le parser XML
- [ ] Activer AddressSanitizer en mode debug (`--enable-debug --enable-asan`)
- [ ] Vérifier que pkexec est utilisé pour toute élévation (jamais sudo GUI)
- [ ] Ajouter `seccomp` en option de compilation (sandboxing syscalls)
- [ ] Auditer `actions.c` — CMD_INPUT : liste blanche stricte
- [ ] CI : intégrer `hardening-check` dans le pipeline de packaging

---

*Document horodaté — dernière mise à jour : 2026-06-07 (audit #3 — haplo-dialog v1.0.0).*
