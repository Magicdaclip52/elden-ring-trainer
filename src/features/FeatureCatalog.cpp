#include "features/FeatureCatalog.h"

#include <Windows.h>

namespace er {
namespace {

void tuneCatalog(std::vector<FeatureInfo>& k) {
    auto find = [&](const char* id) -> FeatureInfo* {
        for (auto& f : k) {
            if (f.id == id) {
                return &f;
            }
        }
        return nullptr;
    };
    auto mark = [&](const char* id, bool risky, bool inject, float fv = 0, int iv = 0) {
        if (auto* f = find(id)) {
            f->risky = risky;
            f->needsInject = inject;
            if (fv != 0) {
                f->defaultFloat = fv;
            }
            if (iv != 0) {
                f->defaultInt = iv;
            }
        }
    };

    mark("god_mode", false, true);
    mark("infinite_hp", false, true);
    mark("infinite_fp", false, true);
    mark("infinite_stamina", false, true);
    mark("zero_weight", false, true);
    mark("infinite_item_usage", true, true);
    mark("drop_rate_100", false, true);
    mark("stealth_mode", true, true);
    mark("immune_status", false, true);
    mark("one_hit_kill", false, true);
    mark("damage_multiplier", false, true, 2.0f);
    mark("defense_multiplier", false, true, 2.0f);
    mark("infinite_horse_hp", true, true);

    mark("edit_runes", false, true, 0, 99999999);
    mark("runes_multiplier", false, true, 2.0f);
    mark("keep_runes_on_death", false, true);
    mark("freeze_daytime", false, true);
    mark("daytime_plus_1h", false, true);
    mark("game_speed", false, true, 2.5f);
    mark("fly_mode", true, true);
    mark("fly_up", false, true);
    mark("fly_down", false, true);
    mark("freeze_enemies", false, true);

    mark("edit_level", false, true, 0, 1);
    mark("edit_vigor", false, true, 0, 99);
    mark("edit_mind", false, true, 0, 99);
    mark("edit_endurance", false, true, 0, 99);
    mark("edit_strength", false, true, 0, 99);
    mark("edit_dexterity", false, true, 0, 99);
    mark("edit_intelligence", false, true, 0, 99);
    mark("edit_faith", false, true, 0, 99);
    mark("edit_arcane", false, true, 0, 99);
    mark("edit_max_hp", false, true, 0, 5000);
    mark("edit_max_fp", false, true, 0, 5000);
    mark("edit_max_stamina", false, true, 0, 1000);
}

}  // namespace

const char* groupTitleRu(FeatureGroup group) {
    switch (group) {
        case FeatureGroup::Survival: return "Выживание";
        case FeatureGroup::Items:    return "Предметы";
        case FeatureGroup::Combat:   return "Бой";
        case FeatureGroup::World:    return "Мир";
        case FeatureGroup::Stats:    return "Характеристики";
    }
    return "";
}

const char* groupTitleEn(FeatureGroup group) {
    switch (group) {
        case FeatureGroup::Survival: return "Survival";
        case FeatureGroup::Items:    return "Items";
        case FeatureGroup::Combat:   return "Combat";
        case FeatureGroup::World:    return "World";
        case FeatureGroup::Stats:    return "Stats";
    }
    return "";
}

const char* hotkeySectionRu(bool ctrl, bool alt) {
    if (ctrl) {
        return "CTRL + NUM: общие";
    }
    if (alt) {
        return "ALT + NUM: характеристики";
    }
    return "NUM: основные";
}

const std::vector<FeatureInfo>& featureCatalog() {
    static std::vector<FeatureInfo> k = {
        {"god_mode", "Режим бога / игнор ударов", "GOD MODE/IGNORE HITS", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_NUMPAD1}},
        {"infinite_hp", "Бесконечное HP", "INFINITE HP", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_NUMPAD2}},
        {"infinite_fp", "Бесконечное FP", "INFINITE FP", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_NUMPAD3}},
        {"infinite_stamina", "Бесконечная выносливость", "INFINITE STAMINA", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_NUMPAD4}},
        {"zero_weight", "Нулевой вес", "ZERO WEIGHT", FeatureGroup::Items, FeatureKind::Toggle,
         {false, false, false, VK_NUMPAD5}},
        {"infinite_item_usage", "Бесконечное использование предметов", "INFINITE ITEM USAGE", FeatureGroup::Items,
         FeatureKind::Toggle, {false, false, false, VK_NUMPAD6}},
        {"drop_rate_100", "100% шанс дропа", "100% DROP RATE", FeatureGroup::Items, FeatureKind::Toggle,
         {false, false, false, VK_NUMPAD7}},
        {"stealth_mode", "Стелс (враги не видят)", "STEALTH MODE", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_NUMPAD8}},
        {"immune_status", "Иммунитет к негативным статусам", "IMMUNE TO ALL NEGATIVE STATUS", FeatureGroup::Survival,
         FeatureKind::Toggle, {false, false, false, VK_NUMPAD9}},
        {"one_hit_kill", "Супер-урон / один удар", "SUPER DAMAGE/ONE HIT KILL", FeatureGroup::Combat,
         FeatureKind::Toggle, {false, false, false, VK_NUMPAD0}},
        {"damage_multiplier", "Множитель урона", "DAMAGE MULTIPLIER", FeatureGroup::Combat, FeatureKind::Toggle,
         {false, false, false, VK_DECIMAL}},
        {"defense_multiplier", "Множитель защиты", "DEFENSE MULTIPLIER", FeatureGroup::Combat, FeatureKind::Toggle,
         {false, false, false, VK_ADD}},
        {"infinite_horse_hp", "Бесконечное HP лошади", "INFINITE HORSE HP", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_DIVIDE}},

        {"edit_runes", "Изменить руны", "EDIT RUNES", FeatureGroup::Items, FeatureKind::ApplyValue,
         {true, false, false, VK_NUMPAD1}},
        {"runes_multiplier", "Множитель рун", "RUNES MULTIPLIER", FeatureGroup::Items, FeatureKind::Toggle,
         {true, false, false, VK_NUMPAD2}},
        {"keep_runes_on_death", "Не терять руны при смерти", "WON'T LOSE RUNES WHEN PLAYER DIES", FeatureGroup::Items,
         FeatureKind::Toggle, {true, false, false, VK_NUMPAD3}},
        {"freeze_daytime", "Заморозить время суток", "FREEZE DAYTIME", FeatureGroup::World, FeatureKind::Toggle,
         {true, false, false, VK_NUMPAD4}},
        {"daytime_plus_1h", "Время суток +1 час", "DAYTIME +1 HOUR", FeatureGroup::World, FeatureKind::ApplyValue,
         {true, false, false, VK_NUMPAD5}},
        {"game_speed", "Скорость игры", "SET GAME SPEED", FeatureGroup::World, FeatureKind::Toggle,
         {true, false, false, VK_NUMPAD6}},
        {"fly_mode", "Режим полёта", "ENABLE FLY MODE", FeatureGroup::World, FeatureKind::Toggle,
         {true, false, false, VK_NUMPAD7}},
        {"fly_up", "Полёт вверх", "FLY UP", FeatureGroup::World, FeatureKind::Hold,
         {true, false, false, VK_NUMPAD8}},
        {"fly_down", "Полёт вниз", "FLY DOWN", FeatureGroup::World, FeatureKind::Hold,
         {true, false, false, VK_NUMPAD9}},
        {"freeze_enemies", "Заморозить врагов", "FREEZE ENEMIES POSITION", FeatureGroup::Combat, FeatureKind::Toggle,
         {true, false, false, VK_NUMPAD0}},

        {"edit_level", "Изменить уровень", "EDIT LEVEL", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD1}},
        {"edit_vigor", "Телосложение (Vigor)", "EDIT VIGOR", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD2}},
        {"edit_mind", "Разум (Mind)", "EDIT MIND", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD3}},
        {"edit_endurance", "Выносливость (End)", "EDIT ENDURANCE", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD4}},
        {"edit_strength", "Сила (Str)", "EDIT STRENGTH", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD5}},
        {"edit_dexterity", "Ловкость (Dex)", "EDIT Dexterity", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD6}},
        {"edit_intelligence", "Интеллект (Int)", "EDIT INTELLIGENCE", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD7}},
        {"edit_faith", "Вера (Fai)", "EDIT FAITH", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD8}},
        {"edit_arcane", "Мудрость (Arc)", "EDIT ARCANE", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD9}},
        {"edit_max_hp", "Макс. HP", "EDIT MAX HP", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_NUMPAD0}},
        {"edit_max_fp", "Макс. FP", "EDIT MAX FP", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_DECIMAL}},
        {"edit_max_stamina", "Макс. выносливость", "EDIT MAX STAMINA", FeatureGroup::Stats, FeatureKind::ApplyValue,
         {false, true, false, VK_ADD}},

        {"no_fall_damage", "Нет урона от падения", "NO FALL DAMAGE", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_F1}},
        {"infinite_flasks", "Бесконечные фляги", "INFINITE FLASKS", FeatureGroup::Items, FeatureKind::Toggle,
         {false, false, false, VK_F2}},
        {"silent_roll", "Бесшумный перекат", "SILENT ROLL", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_F3}},
        {"always_crit", "Всегда крит", "ALWAYS CRIT", FeatureGroup::Combat, FeatureKind::Toggle,
         {false, false, false, VK_F4}},
        {"no_fp_cost", "Заклинания без FP", "NO FP COST", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_F5}},
        {"unlock_map", "Вся карта открыта", "UNLOCK MAP", FeatureGroup::World, FeatureKind::Toggle,
         {false, false, false, VK_F6}},
        {"no_weapon_req", "Без требований к оружию", "NO STAT REQUIREMENTS", FeatureGroup::Combat, FeatureKind::Toggle,
         {false, false, false, VK_F7}},
        {"infinite_arrows", "Бесконечные стрелы", "INFINITE ARROWS", FeatureGroup::Items, FeatureKind::Toggle,
         {false, false, false, VK_F8}},
        {"no_summon_limit", "Без лимита фантомов", "NO SUMMON LIMIT", FeatureGroup::World, FeatureKind::Toggle,
         {false, false, false, VK_F9}},
        {"invisibility", "Невидимость", "INVISIBILITY", FeatureGroup::Survival, FeatureKind::Toggle,
         {false, false, false, VK_F10}},
        {"torrent_anywhere", "Поток в подземельях", "TORRENT ANYWHERE", FeatureGroup::World, FeatureKind::Toggle,
         {false, false, true, VK_NUMPAD1}},
        {"skip_fog", "Проход сквозь туман", "SKIP FOG WALLS", FeatureGroup::World, FeatureKind::Toggle,
         {false, false, true, VK_NUMPAD2}},
    };
    static bool tuned = false;
    if (!tuned) {
        tuneCatalog(k);
        tuned = true;
    }
    return k;
}

}  // namespace er
