/*
 * This file is part of the AzerothCore Project. See AUTHORS file for Copyright information
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

 /*
 GAMEOBJECT_TYPE_DOOR                   = 0,
 GAMEOBJECT_TYPE_BUTTON                 = 1,
 GAMEOBJECT_TYPE_QUESTGIVER             = 2,
 GAMEOBJECT_TYPE_CHEST                  = 3,
 GAMEOBJECT_TYPE_BINDER                 = 4,
 GAMEOBJECT_TYPE_GENERIC                = 5,
 GAMEOBJECT_TYPE_TRAP                   = 6,
 GAMEOBJECT_TYPE_CHAIR                  = 7,
 GAMEOBJECT_TYPE_SPELL_FOCUS            = 8,
 GAMEOBJECT_TYPE_TEXT                   = 9,
 GAMEOBJECT_TYPE_GOOBER                 = 10,
 GAMEOBJECT_TYPE_TRANSPORT              = 11,
 GAMEOBJECT_TYPE_AREADAMAGE             = 12,
 GAMEOBJECT_TYPE_CAMERA                 = 13,
 GAMEOBJECT_TYPE_MAP_OBJECT             = 14,
 GAMEOBJECT_TYPE_MO_TRANSPORT           = 15,
 GAMEOBJECT_TYPE_DUEL_ARBITER           = 16,
 GAMEOBJECT_TYPE_FISHINGNODE            = 17,
 GAMEOBJECT_TYPE_SUMMONING_RITUAL       = 18,
 GAMEOBJECT_TYPE_MAILBOX                = 19,
 GAMEOBJECT_TYPE_DO_NOT_USE             = 20,
 GAMEOBJECT_TYPE_GUARDPOST              = 21,
 GAMEOBJECT_TYPE_SPELLCASTER            = 22,
 GAMEOBJECT_TYPE_MEETINGSTONE           = 23,
 GAMEOBJECT_TYPE_FLAGSTAND              = 24,
 GAMEOBJECT_TYPE_FISHINGHOLE            = 25,
 GAMEOBJECT_TYPE_FLAGDROP               = 26,
 GAMEOBJECT_TYPE_MINI_GAME              = 27,
 GAMEOBJECT_TYPE_DO_NOT_USE_2           = 28,
 GAMEOBJECT_TYPE_CAPTURE_POINT          = 29,
 GAMEOBJECT_TYPE_AURA_GENERATOR         = 30,
 GAMEOBJECT_TYPE_DUNGEON_DIFFICULTY     = 31,
 GAMEOBJECT_TYPE_BARBER_CHAIR           = 32,
 GAMEOBJECT_TYPE_DESTRUCTIBLE_BUILDING  = 33,
 GAMEOBJECT_TYPE_GUILD_BANK             = 34,
 GAMEOBJECT_TYPE_TRAPDOOR               = 35
 */

#include "Log.h"
#include "ScriptMgr.h"
#include "Config.h"
#include "Chat.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "Item.h"
#include "DBCStores.h"
#include "GameObject.h"
#include "CellImpl.h"
#include "GridNotifiers.h"
#include "GridNotifiersImpl.h"
#include "CommandScript.h"

using namespace Acore::ChatCommands;

enum AutoLootString
{
    AOE_ACORE_STRING_MESSAGE = 50000,
    AOE_ITEM_IN_THE_MAIL = 50001
};

// The gathering branch: a mining vein or an herb node within reach is opened
// with the gathering spell of its lock. It used to know Mining only, so herbs
// were never picked (player report 2026-10-29).
static constexpr uint32 AOE_LOOT_SPELL_MINING = 2575;
static constexpr uint32 AOE_LOOT_SPELL_HERB_GATHERING = 2366;

// The gathering spell the player can open this node with, 0 for none (the
// lock's skill case decides; the node's required skill rank is checked by the
// spell itself)
static uint32 GatherSpellFor(Player* player, GameObject* go)
{
    LockEntry const* lock = sLockStore.LookupEntry(go->GetGOInfo()->GetLockId());
    if (!lock)
        return 0;

    for (uint8 i = 0; i < MAX_LOCK_CASE; ++i)
    {
        if (lock->Type[i] != LOCK_KEY_SKILL)
            continue;
        if (lock->Index[i] == LOCKTYPE_MINING && player->HasSkill(SKILL_MINING))
            return AOE_LOOT_SPELL_MINING;
        if (lock->Index[i] == LOCKTYPE_HERBALISM && player->HasSkill(SKILL_HERBALISM))
            return AOE_LOOT_SPELL_HERB_GATHERING;
    }
    return 0;
}

// The skinning ranks, highest first (each one is a Skinning spell with the
// skinning effect; the corpse's required skill is checked by the spell)
static constexpr uint32 AOE_LOOT_SKINNING_SPELLS[] = { 50305, 32678, 10768, 8618, 8617, 8613 };

static Item* StoreLootAndNotify(Player* player, uint32 itemId, uint32 count, ObjectGuid lootSource);

// Skin one of the player's own corpses and put the skinning loot in the bags
// (the Skinning spell fills the corpse's loot as a manual skin would; what
// does not fit stays in the corpse for a manual pick-up)
static void SkinCorpse(Player* player, Creature* creature)
{
    uint32 spellId = 0;
    for (uint32 rank : AOE_LOOT_SKINNING_SPELLS)
        if (player->HasSpell(rank))
        {
            spellId = rank;
            break;
        }
    if (!spellId)
        return;

    player->CastSpell(creature, spellId, true);
    if (creature->HasUnitFlag(UNIT_FLAG_SKINNABLE))
        return; // the spell refused (skill too low, no Skinning Knife)

    Loot* loot = &creature->loot;
    ObjectGuid const creatureGuid = creature->GetGUID();
    uint32 const maxSlot = loot->GetMaxSlotInLootFor(player);
    bool allStored = true;
    for (uint32 i = 0; i < maxSlot; ++i)
        if (LootItem* item = loot->LootItemInSlot(i, player))
        {
            if (StoreLootAndNotify(player, item->itemid, item->count, creatureGuid))
                player->SendNotifyLootItemRemoved(uint8(i));
            else
                allStored = false;
        }
    player->SendLootRelease(player->GetLootGUID());

    // all of it in the bags: the corpse is skinned out, as after a manual skin
    if (allStored)
    {
        loot->clear();
        creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
        creature->AllLootRemovedFromCorpse();
    }
}

// The nearest node within range that is ready and that this player can
// gather. Taking the nearest chest of any kind let a picked herb (or a chest
// the player cannot open) beside a vein hide the vein.
class NearestGatherableNodeCheck
{
public:
    NearestGatherableNodeCheck(Player* player, float range) : _player(player), _range(range) { }

    bool operator()(GameObject* go)
    {
        if (go->GetGoType() != GAMEOBJECT_TYPE_CHEST || go->getLootState() != GO_READY
            || go->IsInvisibleDueToDespawn() || !_player->IsWithinDistInMap(go, _range)
            || !GatherSpellFor(_player, go))
            return false;

        _range = _player->GetDistance(go);
        return true;
    }

private:
    Player* _player;
    float _range;
};

namespace
{
    // Cached at config load instead of read per tick. sConfigMgr->GetOption is not
    // a cheap lookup: it builds "AC_" + transformed key (string allocations) and
    // calls std::getenv() on every call, because a negative env-var result is
    // never cached (src/common/Configuration/Config.cpp). This hook runs ~100x/s
    // per player.
    bool   conf_Enable         = true;
    bool   conf_MailEnable     = true;
    uint32 conf_ScanIntervalMS = 250;
}

// Per-player throttle state. Kept on the Player itself rather than in a module
// map so there is no shared container to lock if MapUpdate.Threads is ever raised
// above 1.
struct AutoLootScanTimer : public DataMap::Base
{
    uint32 elapsedMs = 0;
};

// The player's own switch (the /fl hub's Settings, or .autoloot on|off), kept per
// character in character_settings. A "core." source is saved even with
// EnablePlayerSettings = 0. Index 0: 0 = on (the default, no row needed), 1 = off.
static constexpr char const* AUTO_LOOT_SETTING = "core.fl_auto_loot";
static constexpr uint32 AUTO_LOOT_SETTING_OFF = 1;

static bool IsAutoLootOff(Player const* player)
{
    PlayerSettingVector const* values = player->FindPlayerSettings(AUTO_LOOT_SETTING);
    return values && !values->empty() && (*values)[0].value == AUTO_LOOT_SETTING_OFF;
}

class AutoLoot_World : public WorldScript
{
public:
    AutoLoot_World() : WorldScript("AutoLoot_World") { }

    void OnAfterConfigLoad(bool /*reload*/) override
    {
        conf_Enable         = sConfigMgr->GetOption<bool>("AOELoot.Enable", true);
        conf_MailEnable     = sConfigMgr->GetOption<bool>("AOELoot.MailEnable", true);
        conf_ScanIntervalMS = sConfigMgr->GetOption<uint32>("AOELoot.ScanIntervalMS", 250);
    }
};

// Store item and fire OnPlayerLootItem so other modules (e.g. paragon-itemgen)
// can process the item.  Returns the created Item* or nullptr on failure.
static Item* StoreLootAndNotify(Player* player, uint32 itemId, uint32 count, ObjectGuid lootSource)
{
    ItemPosCountVec dest;
    if (player->CanStoreNewItem(NULL_BAG, NULL_SLOT, dest, itemId, count) != EQUIP_ERR_OK)
        return nullptr;

    Item* newItem = player->StoreNewItem(dest, itemId, true);
    if (!newItem)
        return nullptr;

    player->SendNewItem(newItem, count, true, false);
    sScriptMgr->OnPlayerLootItem(player, newItem, count, lootSource);
    return newItem;
}

class AutoLoot_Player : public PlayerScript
{
public:
    AutoLoot_Player() : PlayerScript("AutoLoot_Player", {PLAYERHOOK_ON_LOGIN, PLAYERHOOK_CAN_SEND_ERROR_ALREADY_LOOTED, PLAYERHOOK_ON_UPDATE}) { }

    void OnPlayerLogin(Player* player) override
    {
        if (!conf_Enable)
            return;

        if (IsAutoLootOff(player))
            ChatHandler(player->GetSession()).SendSysMessage("Auto-Loot is off. Turn it on with /fl (Settings) or .autoloot on.");
        else
            ChatHandler(player->GetSession()).PSendSysMessage(AOE_ACORE_STRING_MESSAGE);
    }

    bool OnPlayerCanSendErrorAlreadyLooted(Player* /*player*/) override
    {
        return true;
    }

    void OnPlayerUpdate(Player* player, uint32 p_time) override
    {
        if (!conf_Enable)
            return;

        // Throttle DISCOVERY only. Once the interval elapses the body below runs
        // exactly as it always did: same order, same authoritative effects, one
        // call each for every inventory grant, mail operation, item destruction,
        // storage mutation, Paragon write, hook, packet and corpse mutation.
        //
        // This hook is dispatched unthrottled from Player::Update, so with
        // MapUpdateInterval=10 it ran ~100x/s per player -- and each pass did TWO
        // grid searches (dead creatures below, and the chest search further down)
        // plus a full bag-slot walk in GetFreeInventorySpace(). At a 250 ms
        // cadence that is ~4/s instead of ~100/s per player, at the cost of an
        // imperceptible pickup delay well inside the 60 s corpse window.
        //
        // AOELoot.ScanIntervalMS = 0 restores the legacy every-tick behaviour
        // bit-for-bit.
        if (conf_ScanIntervalMS)
        {
            auto* scan = player->CustomData.GetDefault<AutoLootScanTimer>("mod-auto-loot");
            scan->elapsedMs += p_time;
            if (scan->elapsedMs < conf_ScanIntervalMS)
                return;

            scan->elapsedMs = 0;
        }

        // switched off by the player: nothing is picked up in passing, not
        // corpses, not skins, not herbs or ore
        if (IsAutoLootOff(player) || player->GetGroup() || player->GetFreeInventorySpace() < 4)
            return;

        float range = 10.0f;
        uint32 gold = 0;

        std::list<Creature*> creaturedie;
        player->GetDeadCreatureListInGrid(creaturedie, range);

        if (creaturedie.size()) {
            for (auto const& _creature : creaturedie)
            {
                // only the player's own kills: a corpse belongs to its loot
                // recipient (the core's tap), not to whoever runs past it
                // (player report 2026-10-29: another player's kills were
                // auto-looted away from him). Grouped players never get here.
                if (_creature->GetLootRecipient() != player)
                    continue;

                Loot* loot = &_creature->loot;
                gold += loot->gold;
                loot->gold = 0;
                uint8 lootSlot = 0;
                uint32 maxSlot = loot->GetMaxSlotInLootFor(player);

                ObjectGuid creatureGuid = _creature->GetGUID();
                for (uint32 i = 0; i < maxSlot; ++i)
                {
                    if (LootItem* item = loot->LootItemInSlot(i, player))
                    {
                        ItemTemplate const* itemTemplate = sObjectMgr->GetItemTemplate(item->itemid);

                        if (itemTemplate->MaxCount != 1)
                        {
                            if (StoreLootAndNotify(player, item->itemid, item->count, creatureGuid))
                            {
                                player->SendNotifyLootItemRemoved(lootSlot);
                                player->SendLootRelease(player->GetLootGUID());
                            }
                            // A quest drop that does not fit is surplus (the quest holds what it needs; the
                            // item's max count is the quest's count): mailing it only fills the mailbox
                            // (FL 2026-10-17: several corpses looted in one scan mailed the extra claws).
                            else if (conf_MailEnable && !item->needs_quest && itemTemplate->Class != ITEM_CLASS_QUEST)
                            {
                                player->SendItemRetrievalMail(item->itemid, item->count);
                                ChatHandler(player->GetSession()).SendSysMessage(AOE_ITEM_IN_THE_MAIL);
                            }
                        }
                        else
                        {
                            if (!player->HasItemCount(item->itemid, 1))
                            {
                                StoreLootAndNotify(player, item->itemid, item->count, creatureGuid);
                            }
                            player->SendNotifyLootItemRemoved(lootSlot);
                            player->SendLootRelease(player->GetLootGUID());
                        }
                    }
                }

                if (!loot->empty())
                {
                    if (!_creature->IsAlive())
                    {
                        _creature->AllLootRemovedFromCorpse();
                        _creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
                        loot->clear();

                        // a skinner skins it below; for anyone else the corpse is done
                        if (_creature->HasUnitFlag(UNIT_FLAG_SKINNABLE) && !player->HasSkill(SKILL_SKINNING))
                        {
                            _creature->RemoveUnitFlag(UNIT_FLAG_SKINNABLE);
                        }
                    }
                }
                else
                {
                    _creature->RemoveDynamicFlag(UNIT_DYNFLAG_LOOTABLE);
                    _creature->AllLootRemovedFromCorpse();
                    // Loot::empty() looks at the shared items and gold only, not at the per-player quest
                    // items: a corpse whose loot is only a quest item landed here with that item still in
                    // it, and every scan (250 ms) handed it out again until the corpse decayed (FL
                    // 2026-10-17: one Ancient Spirit Bear gave nine claws). Clear it like the branch above.
                    if (!_creature->IsAlive())
                        loot->clear();
                }

                // Skinning in passing, as mining and herbs (operator 2026-10-29: "automatisch"; before, the
                // branch above stripped the skinnable flag and skinners got no leather at all)
                if (!_creature->IsAlive() && _creature->HasUnitFlag(UNIT_FLAG_SKINNABLE)
                    && player->HasSkill(SKILL_SKINNING))
                    SkinCorpse(player, _creature);
            }

            player->ModifyMoney(gold);
            player->UpdateAchievementCriteria(ACHIEVEMENT_CRITERIA_TYPE_LOOT_MONEY, gold);
            WorldPacket data(SMSG_LOOT_MONEY_NOTIFY, 4 + 1);
            data << uint32(gold);
            data << uint8(1);
            player->GetSession()->SendPacket(&data);
        }

        // The chest branch: gate on the skill BEFORE searching. The GameObject grid
        // search used to run for every player on every tick even though only a
        // character with this skill can act on the result, so non-gatherers paid a
        // full grid search for nothing. Observable behaviour is unchanged -- the
        // search result was only ever used inside this same skill check.
        if (!player->HasSkill(SKILL_MINING) && !player->HasSkill(SKILL_HERBALISM))
            return;

        GameObject* myObj = nullptr;
        NearestGatherableNodeCheck check(player, 10.f);
        Acore::GameObjectLastSearcher<NearestGatherableNodeCheck> searcher(player, myObj, check);
        Cell::VisitObjects(player, searcher, 10.f);
        if (myObj)
        {
            uint32 const gatherSpell = GatherSpellFor(player, myObj);
            {
                player->CastSpell(myObj, gatherSpell, true);
                Loot* loot = &myObj->loot;
                uint8 lootSlot = 0;
                uint32 maxSlot = loot->GetMaxSlotInLootFor(player);

                ObjectGuid objGuid = myObj->GetGUID();
                for (uint32 i = 0; i < maxSlot; ++i)
                {
                    if (LootItem* item = loot->LootItemInSlot(i, player))
                    {
                        uint32 itemcount = item->count;
                        if (StoreLootAndNotify(player, item->itemid, itemcount, objGuid))
                        {
                            player->SendNotifyLootItemRemoved(lootSlot);
                            player->SendLootRelease(player->GetLootGUID());
                            myObj->DespawnOrUnsummon();
                        }
                    }
                }
            }
        }


    }

};

// .autoloot          shows the player's switch
// .autoloot on|off   sets it (the /fl hub's Settings buttons send these)
class AutoLoot_Command : public CommandScript
{
public:
    AutoLoot_Command() : CommandScript("AutoLoot_Command") { }

    ChatCommandTable GetCommands() const override
    {
        static ChatCommandTable autoLootTable =
        {
            { "on",  HandleOn,   SEC_PLAYER, Console::No },
            { "off", HandleOff,  SEC_PLAYER, Console::No },
            { "",    HandleShow, SEC_PLAYER, Console::No },
        };

        static ChatCommandTable commandTable =
        {
            { "autoloot", autoLootTable },
        };

        return commandTable;
    }

    static void SendState(ChatHandler* handler, Player* player)
    {
        if (!conf_Enable)
            handler->SendSysMessage("Auto-Loot is disabled on this server.");
        else if (IsAutoLootOff(player))
            handler->SendSysMessage("Auto-Loot is off: loot, skin and gather by hand.");
        else
            handler->SendSysMessage("Auto-Loot is on: your own kills are looted and skinned, herbs and ore within 10 yards are gathered (not in a group).");
    }

    static bool HandleOn(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        player->UpdatePlayerSetting(AUTO_LOOT_SETTING, 0, 0);
        SendState(handler, player);
        return true;
    }

    static bool HandleOff(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        player->UpdatePlayerSetting(AUTO_LOOT_SETTING, 0, AUTO_LOOT_SETTING_OFF);
        SendState(handler, player);
        return true;
    }

    static bool HandleShow(ChatHandler* handler)
    {
        Player* player = handler->GetPlayer();
        if (!player)
            return false;

        SendState(handler, player);
        return true;
    }
};

void AddSC_AutoLoot()
{
    new AutoLoot_World();
    new AutoLoot_Player();
    new AutoLoot_Command();
}
