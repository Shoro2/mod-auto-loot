# Funktionen & Mechaniken — mod-auto-loot

> Detaillierte Funktions- und Mechanik-Referenz. Für reinen Inhalt (was tut das Modul, IDs, DB-Bezüge) siehe `CLAUDE.md`.

## Modul-Loader

### `AddSC_AutoLoot()`
- **Datei**: `src/mod_auto_loot_loader.cpp`
- **Aufrufer**: `Addmod_auto_lootScripts()` (vom AzerothCore-Modul-System generiert)
- **Wirkung**: erzeugt eine globale `new AutoLoot_Player()` Instanz, die sich beim `ScriptMgr` registriert.

## Hauptklasse: `AutoLoot_Player : public PlayerScript`

Konstruktor registriert exakt drei Hooks via Constructor-Init-Liste:
```cpp
PlayerScript("AutoLoot_Player",
  {PLAYERHOOK_ON_LOGIN,
   PLAYERHOOK_CAN_SEND_ERROR_ALREADY_LOOTED,
   PLAYERHOOK_ON_UPDATE})
```

### Hook 1 — `OnPlayerLogin(Player*)`
Sendet Welcome-Message via `ChatHandler::PSendSysMessage(AOE_ACORE_STRING_MESSAGE)` (= acore_string `50000`), wenn `AOELoot.Enable=true`. Reine UX, keine Logik.

### Hook 2 — `OnPlayerCanSendErrorAlreadyLooted(Player*) → bool`
Returnt immer `true`. Damit wird der "Already Looted"-Fehler unterdrückt, der sonst entstehen würde, wenn der Auto-Loot eine bereits geleerte Corpse-Instanz nochmal anfasst.

### Hook 3 — `OnPlayerUpdate(Player*, uint32 p_time)` *(Hauptlogik)*

Frame-Tick. Bricht früh ab, wenn:
- Spieler in einer Gruppe ist (kein Auto-Loot in Group, um Loot-Rolls nicht zu umgehen),
- `AOELoot.Enable=false`,
- weniger als **4 freie Inventarslots**.

Andernfalls:

#### a) Creature-Loot
1. `GetDeadCreatureListInGrid(creaturedie, range=10.0f)` — alle toten Creatures im 10-Yard-Radius.
2. Für jede Creature:
   - `Loot* loot = &creature->loot;`
   - Gold akkumulieren in lokales `gold` (siehe Schritt 3).
   - `loot->gold = 0;` damit nicht doppelt.
   - Für jeden Slot 0..maxSlot:
     - `LootItem* item = loot->LootItemInSlot(i, player)`
     - **Stackable Items** (`itemTemplate->MaxCount != 1`):
       - `StoreLootAndNotify(player, item->itemid, item->count, creatureGuid)` versuchen.
       - Bei Erfolg: `SendNotifyLootItemRemoved(slot)` + `SendLootRelease(GetLootGUID())`.
       - Bei Fail (volles Inventar) und `AOELoot.MailEnable=true`: `SendItemRetrievalMail(itemid, count)` + Chat-Hinweis (acore_string `50001`).
     - **Unique Items** (`MaxCount == 1`):
       - Nur looten, wenn `!HasItemCount(itemid, 1)` (Spieler besitzt es noch nicht).
       - In jedem Fall LootSlot freigeben und Loot-Release senden.
   - Nach der Iteration: wenn `loot` nicht leer (verbliebene Items, die wegen Limit nicht gelootet wurden) und Creature tot:
     - `AllLootRemovedFromCorpse()`, `RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE)`, `loot->clear()`, `RemoveUnitFlag(UNIT_FLAG_SKINNABLE)` falls gesetzt.
   - Wenn Loot leer: nur Flags entfernen.
3. Gold an den Spieler:
   - `ModifyMoney(gold)`, `UpdateAchievementCriteria(LOOT_MONEY, gold)`.
   - `SMSG_LOOT_MONEY_NOTIFY` senden mit `data << uint32(gold) << uint8(1)`.

#### b) Chest/GameObject-Loot
1. `FindNearestGameObjectOfType(GAMEOBJECT_TYPE_CHEST, 10.0f)`.
2. Nur weiter, wenn Spieler `HasSkill(186)` (= Lockpicking).
3. Truhe muss `GO_READY` sein und nicht despawned.
4. `CastSpell(myObj, 2575, true)` — Spell 2575 = "Pick Lock", ohne Cast-Time.
5. Slots iterieren, jedes Item via `StoreLootAndNotify`. Bei Erfolg: `myObj->DespawnOrUnsummon()`.

> Hinweis: Nur Chests, keine Mining/Herbalism Nodes — die liefen früher über `OnAfterGObjLoot`, der entfernt wurde (siehe `log.md`).

## Helper-Funktion: `StoreLootAndNotify`

```cpp
static Item* StoreLootAndNotify(Player*, uint32 itemId, uint32 count, ObjectGuid lootSource)
```
- `CanStoreNewItem` prüfen → bei Fail `nullptr`.
- `StoreNewItem` → `SendNewItem` → **`sScriptMgr->OnPlayerLootItem(player, item, count, lootSource)`**.
- **Wichtig**: Der Hook-Aufruf ist der ganze Sinn dieser Helper-Funktion. Frühere Versionen nutzten `player->AddItem()` direkt, was den Hook umging und mod-paragon-itemgen die Item-Stats nicht aufprägen ließ.

## Konfigurations-Optionen (`mod_auto_loot.conf.dist`)

| Schlüssel | Default | Wirkung |
|-----------|---------|---------|
| `AOELoot.Enable` | `true` | Modul global aktivieren |
| `AOELoot.MailEnable` | `true` | Bei vollem Inventar Items per Mail versenden |

## Konstanten

```cpp
enum AutoLootString {
    AOE_ACORE_STRING_MESSAGE = 50000,   // Login-Welcome Text
    AOE_ITEM_IN_THE_MAIL     = 50001    // "Item in der Post" Hinweis
};
```

Beide IDs verweisen auf Einträge in der `acore_string`-Tabelle (`acore_world`-DB).

## Spell-/Skill-Referenzen

| ID | Verwendung |
|----|-----------|
| Spell `2575` | "Pick Lock" — wird auf Chests gecastet, um sie zu öffnen |
| Skill `186` | "Lockpicking" — Voraussetzung für Chest-Loot |

## Performance-Anmerkungen

- `OnPlayerUpdate` läuft **jeden Frame**. Die Frühaussteiger (Group / Disabled / Inventory full) verhindern Logik bei den meisten Spielern.
- `GetDeadCreatureListInGrid` ist O(Cells × Creatures) — kann bei großen AOE-Pulls (Festungs-Wipes) teuer werden. Range fest auf 10.0f.
- Es existiert **keine Cooldown-/Throttle-Mechanik** zwischen den Auto-Loot-Pässen. Wenn das je zum Problem wird, wäre ein TimeOf-Last-Run-Check pro Spieler die offensichtliche Erweiterung.

## Bekannte Einschränkungen

- **Kein Group-Support** — Spieler in Gruppe werden komplett übersprungen, weil das Modul keine Loot-Rolls unterstützt.
- **Kein Mining/Herbalism** mehr (siehe Hinweis oben).
- **Kein Skinning** integriert — Skinnable-Flag wird sogar entfernt, falls die Corpse leer ist.
- **Kein Reagent-Routing in Endless Storage** — wer mod-endless-storage einsetzt, bekommt Items ins Inventar, nicht direkt ins Storage. Das wäre ein Folge-Feature.
