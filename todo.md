# TODOs — mod-auto-loot

> Offene Aufgaben für dieses Modul. Erledigte TODOs in `log.md` festhalten und hier entfernen.
> Format: `[ ]` offen, `[x]` erledigt (vor Cleanup), Priorität in Klammern.

## Funktionale Lücken

- [ ] **(niedrig)** Mining-/Herbalism-Auto-Loot: vor 2026-03-06 lief Node-Loot über `OnAfterGObjLoot`, der Hook wurde aus AzerothCore entfernt. Workaround müsste über aktiven `CastSpell`-Pfad pro Skill gebaut werden — derzeit nicht implementiert.
- [ ] **(niedrig)** Skinning-Integration: Skinnable-Flag wird beim Aufräumen sogar entfernt. Echte Skinning-Automation würde `HasSkill(393)` + `CastSpell(Skinning)` + Loot-Run benötigen.
- [ ] **(niedrig)** Reagent-Routing in mod-endless-storage: looted Trade-Goods/Gems landen aktuell im Inventar, nicht direkt im Storage. Ein optionaler Pfad könnte direkt in `custom_endless_storage` schreiben — sinnvoll, wenn der Spieler beides aktiv nutzt.

## Performance / Robustheit

- [ ] **(niedrig)** Throttle / Cooldown zwischen Auto-Loot-Pässen: aktuell pro Frame-Tick — bei großen AOE-Pulls in vollen Festungen potenziell teuer. Ein einfacher per-Player `lastRunMs`-Check könnte das halbieren.

## Dokumentation

- [ ] keine offenen Punkte.

## Konvention

Erledigte Items NICHT durchstreichen — sondern aus dieser Datei entfernen und in `log.md` als Commit-Eintrag dokumentieren.
