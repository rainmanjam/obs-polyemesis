/**
 * Unit Tests for Backup/Failover System
 * Tests failover logic, backup relationships, and recovery
 */

#include "test_framework.h"
#include "../src/restreamer-output-profile.h"
#include "../src/restreamer-api.h"

/* Test: Set Backup Destination */
static bool test_set_backup_destination(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Failover Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    /* Add primary and backup destinations */
    profile_add_destination(profile, SERVICE_YOUTUBE, "youtube-primary",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_YOUTUBE, "youtube-backup",
                          ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(profile->destination_count, 2, "Should have 2 destinations");

    /* Set destination 1 as backup for destination 0 */
    bool result = profile_set_destination_backup(profile, 0, 1);
    ASSERT_TRUE(result, "Set backup should succeed");

    /* Verify backup relationship */
    ASSERT_EQ(profile->destinations[0].backup_index, 1, "Primary should reference backup");
    ASSERT_TRUE(profile->destinations[1].is_backup, "Destination 1 should be marked as backup");
    ASSERT_EQ(profile->destinations[1].primary_index, 0, "Backup should reference primary");
    ASSERT_FALSE(profile->destinations[1].enabled, "Backup should start disabled");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Remove Backup Relationship */
static bool test_remove_backup(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Failover Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    profile_add_destination(profile, SERVICE_YOUTUBE, "primary",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_YOUTUBE, "backup",
                          ORIENTATION_HORIZONTAL, &encoding);

    /* Set and then remove backup */
    profile_set_destination_backup(profile, 0, 1);
    bool removed = profile_remove_destination_backup(profile, 0);

    ASSERT_TRUE(removed, "Remove backup should succeed");
    ASSERT_EQ(profile->destinations[0].backup_index, (size_t)-1,
              "Primary should no longer reference backup");
    ASSERT_FALSE(profile->destinations[1].is_backup, "Destination should no longer be backup");
    ASSERT_EQ(profile->destinations[1].primary_index, (size_t)-1,
              "Backup should no longer reference primary");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Invalid Backup Configurations */
static bool test_invalid_backup_configs(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Failover Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    profile_add_destination(profile, SERVICE_YOUTUBE, "dest1",
                          ORIENTATION_HORIZONTAL, &encoding);

    /* Test: Can't set destination as its own backup */
    bool result = profile_set_destination_backup(profile, 0, 0);
    ASSERT_FALSE(result, "Should fail to set destination as its own backup");

    /* Test: Invalid indices */
    result = profile_set_destination_backup(profile, 0, 999);
    ASSERT_FALSE(result, "Should fail with invalid backup index");

    result = profile_set_destination_backup(profile, 999, 0);
    ASSERT_FALSE(result, "Should fail with invalid primary index");

    /* Test: Null profile */
    result = profile_set_destination_backup(NULL, 0, 1);
    ASSERT_FALSE(result, "Should fail with NULL profile");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Replace Existing Backup */
static bool test_replace_backup(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Failover Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    /* Add primary and two backup candidates */
    profile_add_destination(profile, SERVICE_YOUTUBE, "primary",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_YOUTUBE, "backup1",
                          ORIENTATION_HORIZONTAL, &encoding);
    profile_add_destination(profile, SERVICE_YOUTUBE, "backup2",
                          ORIENTATION_HORIZONTAL, &encoding);

    /* Set first backup */
    profile_set_destination_backup(profile, 0, 1);
    ASSERT_EQ(profile->destinations[0].backup_index, 1, "Should have backup1");
    ASSERT_TRUE(profile->destinations[1].is_backup, "Backup1 should be marked");

    /* Replace with second backup */
    profile_set_destination_backup(profile, 0, 2);
    ASSERT_EQ(profile->destinations[0].backup_index, 2, "Should now have backup2");
    ASSERT_FALSE(profile->destinations[1].is_backup, "Backup1 should no longer be marked");
    ASSERT_TRUE(profile->destinations[2].is_backup, "Backup2 should be marked");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Failover State Initialization */
static bool test_failover_state_init(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Failover Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    profile_add_destination(profile, SERVICE_YOUTUBE, "dest",
                          ORIENTATION_HORIZONTAL, &encoding);

    /* Verify initial failover state */
    profile_destination_t *dest = &profile->destinations[0];
    ASSERT_FALSE(dest->is_backup, "Should not be backup initially");
    ASSERT_FALSE(dest->failover_active, "Failover should not be active initially");
    ASSERT_EQ(dest->failover_start_time, 0, "Failover time should be 0 initially");
    ASSERT_EQ(dest->primary_index, (size_t)-1, "Primary index should be unset");
    ASSERT_EQ(dest->backup_index, (size_t)-1, "Backup index should be unset");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Bulk Destination Operations - Enable/Disable */
static bool test_bulk_enable_disable(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Bulk Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    /* Add 5 destinations */
    for (int i = 0; i < 5; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        profile_add_destination(profile, SERVICE_YOUTUBE, key,
                              ORIENTATION_HORIZONTAL, &encoding);
    }

    /* All should be enabled initially */
    for (int i = 0; i < 5; i++) {
        ASSERT_TRUE(profile->destinations[i].enabled, "All should be enabled initially");
    }

    /* Bulk disable destinations 1, 2, and 4 */
    size_t indices[] = {1, 2, 4};
    bool result = profile_bulk_enable_destinations(profile, NULL, indices, 3, false);
    ASSERT_TRUE(result, "Bulk disable should succeed");

    /* Verify results */
    ASSERT_TRUE(profile->destinations[0].enabled, "Dest 0 should still be enabled");
    ASSERT_FALSE(profile->destinations[1].enabled, "Dest 1 should be disabled");
    ASSERT_FALSE(profile->destinations[2].enabled, "Dest 2 should be disabled");
    ASSERT_TRUE(profile->destinations[3].enabled, "Dest 3 should still be enabled");
    ASSERT_FALSE(profile->destinations[4].enabled, "Dest 4 should be disabled");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Bulk Delete Destinations */
static bool test_bulk_delete(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Bulk Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    /* Add 5 destinations */
    for (int i = 0; i < 5; i++) {
        char key[32];
        snprintf(key, sizeof(key), "key%d", i);
        profile_add_destination(profile, SERVICE_YOUTUBE, key,
                              ORIENTATION_HORIZONTAL, &encoding);
    }

    ASSERT_EQ(profile->destination_count, 5, "Should have 5 destinations");

    /* Bulk delete destinations 1 and 3 */
    size_t indices[] = {1, 3};
    bool result = profile_bulk_delete_destinations(profile, indices, 2);
    ASSERT_TRUE(result, "Bulk delete should succeed");

    /* Should have 3 destinations remaining */
    ASSERT_EQ(profile->destination_count, 3, "Should have 3 destinations after deletion");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Bulk Operations - Invalid Indices */
static bool test_bulk_invalid_indices(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Bulk Test");

    encoding_settings_t encoding = profile_get_default_encoding();

    profile_add_destination(profile, SERVICE_YOUTUBE, "key",
                          ORIENTATION_HORIZONTAL, &encoding);

    /* Try to bulk enable with invalid index */
    size_t bad_indices[] = {0, 999};
    bool result = profile_bulk_enable_destinations(profile, NULL, bad_indices, 2, false);

    /* Should partially succeed (index 0 ok, index 999 fails) */
    /* The function returns false if ANY operation failed */
    ASSERT_FALSE(result, "Should return false when some operations fail");

    profile_manager_destroy(manager);
    return true;
}

/* Test: Bulk Operations - Null Safety */
static bool test_bulk_null_safety(void) {
    profile_manager_t *manager = profile_manager_create(NULL);
    output_profile_t *profile = profile_manager_create_profile(manager, "Bulk Test");

    /* NULL profile */
    size_t indices[] = {0};
    bool result = profile_bulk_enable_destinations(NULL, NULL, indices, 1, false);
    ASSERT_FALSE(result, "Should fail with NULL profile");

    /* NULL indices */
    result = profile_bulk_enable_destinations(profile, NULL, NULL, 1, false);
    ASSERT_FALSE(result, "Should fail with NULL indices");

    /* Zero count */
    result = profile_bulk_enable_destinations(profile, NULL, indices, 0, false);
    ASSERT_FALSE(result, "Should fail with zero count");

    profile_manager_destroy(manager);
    return true;
}

BEGIN_TEST_SUITE("Backup/Failover System")
    RUN_TEST(test_set_backup_destination, "Set Backup Destination");
    RUN_TEST(test_remove_backup, "Remove Backup Relationship");
    RUN_TEST(test_invalid_backup_configs, "Invalid Backup Configurations");
    RUN_TEST(test_replace_backup, "Replace Existing Backup");
    RUN_TEST(test_failover_state_init, "Failover State Initialization");
    RUN_TEST(test_bulk_enable_disable, "Bulk Enable/Disable");
    RUN_TEST(test_bulk_delete, "Bulk Delete");
    RUN_TEST(test_bulk_invalid_indices, "Bulk Operations - Invalid Indices");
    RUN_TEST(test_bulk_null_safety, "Bulk Operations - Null Safety");
END_TEST_SUITE()
