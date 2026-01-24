/*
====================
test_inventory.c

Unit tests for inventory management - items, ammo tracking, and stats
====================
*/

#include "../unity/unity.h"
#include <string.h>

// Forward declarations
typedef short int16;
typedef int qBool;

#define qTrue 1
#define qFalse 0

// Constants
#define MAX_CS_ITEMS 256
#define MAX_STATS 32

// Stats indices
enum {
	STAT_HEALTH_ICON		= 0,
	STAT_HEALTH,
	STAT_AMMO_ICON,
	STAT_AMMO,
	STAT_ARMOR_ICON,
	STAT_ARMOR,
	STAT_SELECTED_ICON,
	STAT_PICKUP_ICON,
	STAT_PICKUP_STRING,
	STAT_TIMER_ICON,
	STAT_TIMER,
	STAT_HELPICON,
	STAT_SELECTED_ITEM,
	STAT_LAYOUTS,
	STAT_FRAGS,
	STAT_FLASHES,
	STAT_CHASE,
	STAT_SPECTATOR
};

// Item indices (simplified for testing)
enum {
	ITEM_NULL = 0,
	ITEM_ARMOR_BODY,
	ITEM_ARMOR_COMBAT,
	ITEM_ARMOR_JACKET,
	ITEM_ARMOR_SHARD,
	ITEM_POWER_SCREEN,
	ITEM_POWER_SHIELD,
	ITEM_QUAD_DAMAGE,
	ITEM_INVULNERABILITY,
	ITEM_BULLETS,
	ITEM_SHELLS,
	ITEM_ROCKETS,
	ITEM_GRENADES,
	ITEM_CELLS,
	ITEM_SLUGS,
	ITEM_BLASTER,
	ITEM_SHOTGUN,
	ITEM_SUPERSHOTGUN,
	ITEM_MACHINEGUN,
	ITEM_CHAINGUN,
	ITEM_GRENADE_LAUNCHER,
	ITEM_ROCKET_LAUNCHER,
	ITEM_HYPERBLASTER,
	ITEM_RAILGUN,
	ITEM_BFG
};

// Inventory structure
typedef struct {
	int items[MAX_CS_ITEMS];
	int16 stats[MAX_STATS];
	
	// Ammo capacities
	int max_bullets;
	int max_shells;
	int max_rockets;
	int max_grenades;
	int max_cells;
	int max_slugs;
	
	// Current weapon
	int selected_item;
	int selected_weapon;
} inventory_t;

// Default ammo capacities
#define DEFAULT_BULLETS_MAX 200
#define DEFAULT_SHELLS_MAX 100
#define DEFAULT_ROCKETS_MAX 50
#define DEFAULT_GRENADES_MAX 50
#define DEFAULT_CELLS_MAX 200
#define DEFAULT_SLUGS_MAX 50

// Pack increases
#define PACK_BULLETS_MAX 300
#define PACK_SHELLS_MAX 200
#define PACK_ROCKETS_MAX 100
#define PACK_GRENADES_MAX 100
#define PACK_CELLS_MAX 300
#define PACK_SLUGS_MAX 100

// ============================================================================
// HELPER FUNCTIONS
// ============================================================================

static void InitInventory(inventory_t *inv) {
	memset(inv, 0, sizeof(*inv));
	inv->max_bullets = DEFAULT_BULLETS_MAX;
	inv->max_shells = DEFAULT_SHELLS_MAX;
	inv->max_rockets = DEFAULT_ROCKETS_MAX;
	inv->max_grenades = DEFAULT_GRENADES_MAX;
	inv->max_cells = DEFAULT_CELLS_MAX;
	inv->max_slugs = DEFAULT_SLUGS_MAX;
}

static void AddItem(inventory_t *inv, int item_index, int quantity) {
	if (item_index < 0 || item_index >= MAX_CS_ITEMS)
		return;
	inv->items[item_index] += quantity;
}

static void RemoveItem(inventory_t *inv, int item_index, int quantity) {
	if (item_index < 0 || item_index >= MAX_CS_ITEMS)
		return;
	inv->items[item_index] -= quantity;
	if (inv->items[item_index] < 0)
		inv->items[item_index] = 0;
}

static int GetItemCount(const inventory_t *inv, int item_index) {
	if (item_index < 0 || item_index >= MAX_CS_ITEMS)
		return 0;
	return inv->items[item_index];
}

static void AddAmmo(inventory_t *inv, int ammo_type, int quantity, int max_ammo) {
	inv->items[ammo_type] += quantity;
	if (inv->items[ammo_type] > max_ammo)
		inv->items[ammo_type] = max_ammo;
}

static qBool HasItem(const inventory_t *inv, int item_index) {
	return GetItemCount(inv, item_index) > 0;
}

static void SetStat(inventory_t *inv, int stat_index, int16 value) {
	if (stat_index < 0 || stat_index >= MAX_STATS)
		return;
	inv->stats[stat_index] = value;
}

static int16 GetStat(const inventory_t *inv, int stat_index) {
	if (stat_index < 0 || stat_index >= MAX_STATS)
		return 0;
	return inv->stats[stat_index];
}

// ============================================================================
// TESTS: Basic Item Operations
// ============================================================================

void test_Inventory_Init_EmptyInventory(void) {
	inventory_t inv;
	int i;
	
	InitInventory(&inv);
	
	// All items should be zero
	for (i = 0; i < MAX_CS_ITEMS; i++) {
		TEST_ASSERT_EQUAL(0, inv.items[i]);
	}
	
	// All stats should be zero
	for (i = 0; i < MAX_STATS; i++) {
		TEST_ASSERT_EQUAL(0, inv.stats[i]);
	}
	
	// Ammo capacities should be defaults
	TEST_ASSERT_EQUAL(DEFAULT_BULLETS_MAX, inv.max_bullets);
	TEST_ASSERT_EQUAL(DEFAULT_SHELLS_MAX, inv.max_shells);
	TEST_ASSERT_EQUAL(DEFAULT_ROCKETS_MAX, inv.max_rockets);
	TEST_ASSERT_EQUAL(DEFAULT_GRENADES_MAX, inv.max_grenades);
	TEST_ASSERT_EQUAL(DEFAULT_CELLS_MAX, inv.max_cells);
	TEST_ASSERT_EQUAL(DEFAULT_SLUGS_MAX, inv.max_slugs);
}

void test_Inventory_AddItem_Single(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_SHOTGUN, 1);
	
	TEST_ASSERT_EQUAL(1, GetItemCount(&inv, ITEM_SHOTGUN));
}

void test_Inventory_AddItem_Multiple(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_BULLETS, 50);
	
	TEST_ASSERT_EQUAL(50, GetItemCount(&inv, ITEM_BULLETS));
}

void test_Inventory_AddItem_Accumulate(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_BULLETS, 50);
	AddItem(&inv, ITEM_BULLETS, 30);
	
	TEST_ASSERT_EQUAL(80, GetItemCount(&inv, ITEM_BULLETS));
}

void test_Inventory_RemoveItem_Single(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_SHOTGUN, 1);
	RemoveItem(&inv, ITEM_SHOTGUN, 1);
	
	TEST_ASSERT_EQUAL(0, GetItemCount(&inv, ITEM_SHOTGUN));
}

void test_Inventory_RemoveItem_Partial(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_BULLETS, 100);
	RemoveItem(&inv, ITEM_BULLETS, 30);
	
	TEST_ASSERT_EQUAL(70, GetItemCount(&inv, ITEM_BULLETS));
}

void test_Inventory_RemoveItem_NegativeClamped(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_BULLETS, 50);
	RemoveItem(&inv, ITEM_BULLETS, 100);
	
	// Should clamp to zero, not go negative
	TEST_ASSERT_EQUAL(0, GetItemCount(&inv, ITEM_BULLETS));
}

void test_Inventory_HasItem_Present(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_SHOTGUN, 1);
	
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_SHOTGUN));
}

void test_Inventory_HasItem_Absent(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	TEST_ASSERT_FALSE(HasItem(&inv, ITEM_SHOTGUN));
}

// ============================================================================
// TESTS: Ammo Management
// ============================================================================

void test_Ammo_AddBullets_WithinLimit(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_BULLETS, 50, inv.max_bullets);
	
	TEST_ASSERT_EQUAL(50, GetItemCount(&inv, ITEM_BULLETS));
}

void test_Ammo_AddBullets_ExceedLimit(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_BULLETS, 250, inv.max_bullets);
	
	// Should be clamped to max
	TEST_ASSERT_EQUAL(DEFAULT_BULLETS_MAX, GetItemCount(&inv, ITEM_BULLETS));
}

void test_Ammo_AddShells_MultiplePickups(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_SHELLS, 10, inv.max_shells);
	AddAmmo(&inv, ITEM_SHELLS, 10, inv.max_shells);
	AddAmmo(&inv, ITEM_SHELLS, 10, inv.max_shells);
	
	TEST_ASSERT_EQUAL(30, GetItemCount(&inv, ITEM_SHELLS));
}

void test_Ammo_AddRockets_AtMaximum(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_ROCKETS, DEFAULT_ROCKETS_MAX, inv.max_rockets);
	AddAmmo(&inv, ITEM_ROCKETS, 10, inv.max_rockets);
	
	// Should stay at max
	TEST_ASSERT_EQUAL(DEFAULT_ROCKETS_MAX, GetItemCount(&inv, ITEM_ROCKETS));
}

void test_Ammo_Grenades_Capacity(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_GRENADES, 100, inv.max_grenades);
	
	TEST_ASSERT_EQUAL(DEFAULT_GRENADES_MAX, GetItemCount(&inv, ITEM_GRENADES));
}

void test_Ammo_Cells_Capacity(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_CELLS, 300, inv.max_cells);
	
	TEST_ASSERT_EQUAL(DEFAULT_CELLS_MAX, GetItemCount(&inv, ITEM_CELLS));
}

void test_Ammo_Slugs_Capacity(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_SLUGS, 100, inv.max_slugs);
	
	TEST_ASSERT_EQUAL(DEFAULT_SLUGS_MAX, GetItemCount(&inv, ITEM_SLUGS));
}

void test_Ammo_AllTypes_Independent(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_BULLETS, 50, inv.max_bullets);
	AddAmmo(&inv, ITEM_SHELLS, 30, inv.max_shells);
	AddAmmo(&inv, ITEM_ROCKETS, 20, inv.max_rockets);
	
	TEST_ASSERT_EQUAL(50, GetItemCount(&inv, ITEM_BULLETS));
	TEST_ASSERT_EQUAL(30, GetItemCount(&inv, ITEM_SHELLS));
	TEST_ASSERT_EQUAL(20, GetItemCount(&inv, ITEM_ROCKETS));
}

// ============================================================================
// TESTS: Ammo Pack Upgrades
// ============================================================================

void test_AmmoPack_IncreaseCapacity_Bullets(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	// Simulate ammo pack pickup
	if (inv.max_bullets < PACK_BULLETS_MAX)
		inv.max_bullets = PACK_BULLETS_MAX;
	
	TEST_ASSERT_EQUAL(PACK_BULLETS_MAX, inv.max_bullets);
}

void test_AmmoPack_FillAfterCapacityIncrease(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_BULLETS, 200, inv.max_bullets);
	
	// Increase capacity
	inv.max_bullets = PACK_BULLETS_MAX;
	
	// Add more ammo with new limit
	AddAmmo(&inv, ITEM_BULLETS, 100, inv.max_bullets);
	
	TEST_ASSERT_EQUAL(300, GetItemCount(&inv, ITEM_BULLETS));
}

void test_AmmoPack_AllAmmoTypes(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	// Simulate ammo pack pickup - increase all capacities
	inv.max_bullets = PACK_BULLETS_MAX;
	inv.max_shells = PACK_SHELLS_MAX;
	inv.max_rockets = PACK_ROCKETS_MAX;
	inv.max_grenades = PACK_GRENADES_MAX;
	inv.max_cells = PACK_CELLS_MAX;
	inv.max_slugs = PACK_SLUGS_MAX;
	
	TEST_ASSERT_EQUAL(PACK_BULLETS_MAX, inv.max_bullets);
	TEST_ASSERT_EQUAL(PACK_SHELLS_MAX, inv.max_shells);
	TEST_ASSERT_EQUAL(PACK_ROCKETS_MAX, inv.max_rockets);
	TEST_ASSERT_EQUAL(PACK_GRENADES_MAX, inv.max_grenades);
	TEST_ASSERT_EQUAL(PACK_CELLS_MAX, inv.max_cells);
	TEST_ASSERT_EQUAL(PACK_SLUGS_MAX, inv.max_slugs);
}

// ============================================================================
// TESTS: Stats System
// ============================================================================

void test_Stats_SetHealth(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_HEALTH, 100);
	
	TEST_ASSERT_EQUAL(100, GetStat(&inv, STAT_HEALTH));
}

void test_Stats_SetAmmo(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_AMMO, 50);
	
	TEST_ASSERT_EQUAL(50, GetStat(&inv, STAT_AMMO));
}

void test_Stats_SetArmor(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_ARMOR, 150);
	
	TEST_ASSERT_EQUAL(150, GetStat(&inv, STAT_ARMOR));
}

void test_Stats_SetFrags(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_FRAGS, 10);
	
	TEST_ASSERT_EQUAL(10, GetStat(&inv, STAT_FRAGS));
}

void test_Stats_NegativeFrags(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_FRAGS, -5);
	
	TEST_ASSERT_EQUAL(-5, GetStat(&inv, STAT_FRAGS));
}

void test_Stats_MultipleStats(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_HEALTH, 100);
	SetStat(&inv, STAT_ARMOR, 50);
	SetStat(&inv, STAT_AMMO, 200);
	SetStat(&inv, STAT_FRAGS, 15);
	
	TEST_ASSERT_EQUAL(100, GetStat(&inv, STAT_HEALTH));
	TEST_ASSERT_EQUAL(50, GetStat(&inv, STAT_ARMOR));
	TEST_ASSERT_EQUAL(200, GetStat(&inv, STAT_AMMO));
	TEST_ASSERT_EQUAL(15, GetStat(&inv, STAT_FRAGS));
}

void test_Stats_SelectedItem(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_SELECTED_ITEM, ITEM_SHOTGUN);
	
	TEST_ASSERT_EQUAL(ITEM_SHOTGUN, GetStat(&inv, STAT_SELECTED_ITEM));
}

void test_Stats_HealthFlash(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_FLASHES, 1); // Health flash
	
	TEST_ASSERT_EQUAL(1, GetStat(&inv, STAT_FLASHES));
}

void test_Stats_ArmorFlash(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_FLASHES, 2); // Armor flash
	
	TEST_ASSERT_EQUAL(2, GetStat(&inv, STAT_FLASHES));
}

// ============================================================================
// TESTS: Weapon Selection
// ============================================================================

void test_Weapon_SelectWeapon(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	inv.selected_weapon = ITEM_SHOTGUN;
	
	TEST_ASSERT_EQUAL(ITEM_SHOTGUN, inv.selected_weapon);
}

void test_Weapon_SwitchWeapon(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	inv.selected_weapon = ITEM_BLASTER;
	inv.selected_weapon = ITEM_SHOTGUN;
	
	TEST_ASSERT_EQUAL(ITEM_SHOTGUN, inv.selected_weapon);
}

void test_Weapon_RequiresAmmo(void) {
	inventory_t inv;
	qBool hasAmmo;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_SHOTGUN, 1);
	AddAmmo(&inv, ITEM_SHELLS, 10, inv.max_shells);
	
	// Check if player has weapon and ammo
	hasAmmo = HasItem(&inv, ITEM_SHOTGUN) && GetItemCount(&inv, ITEM_SHELLS) > 0;
	
	TEST_ASSERT_TRUE(hasAmmo);
}

void test_Weapon_NoAmmo(void) {
	inventory_t inv;
	qBool hasAmmo;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_SHOTGUN, 1);
	// Don't add shells
	
	hasAmmo = HasItem(&inv, ITEM_SHOTGUN) && GetItemCount(&inv, ITEM_SHELLS) > 0;
	
	TEST_ASSERT_FALSE(hasAmmo);
}

// ============================================================================
// TESTS: Complex Scenarios
// ============================================================================

void test_Scenario_PickupWeaponAndAmmo(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	// Pick up shotgun
	AddItem(&inv, ITEM_SHOTGUN, 1);
	
	// Pick up shells
	AddAmmo(&inv, ITEM_SHELLS, 10, inv.max_shells);
	
	// Select weapon
	inv.selected_weapon = ITEM_SHOTGUN;
	
	// Update stats
	SetStat(&inv, STAT_SELECTED_ITEM, ITEM_SHOTGUN);
	SetStat(&inv, STAT_AMMO, GetItemCount(&inv, ITEM_SHELLS));
	
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_SHOTGUN));
	TEST_ASSERT_EQUAL(10, GetItemCount(&inv, ITEM_SHELLS));
	TEST_ASSERT_EQUAL(ITEM_SHOTGUN, inv.selected_weapon);
	TEST_ASSERT_EQUAL(10, GetStat(&inv, STAT_AMMO));
}

void test_Scenario_FireWeapon_ConsumeAmmo(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_SHOTGUN, 1);
	AddAmmo(&inv, ITEM_SHELLS, 10, inv.max_shells);
	
	// Fire weapon (consume 1 shell)
	RemoveItem(&inv, ITEM_SHELLS, 1);
	
	TEST_ASSERT_EQUAL(9, GetItemCount(&inv, ITEM_SHELLS));
}

void test_Scenario_MultipleWeapons(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	// Collect multiple weapons
	AddItem(&inv, ITEM_BLASTER, 1);
	AddItem(&inv, ITEM_SHOTGUN, 1);
	AddItem(&inv, ITEM_SUPERSHOTGUN, 1);
	AddItem(&inv, ITEM_MACHINEGUN, 1);
	
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_BLASTER));
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_SHOTGUN));
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_SUPERSHOTGUN));
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_MACHINEGUN));
}

void test_Scenario_TakeDamage_UpdateHealth(void) {
	inventory_t inv;
	int16 health;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_HEALTH, 100);
	
	// Take damage
	health = GetStat(&inv, STAT_HEALTH);
	health -= 25;
	SetStat(&inv, STAT_HEALTH, health);
	
	TEST_ASSERT_EQUAL(75, GetStat(&inv, STAT_HEALTH));
}

void test_Scenario_PickupArmor(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	// Pick up combat armor (100 points)
	AddItem(&inv, ITEM_ARMOR_COMBAT, 1);
	SetStat(&inv, STAT_ARMOR, 100);
	
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_ARMOR_COMBAT));
	TEST_ASSERT_EQUAL(100, GetStat(&inv, STAT_ARMOR));
}

void test_Scenario_ArmorAbsorption(void) {
	inventory_t inv;
	int16 armor, health;
	int damage = 30;
	int absorbed;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_HEALTH, 100);
	SetStat(&inv, STAT_ARMOR, 50);
	
	// Simulate armor absorption (2/3 absorbed)
	absorbed = (damage * 2) / 3;
	
	armor = GetStat(&inv, STAT_ARMOR) - absorbed;
	health = GetStat(&inv, STAT_HEALTH) - (damage - absorbed);
	
	SetStat(&inv, STAT_ARMOR, armor);
	SetStat(&inv, STAT_HEALTH, health);
	
	TEST_ASSERT_EQUAL(30, GetStat(&inv, STAT_ARMOR));  // 50 - 20 = 30
	TEST_ASSERT_EQUAL(90, GetStat(&inv, STAT_HEALTH)); // 100 - 10 = 90
}

void test_Scenario_PowerupPickup(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	// Pick up quad damage
	AddItem(&inv, ITEM_QUAD_DAMAGE, 1);
	
	TEST_ASSERT_TRUE(HasItem(&inv, ITEM_QUAD_DAMAGE));
	TEST_ASSERT_EQUAL(1, GetItemCount(&inv, ITEM_QUAD_DAMAGE));
}

void test_Scenario_PowerupUse(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, ITEM_QUAD_DAMAGE, 1);
	
	// Use powerup
	RemoveItem(&inv, ITEM_QUAD_DAMAGE, 1);
	
	TEST_ASSERT_FALSE(HasItem(&inv, ITEM_QUAD_DAMAGE));
}

// ============================================================================
// TESTS: Edge Cases
// ============================================================================

void test_EdgeCase_MaxInventorySlots(void) {
	inventory_t inv;
	int i;
	
	InitInventory(&inv);
	
	// Fill all slots
	for (i = 0; i < MAX_CS_ITEMS; i++) {
		AddItem(&inv, i, 1);
	}
	
	// Verify all slots used
	for (i = 0; i < MAX_CS_ITEMS; i++) {
		TEST_ASSERT_EQUAL(1, GetItemCount(&inv, i));
	}
}

void test_EdgeCase_NegativeItemIndex(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, -1, 10);
	
	// Should be ignored
	TEST_ASSERT_EQUAL(0, GetItemCount(&inv, -1));
}

void test_EdgeCase_TooLargeItemIndex(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddItem(&inv, MAX_CS_ITEMS + 10, 10);
	
	// Should be ignored
	TEST_ASSERT_EQUAL(0, GetItemCount(&inv, MAX_CS_ITEMS + 10));
}

void test_EdgeCase_ZeroAmmoAdd(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	AddAmmo(&inv, ITEM_BULLETS, 0, inv.max_bullets);
	
	TEST_ASSERT_EQUAL(0, GetItemCount(&inv, ITEM_BULLETS));
}

void test_EdgeCase_StatsMaxValue(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_HEALTH, 32767); // Max int16
	
	TEST_ASSERT_EQUAL(32767, GetStat(&inv, STAT_HEALTH));
}

void test_EdgeCase_StatsMinValue(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	SetStat(&inv, STAT_FRAGS, -32768); // Min int16
	
	TEST_ASSERT_EQUAL(-32768, GetStat(&inv, STAT_FRAGS));
}

void test_EdgeCase_AllAmmoAtMax(void) {
	inventory_t inv;
	
	InitInventory(&inv);
	
	// Fill all ammo types to maximum
	AddAmmo(&inv, ITEM_BULLETS, 999, inv.max_bullets);
	AddAmmo(&inv, ITEM_SHELLS, 999, inv.max_shells);
	AddAmmo(&inv, ITEM_ROCKETS, 999, inv.max_rockets);
	AddAmmo(&inv, ITEM_GRENADES, 999, inv.max_grenades);
	AddAmmo(&inv, ITEM_CELLS, 999, inv.max_cells);
	AddAmmo(&inv, ITEM_SLUGS, 999, inv.max_slugs);
	
	TEST_ASSERT_EQUAL(DEFAULT_BULLETS_MAX, GetItemCount(&inv, ITEM_BULLETS));
	TEST_ASSERT_EQUAL(DEFAULT_SHELLS_MAX, GetItemCount(&inv, ITEM_SHELLS));
	TEST_ASSERT_EQUAL(DEFAULT_ROCKETS_MAX, GetItemCount(&inv, ITEM_ROCKETS));
	TEST_ASSERT_EQUAL(DEFAULT_GRENADES_MAX, GetItemCount(&inv, ITEM_GRENADES));
	TEST_ASSERT_EQUAL(DEFAULT_CELLS_MAX, GetItemCount(&inv, ITEM_CELLS));
	TEST_ASSERT_EQUAL(DEFAULT_SLUGS_MAX, GetItemCount(&inv, ITEM_SLUGS));
}

// ============================================================================
// MAIN
// ============================================================================

int main(void) {
	UNITY_BEGIN();

	// Basic Item Operations
	RUN_TEST(test_Inventory_Init_EmptyInventory);
	RUN_TEST(test_Inventory_AddItem_Single);
	RUN_TEST(test_Inventory_AddItem_Multiple);
	RUN_TEST(test_Inventory_AddItem_Accumulate);
	RUN_TEST(test_Inventory_RemoveItem_Single);
	RUN_TEST(test_Inventory_RemoveItem_Partial);
	RUN_TEST(test_Inventory_RemoveItem_NegativeClamped);
	RUN_TEST(test_Inventory_HasItem_Present);
	RUN_TEST(test_Inventory_HasItem_Absent);

	// Ammo Management
	RUN_TEST(test_Ammo_AddBullets_WithinLimit);
	RUN_TEST(test_Ammo_AddBullets_ExceedLimit);
	RUN_TEST(test_Ammo_AddShells_MultiplePickups);
	RUN_TEST(test_Ammo_AddRockets_AtMaximum);
	RUN_TEST(test_Ammo_Grenades_Capacity);
	RUN_TEST(test_Ammo_Cells_Capacity);
	RUN_TEST(test_Ammo_Slugs_Capacity);
	RUN_TEST(test_Ammo_AllTypes_Independent);

	// Ammo Pack Upgrades
	RUN_TEST(test_AmmoPack_IncreaseCapacity_Bullets);
	RUN_TEST(test_AmmoPack_FillAfterCapacityIncrease);
	RUN_TEST(test_AmmoPack_AllAmmoTypes);

	// Stats System
	RUN_TEST(test_Stats_SetHealth);
	RUN_TEST(test_Stats_SetAmmo);
	RUN_TEST(test_Stats_SetArmor);
	RUN_TEST(test_Stats_SetFrags);
	RUN_TEST(test_Stats_NegativeFrags);
	RUN_TEST(test_Stats_MultipleStats);
	RUN_TEST(test_Stats_SelectedItem);
	RUN_TEST(test_Stats_HealthFlash);
	RUN_TEST(test_Stats_ArmorFlash);

	// Weapon Selection
	RUN_TEST(test_Weapon_SelectWeapon);
	RUN_TEST(test_Weapon_SwitchWeapon);
	RUN_TEST(test_Weapon_RequiresAmmo);
	RUN_TEST(test_Weapon_NoAmmo);

	// Complex Scenarios
	RUN_TEST(test_Scenario_PickupWeaponAndAmmo);
	RUN_TEST(test_Scenario_FireWeapon_ConsumeAmmo);
	RUN_TEST(test_Scenario_MultipleWeapons);
	RUN_TEST(test_Scenario_TakeDamage_UpdateHealth);
	RUN_TEST(test_Scenario_PickupArmor);
	RUN_TEST(test_Scenario_ArmorAbsorption);
	RUN_TEST(test_Scenario_PowerupPickup);
	RUN_TEST(test_Scenario_PowerupUse);

	// Edge Cases
	RUN_TEST(test_EdgeCase_MaxInventorySlots);
	RUN_TEST(test_EdgeCase_NegativeItemIndex);
	RUN_TEST(test_EdgeCase_TooLargeItemIndex);
	RUN_TEST(test_EdgeCase_ZeroAmmoAdd);
	RUN_TEST(test_EdgeCase_StatsMaxValue);
	RUN_TEST(test_EdgeCase_StatsMinValue);
	RUN_TEST(test_EdgeCase_AllAmmoAtMax);

	return UNITY_END();
}
