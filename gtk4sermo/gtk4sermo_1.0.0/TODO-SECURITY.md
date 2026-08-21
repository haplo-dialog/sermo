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

- [ ] Remplacer `strcat()`/`strcpy()` par `g_strlcat()`/`g_strlcpy()` dans variables.c
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
