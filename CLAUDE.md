# CLAUDE.md — mod-auto-loot

> **Inhaltsorientiert**. Was ist dieses Modul, was tut es, welche IDs/DB-Bezüge gibt es?
> Mechanik-Details (Hooks, Funktions-Signaturen, Flow): siehe [`functions.md`](./functions.md).
> Datei-Layout: siehe [`data_structure.md`](./data_structure.md).
> Commit-Historie: siehe [`log.md`](./log.md).
> Projekt-Gesamtkontext: siehe [`share-public/AI_GUIDE.md`](https://github.com/Shoro2/share-public/blob/main/AI_GUIDE.md).

## Was ist mod-auto-loot?

Ein AzerothCore-Modul, das **AOE-Looting** im 10-Yard-Radius automatisiert. Tote Creatures und (mit Lockpicking-Skill) Truhen im Umkreis werden beim regulären `OnPlayerUpdate`-Tick automatisch geleert, ohne dass der Spieler jede Leiche einzeln anklicken muss. Volle Inventare können optional per Mail nachgesendet werden.

Das Modul ist klein (eine `.cpp`-Datei mit ~250 Zeilen Hauptlogik) und hat **keinerlei DB-Schema**.

## Rolle im Gesamtprojekt

```
Creature Death / Chest Open
        │
        ▼
mod-auto-loot   (greift Loot proaktiv im 10-yd-Radius auf)
        │
        ▼ sScriptMgr->OnPlayerLootItem()
        │
        ├─→ mod-paragon-itemgen  (vergibt Bonus-Stats / Cursed-Marker)
        └─→ mod-loot-filter      (entscheidet: Keep / Sell / DE / Delete)
```

Das Modul ist **Voraussetzung** für mod-paragon-itemgen + mod-loot-filter, sobald der Spieler "automatisch lootet" — ohne den `OnPlayerLootItem`-Hook würden auto-gelootete Items keine Stats bekommen und nicht gefiltert werden.

## Was triggert Auto-Loot?

| Bedingung | erforderlich |
|-----------|-------------|
| `AOELoot.Enable` Config | `true` |
| Spieler nicht in Gruppe | ja |
| ≥4 freie Inventarslots | ja |
| Tote Creatures im 10-yd-Radius | für Creature-Loot |
| Truhe (`GAMEOBJECT_TYPE_CHEST`) im 10-yd-Radius **und** Spieler hat Skill 186 (Lockpicking) | für Chest-Loot |

## Item-Behandlung

| Item-Typ (`MaxCount`) | Verhalten |
|-----------------------|-----------|
| stackable (`MaxCount != 1`) | wird gelootet; bei vollem Inventar via Mail nachgesendet (`AOELoot.MailEnable`) |
| unique (`MaxCount == 1`) | nur gelootet, wenn Spieler das Item **noch nicht besitzt** |

Gold wird über alle Creatures akkumuliert und in einem einzigen `SMSG_LOOT_MONEY_NOTIFY`-Packet an den Client geschickt. `LOOT_MONEY`-Achievement wird mitgezählt.

## IDs und konstante Werte

| Ressource | ID | Quelle |
|-----------|----|--------|
| acore_string "Auto-Loot enabled" | `50000` | `AOE_ACORE_STRING_MESSAGE` |
| acore_string "Item in mail" | `50001` | `AOE_ITEM_IN_THE_MAIL` |
| Spell "Pick Lock" | `2575` | Standard-WoW-Spell, von uns nur referenziert |
| Skill "Lockpicking" | `186` | Standard-WoW-Skill |
| Loot-Range | `10.0f` Yards | hartkodiert |
| Min freie Slots | `4` | hartkodiert |

## DB-Bezüge

- **Keine eigenen DB-Tabellen.**
- Konsumiert nur:
  - `acore_world.acore_string` (für die zwei Sysmessages 50000/50001)
  - `acore_world.item_template` (über `sObjectMgr->GetItemTemplate(itemid)` zur `MaxCount`-Prüfung)

## Konfiguration

`conf/mod_auto_loot.conf.dist` (zwei Optionen):

```ini
AOELoot.Enable = 1     # Master-Toggle
AOELoot.MailEnable = 1 # bei vollem Inventar nachsenden
```

## Was das Modul NICHT tut

- Kein Group-Loot, keine Master-Loot-Logik, keine Loot-Rolls.
- Kein Mining-/Herbalism-Auto-Loot (`OnAfterGObjLoot`-Hook entfernt).
- Kein Skinning. Skinnable-Flag wird sogar entfernt, sobald die Leiche leer ist.
- Kein eigener Loot-Filter / kein Anti-Spam — die Filterung übernimmt mod-loot-filter im nachgelagerten Hook.

## Für KI-Sessions wichtig

- Keine SQL-Migrationen → Änderungen sind reine C++-Refactorings.
- Modul-Loader-Funktion: `AddSC_AutoLoot()` (entgegen Konvention `AddSC_*` statt `Addmod_auto_lootScripts()` — Funktioniert, weil das AzerothCore-Loader-System beide Patterns akzeptiert).
- Hook-Registrierung muss korrekte `PLAYERHOOK_*`-Enum-Werte verwenden (siehe `log.md`-Eintrag 2026-03-06).
- Der `StoreLootAndNotify`-Helper darf nicht durch `player->AddItem()` ersetzt werden — der `OnPlayerLootItem`-Hook ist die Schnittstelle zu allen anderen Modulen.
