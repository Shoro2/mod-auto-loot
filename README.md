# mod-auto-loot

Automatic AOE looting for an [AzerothCore](https://www.azerothcore.org/) **WoW 3.3.5a (WotLK)** server.

## What it does

Empties dead creatures (and, with Lockpicking skill, chests) within a 10-yard radius of the player automatically — no clicking required. Gold from all corpses in range is summed and credited in a single message; items are placed in the inventory; full inventories can optionally forward overflow to the player's mailbox.

The module is intentionally minimal (about 250 lines of C++, no DB schema). It hooks the regular `OnPlayerUpdate` tick and operates only when a few simple preconditions are met.

## Key features

- **AOE loot** in a 10-yard radius around the player
- **Chest support** when the player has skill 186 (Lockpicking)
- **Gold aggregation**: all gold in range is summed and sent in one `SMSG_LOOT_MONEY_NOTIFY` packet, with the `LOOT_MONEY` achievement counted along
- **Smart unique-item handling**: items with `MaxCount = 1` are only looted if the player does not already own the copy
- **Mail overflow** (optional): when the inventory is full, looted stackable items can be sent on by mail (`AOELoot.MailEnable`)
- **Solo-only by default**: skipped while the player is in a group (group loot rules stay intact)
- **No DB tables, no DBC patches, no client addon required**

## Trigger conditions

Auto-loot fires on the player's update tick when **all** of the following are true:

| Condition | Required |
|-----------|----------|
| `AOELoot.Enable` is `true` in the config | yes |
| Player is **not** in a group | yes |
| At least 4 free inventory slots | yes |
| Dead creatures in the 10-yd radius | for creature loot |
| Chest (`GAMEOBJECT_TYPE_CHEST`) in radius **and** player has skill 186 (Lockpicking) | for chest loot |

## Installation

1. Place this module inside the AzerothCore `modules/` directory:
   ```bash
   cd azerothcore-wotlk/modules
   git clone https://github.com/Shoro2/mod-auto-loot.git
   ```
2. Re-run CMake and build the server:
   ```bash
   cd ../build
   cmake .. -DCMAKE_INSTALL_PREFIX=$HOME/azeroth-server \
            -DCMAKE_BUILD_TYPE=RelWithDebInfo \
            -DSCRIPTS=static -DMODULES=static
   make -j$(nproc) && make install
   ```
3. Copy the config and adjust if needed:
   ```bash
   cp $HOME/azeroth-server/etc/mod_auto_loot.conf.dist $HOME/azeroth-server/etc/mod_auto_loot.conf
   ```
4. Restart the world server. The module starts working immediately for solo players.

## Configuration

`conf/mod_auto_loot.conf.dist` (excerpt):

- `AOELoot.Enable` — master toggle
- `AOELoot.Range` — radius in yards (default 10)
- `AOELoot.MailEnable` — forward overflow stacks to the player's mailbox when the inventory is full

## Requirements

- [AzerothCore](https://github.com/azerothcore/azerothcore-wotlk) (WoW 3.3.5a / WotLK)

No client addon, no SQL migration, no DBC patch needed.

## Project context

Auto-looted items pass through the standard `OnPlayerLootItem` hook, so they integrate seamlessly with [mod-paragon-itemgen](https://github.com/Shoro2/mod-paragon-itemgen) (bonus stats and cursed marker) and [mod-loot-filter](https://github.com/Shoro2/mod-loot-filter) (auto-sell / disenchant / delete rules).

## License

GPL v2 (see `LICENSE`).
