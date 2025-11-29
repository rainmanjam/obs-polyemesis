/**
 * Unit Tests for Channel Template Management
 * Tests template creation, deletion, retrieval, and persistence
 * Covers lines 1356-1541 in restreamer-channel.c
 */

#include "test_framework.h"
#include "../src/restreamer-channel.h"
#include "../src/restreamer-api.h"
#include <obs.h>

/* Mock API for testing */
static restreamer_api_t *create_mock_api(void) {
    /* For unit tests, we'll use NULL and test the logic without actual API calls */
    return NULL;
}

/* Helper function to verify encoding settings match */
static bool encoding_settings_match(encoding_settings_t *a, encoding_settings_t *b) {
    return a->bitrate == b->bitrate &&
           a->width == b->width &&
           a->height == b->height &&
           a->audio_bitrate == b->audio_bitrate;
}

/* ========================================================================
 * Test: Create Template - Success Case
 * ======================================================================== */
static bool test_create_template_success(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Store initial template count (built-in templates) */
    size_t initial_count = manager->template_count;
    ASSERT_EQ(initial_count, 6, "Should start with 6 built-in templates");

    /* Create custom template */
    encoding_settings_t encoding = channel_get_default_encoding();
    encoding.bitrate = 8000;
    encoding.width = 2560;
    encoding.height = 1440;
    encoding.audio_bitrate = 192;

    output_template_t *tmpl = channel_manager_create_template(
        manager, "Custom 1440p", SERVICE_YOUTUBE,
        ORIENTATION_HORIZONTAL, &encoding);

    /* Verify template was created */
    ASSERT_NOT_NULL(tmpl, "Template should be created");
    ASSERT_STR_EQ(tmpl->template_name, "Custom 1440p", "Template name should match");
    ASSERT_NOT_NULL(tmpl->template_id, "Template ID should be generated");
    ASSERT_EQ(tmpl->service, SERVICE_YOUTUBE, "Service should be YouTube");
    ASSERT_EQ(tmpl->orientation, ORIENTATION_HORIZONTAL, "Orientation should be horizontal");
    ASSERT_FALSE(tmpl->is_builtin, "Should not be a built-in template");

    /* Verify encoding settings */
    ASSERT_EQ(tmpl->encoding.bitrate, 8000, "Bitrate should be 8000");
    ASSERT_EQ(tmpl->encoding.width, 2560, "Width should be 2560");
    ASSERT_EQ(tmpl->encoding.height, 1440, "Height should be 1440");
    ASSERT_EQ(tmpl->encoding.audio_bitrate, 192, "Audio bitrate should be 192");

    /* Verify template was added to manager */
    ASSERT_EQ(manager->template_count, initial_count + 1, "Template count should increase by 1");

    /* Verify template is in the array */
    output_template_t *retrieved = manager->templates[manager->template_count - 1];
    ASSERT(retrieved == tmpl, "Last template should be the one we created");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Create Template - NULL Parameters
 * ======================================================================== */
static bool test_create_template_null_params(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    encoding_settings_t encoding = channel_get_default_encoding();

    /* Test NULL manager */
    output_template_t *result1 = channel_manager_create_template(
        NULL, "Test", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &encoding);
    ASSERT_NULL(result1, "Should return NULL for NULL manager");

    /* Test NULL name */
    output_template_t *result2 = channel_manager_create_template(
        manager, NULL, SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &encoding);
    ASSERT_NULL(result2, "Should return NULL for NULL name");

    /* Test NULL encoding */
    output_template_t *result3 = channel_manager_create_template(
        manager, "Test", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, NULL);
    ASSERT_NULL(result3, "Should return NULL for NULL encoding");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Delete Template - Success Case
 * ======================================================================== */
static bool test_delete_template_success(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Create custom templates */
    encoding_settings_t encoding = channel_get_default_encoding();
    output_template_t *tmpl1 = channel_manager_create_template(
        manager, "Custom 1", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &encoding);
    output_template_t *tmpl2 = channel_manager_create_template(
        manager, "Custom 2", SERVICE_TWITCH, ORIENTATION_HORIZONTAL, &encoding);
    output_template_t *tmpl3 = channel_manager_create_template(
        manager, "Custom 3", SERVICE_FACEBOOK, ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_NOT_NULL(tmpl1, "Template 1 should be created");
    ASSERT_NOT_NULL(tmpl2, "Template 2 should be created");
    ASSERT_NOT_NULL(tmpl3, "Template 3 should be created");

    size_t count_before = manager->template_count;

    /* Save template ID before deleting (to avoid use-after-free) */
    char *tmpl2_id = bstrdup(tmpl2->template_id);

    /* Delete middle template */
    bool deleted = channel_manager_delete_template(manager, tmpl2_id);
    ASSERT_TRUE(deleted, "Delete should succeed");
    ASSERT_EQ(manager->template_count, count_before - 1, "Template count should decrease by 1");

    /* Verify template was removed */
    output_template_t *search = channel_manager_get_template(manager, tmpl2_id);
    ASSERT_NULL(search, "Deleted template should not be found");

    bfree(tmpl2_id);

    /* Verify other templates still exist */
    output_template_t *found1 = channel_manager_get_template(manager, tmpl1->template_id);
    output_template_t *found3 = channel_manager_get_template(manager, tmpl3->template_id);
    ASSERT_NOT_NULL(found1, "Template 1 should still exist");
    ASSERT_NOT_NULL(found3, "Template 3 should still exist");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Delete Template - Built-in Templates Cannot Be Deleted
 * ======================================================================== */
static bool test_delete_template_builtin_fails(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Try to delete a built-in template */
    ASSERT_TRUE(manager->template_count > 0, "Should have built-in templates");

    output_template_t *builtin = manager->templates[0];
    ASSERT_TRUE(builtin->is_builtin, "First template should be built-in");

    char *builtin_id = bstrdup(builtin->template_id);
    size_t count_before = manager->template_count;

    /* Attempt to delete built-in template */
    bool deleted = channel_manager_delete_template(manager, builtin_id);
    ASSERT_FALSE(deleted, "Should fail to delete built-in template");
    ASSERT_EQ(manager->template_count, count_before, "Template count should not change");

    /* Verify template still exists */
    output_template_t *still_there = channel_manager_get_template(manager, builtin_id);
    ASSERT_NOT_NULL(still_there, "Built-in template should still exist");

    bfree(builtin_id);
    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Delete Template - Non-existent Template
 * ======================================================================== */
static bool test_delete_template_not_found(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    size_t count_before = manager->template_count;

    /* Try to delete non-existent template */
    bool deleted = channel_manager_delete_template(manager, "nonexistent_id_12345");
    ASSERT_FALSE(deleted, "Should fail to delete non-existent template");
    ASSERT_EQ(manager->template_count, count_before, "Template count should not change");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Get Template - Success Case
 * ======================================================================== */
static bool test_get_template_success(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Create custom template */
    encoding_settings_t encoding = channel_get_default_encoding();
    encoding.bitrate = 5000;

    output_template_t *created = channel_manager_create_template(
        manager, "Test Template", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &encoding);
    ASSERT_NOT_NULL(created, "Template should be created");

    /* Retrieve template by ID */
    output_template_t *retrieved = channel_manager_get_template(manager, created->template_id);
    ASSERT_NOT_NULL(retrieved, "Template should be found");
    ASSERT(retrieved == created, "Retrieved template should be the same object");
    ASSERT_STR_EQ(retrieved->template_name, "Test Template", "Template name should match");
    ASSERT_EQ(retrieved->encoding.bitrate, 5000, "Bitrate should match");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Get Template - Not Found
 * ======================================================================== */
static bool test_get_template_not_found(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Try to get non-existent template */
    output_template_t *result = channel_manager_get_template(manager, "does_not_exist");
    ASSERT_NULL(result, "Should return NULL for non-existent template");

    /* Test NULL manager */
    output_template_t *result2 = channel_manager_get_template(NULL, "some_id");
    ASSERT_NULL(result2, "Should return NULL for NULL manager");

    /* Test NULL template_id */
    output_template_t *result3 = channel_manager_get_template(manager, NULL);
    ASSERT_NULL(result3, "Should return NULL for NULL template_id");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Get Template At Index - Success Case
 * ======================================================================== */
static bool test_get_template_at_success(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Get built-in templates by index */
    ASSERT_TRUE(manager->template_count >= 6, "Should have at least 6 built-in templates");

    for (size_t i = 0; i < manager->template_count; i++) {
        output_template_t *tmpl = channel_manager_get_template_at(manager, i);
        ASSERT_NOT_NULL(tmpl, "Template at index should exist");
        ASSERT(tmpl == manager->templates[i], "Should return correct template");
    }

    /* Add custom template and get it by index */
    encoding_settings_t encoding = channel_get_default_encoding();
    output_template_t *custom = channel_manager_create_template(
        manager, "Custom", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &encoding);
    ASSERT_NOT_NULL(custom, "Custom template should be created");

    size_t last_index = manager->template_count - 1;
    output_template_t *last = channel_manager_get_template_at(manager, last_index);
    ASSERT(last == custom, "Last template should be the custom one");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Get Template At Index - Out of Bounds
 * ======================================================================== */
static bool test_get_template_at_out_of_bounds(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Try to get template at out of bounds index */
    output_template_t *result = channel_manager_get_template_at(manager, manager->template_count);
    ASSERT_NULL(result, "Should return NULL for out of bounds index");

    result = channel_manager_get_template_at(manager, manager->template_count + 100);
    ASSERT_NULL(result, "Should return NULL for way out of bounds index");

    /* Test NULL manager */
    result = channel_manager_get_template_at(NULL, 0);
    ASSERT_NULL(result, "Should return NULL for NULL manager");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Apply Template - Success Case
 * ======================================================================== */
static bool test_apply_template_success(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");

    ASSERT_NOT_NULL(channel, "Channel should be created");
    ASSERT_EQ(channel->output_count, 0, "Channel should start with no outputs");

    /* Get a built-in template */
    output_template_t *tmpl = manager->templates[0];
    ASSERT_NOT_NULL(tmpl, "Should have a built-in template");

    /* Apply template to channel */
    bool result = channel_apply_template(channel, tmpl, "test-stream-key-123");
    ASSERT_TRUE(result, "Apply template should succeed");

    /* Verify output was added */
    ASSERT_EQ(channel->output_count, 1, "Channel should have 1 output");

    /* Verify output properties match template */
    channel_output_t *output = &channel->outputs[0];
    ASSERT_EQ(output->service, tmpl->service, "Service should match template");
    ASSERT_STR_EQ(output->stream_key, "test-stream-key-123", "Stream key should match");
    ASSERT_EQ(output->target_orientation, tmpl->orientation, "Orientation should match template");
    ASSERT_EQ(output->encoding.bitrate, tmpl->encoding.bitrate, "Bitrate should match template");
    ASSERT_EQ(output->encoding.width, tmpl->encoding.width, "Width should match template");
    ASSERT_EQ(output->encoding.height, tmpl->encoding.height, "Height should match template");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Apply Template - NULL Parameters
 * ======================================================================== */
static bool test_apply_template_null_params(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Test Channel");
    output_template_t *tmpl = manager->templates[0];

    /* Test NULL channel */
    bool result1 = channel_apply_template(NULL, tmpl, "key");
    ASSERT_FALSE(result1, "Should fail with NULL channel");

    /* Test NULL template */
    bool result2 = channel_apply_template(channel, NULL, "key");
    ASSERT_FALSE(result2, "Should fail with NULL template");

    /* Test NULL stream key */
    bool result3 = channel_apply_template(channel, tmpl, NULL);
    ASSERT_FALSE(result3, "Should fail with NULL stream key");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Save and Load Templates - Round Trip
 * ======================================================================== */
static bool test_save_and_load_templates(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager1 = channel_manager_create(api);

    /* Create custom templates */
    encoding_settings_t enc1 = channel_get_default_encoding();
    enc1.bitrate = 8000;
    enc1.width = 2560;
    enc1.height = 1440;
    enc1.audio_bitrate = 192;

    encoding_settings_t enc2 = channel_get_default_encoding();
    enc2.bitrate = 3000;
    enc2.width = 1280;
    enc2.height = 720;
    enc2.audio_bitrate = 128;

    output_template_t *custom1 = channel_manager_create_template(
        manager1, "Custom 1440p", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &enc1);
    output_template_t *custom2 = channel_manager_create_template(
        manager1, "Custom 720p", SERVICE_TWITCH, ORIENTATION_VERTICAL, &enc2);

    ASSERT_NOT_NULL(custom1, "Custom template 1 should be created");
    ASSERT_NOT_NULL(custom2, "Custom template 2 should be created");

    /* Save templates to settings */
    obs_data_t *settings = obs_data_create();
    channel_manager_save_templates(manager1, settings);

    /* Create new manager and load templates */
    channel_manager_t *manager2 = channel_manager_create(api);
    size_t builtin_count = manager2->template_count;
    ASSERT_EQ(builtin_count, 6, "New manager should have 6 built-in templates");

    channel_manager_load_templates(manager2, settings);

    /* Verify custom templates were loaded */
    ASSERT_EQ(manager2->template_count, builtin_count + 2, "Should have 2 additional custom templates");

    /* Find and verify the loaded templates */
    output_template_t *loaded1 = NULL;
    output_template_t *loaded2 = NULL;

    for (size_t i = builtin_count; i < manager2->template_count; i++) {
        output_template_t *tmpl = manager2->templates[i];
        if (strcmp(tmpl->template_name, "Custom 1440p") == 0) {
            loaded1 = tmpl;
        } else if (strcmp(tmpl->template_name, "Custom 720p") == 0) {
            loaded2 = tmpl;
        }
    }

    ASSERT_NOT_NULL(loaded1, "Custom 1440p should be loaded");
    ASSERT_NOT_NULL(loaded2, "Custom 720p should be loaded");

    /* Verify loaded1 properties */
    ASSERT_FALSE(loaded1->is_builtin, "Loaded template should not be built-in");
    ASSERT_EQ(loaded1->service, SERVICE_YOUTUBE, "Service should match");
    ASSERT_EQ(loaded1->orientation, ORIENTATION_HORIZONTAL, "Orientation should match");
    ASSERT_EQ(loaded1->encoding.bitrate, 8000, "Bitrate should match");
    ASSERT_EQ(loaded1->encoding.width, 2560, "Width should match");
    ASSERT_EQ(loaded1->encoding.height, 1440, "Height should match");
    ASSERT_EQ(loaded1->encoding.audio_bitrate, 192, "Audio bitrate should match");

    /* Verify loaded2 properties */
    ASSERT_FALSE(loaded2->is_builtin, "Loaded template should not be built-in");
    ASSERT_EQ(loaded2->service, SERVICE_TWITCH, "Service should match");
    ASSERT_EQ(loaded2->orientation, ORIENTATION_VERTICAL, "Orientation should match");
    ASSERT_EQ(loaded2->encoding.bitrate, 3000, "Bitrate should match");
    ASSERT_EQ(loaded2->encoding.width, 1280, "Width should match");
    ASSERT_EQ(loaded2->encoding.height, 720, "Height should match");
    ASSERT_EQ(loaded2->encoding.audio_bitrate, 128, "Audio bitrate should match");

    obs_data_release(settings);
    channel_manager_destroy(manager1);
    channel_manager_destroy(manager2);
    return true;
}

/* ========================================================================
 * Test: Save Templates - Only Custom Templates Saved
 * ======================================================================== */
static bool test_save_templates_only_custom(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    size_t builtin_count = manager->template_count;
    ASSERT_EQ(builtin_count, 6, "Should have 6 built-in templates");

    /* Create one custom template */
    encoding_settings_t encoding = channel_get_default_encoding();
    output_template_t *custom = channel_manager_create_template(
        manager, "Custom", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &encoding);
    ASSERT_NOT_NULL(custom, "Custom template should be created");

    /* Save templates */
    obs_data_t *settings = obs_data_create();
    channel_manager_save_templates(manager, settings);

    /* Get the saved array */
    obs_data_array_t *array = obs_data_get_array(settings, "output_templates");
    ASSERT_NOT_NULL(array, "Templates array should exist");

    /* Verify only 1 template was saved (custom only, not built-ins) */
    size_t saved_count = obs_data_array_count(array);
    ASSERT_EQ(saved_count, 1, "Should save only 1 custom template, not built-ins");

    /* Verify the saved template */
    obs_data_t *tmpl_data = obs_data_array_item(array, 0);
    ASSERT_NOT_NULL(tmpl_data, "Template data should exist");

    const char *name = obs_data_get_string(tmpl_data, "name");
    ASSERT_STR_EQ(name, "Custom", "Template name should match");

    obs_data_release(tmpl_data);
    obs_data_array_release(array);
    obs_data_release(settings);
    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Save Templates - NULL Parameters
 * ======================================================================== */
static bool test_save_templates_null_params(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    obs_data_t *settings = obs_data_create();

    /* These should not crash */
    channel_manager_save_templates(NULL, settings);
    channel_manager_save_templates(manager, NULL);
    channel_manager_save_templates(NULL, NULL);

    obs_data_release(settings);
    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Load Templates - NULL Parameters
 * ======================================================================== */
static bool test_load_templates_null_params(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    obs_data_t *settings = obs_data_create();

    /* These should not crash */
    channel_manager_load_templates(NULL, settings);
    channel_manager_load_templates(manager, NULL);
    channel_manager_load_templates(NULL, NULL);

    obs_data_release(settings);
    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Load Templates - Empty Array
 * ======================================================================== */
static bool test_load_templates_empty_array(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    size_t initial_count = manager->template_count;

    /* Create settings with empty template array */
    obs_data_t *settings = obs_data_create();
    obs_data_array_t *empty_array = obs_data_array_create();
    obs_data_set_array(settings, "output_templates", empty_array);

    /* Load should succeed but add no templates */
    channel_manager_load_templates(manager, settings);
    ASSERT_EQ(manager->template_count, initial_count, "Template count should not change");

    obs_data_array_release(empty_array);
    obs_data_release(settings);
    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Load Templates - Missing Array
 * ======================================================================== */
static bool test_load_templates_missing_array(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    size_t initial_count = manager->template_count;

    /* Create settings without template array */
    obs_data_t *settings = obs_data_create();
    /* Don't set "output_templates" at all */

    /* Load should handle gracefully */
    channel_manager_load_templates(manager, settings);
    ASSERT_EQ(manager->template_count, initial_count, "Template count should not change");

    obs_data_release(settings);
    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Delete All Custom Templates
 * ======================================================================== */
static bool test_delete_all_custom_templates(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    size_t builtin_count = manager->template_count;

    /* Create multiple custom templates */
    encoding_settings_t encoding = channel_get_default_encoding();
    output_template_t *custom1 = channel_manager_create_template(
        manager, "Custom 1", SERVICE_YOUTUBE, ORIENTATION_HORIZONTAL, &encoding);
    output_template_t *custom2 = channel_manager_create_template(
        manager, "Custom 2", SERVICE_TWITCH, ORIENTATION_HORIZONTAL, &encoding);
    output_template_t *custom3 = channel_manager_create_template(
        manager, "Custom 3", SERVICE_FACEBOOK, ORIENTATION_HORIZONTAL, &encoding);

    ASSERT_EQ(manager->template_count, builtin_count + 3, "Should have 3 custom templates");

    /* Save template IDs before deletion */
    char *id1 = bstrdup(custom1->template_id);
    char *id2 = bstrdup(custom2->template_id);
    char *id3 = bstrdup(custom3->template_id);

    /* Delete all custom templates */
    bool del1 = channel_manager_delete_template(manager, id1);
    bool del2 = channel_manager_delete_template(manager, id2);
    bool del3 = channel_manager_delete_template(manager, id3);

    ASSERT_TRUE(del1, "Delete 1 should succeed");
    ASSERT_TRUE(del2, "Delete 2 should succeed");
    ASSERT_TRUE(del3, "Delete 3 should succeed");

    /* Verify only built-in templates remain */
    ASSERT_EQ(manager->template_count, builtin_count, "Should only have built-in templates");

    bfree(id1);
    bfree(id2);
    bfree(id3);
    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Built-in Templates Loaded Correctly
 * ======================================================================== */
static bool test_builtin_templates_loaded(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);

    /* Verify we have exactly 6 built-in templates */
    ASSERT_EQ(manager->template_count, 6, "Should have 6 built-in templates");

    /* Verify all templates are marked as built-in */
    for (size_t i = 0; i < 6; i++) {
        output_template_t *tmpl = manager->templates[i];
        ASSERT_NOT_NULL(tmpl, "Template should exist");
        ASSERT_TRUE(tmpl->is_builtin, "Template should be built-in");
        ASSERT_NOT_NULL(tmpl->template_name, "Template should have a name");
        ASSERT_NOT_NULL(tmpl->template_id, "Template should have an ID");
    }

    /* Verify template names contain expected patterns */
    bool found_youtube = false;
    bool found_twitch = false;
    bool found_facebook = false;

    for (size_t i = 0; i < 6; i++) {
        const char *name = manager->templates[i]->template_name;
        if (strstr(name, "YouTube")) found_youtube = true;
        if (strstr(name, "Twitch")) found_twitch = true;
        if (strstr(name, "Facebook")) found_facebook = true;
    }

    ASSERT_TRUE(found_youtube, "Should have YouTube templates");
    ASSERT_TRUE(found_twitch, "Should have Twitch templates");
    ASSERT_TRUE(found_facebook, "Should have Facebook templates");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test: Apply Multiple Templates to Same Channel
 * ======================================================================== */
static bool test_apply_multiple_templates(void) {
    restreamer_api_t *api = create_mock_api();
    channel_manager_t *manager = channel_manager_create(api);
    stream_channel_t *channel = channel_manager_create_channel(manager, "Multi-Output Channel");

    ASSERT_NOT_NULL(channel, "Channel should be created");

    /* Get different built-in templates */
    ASSERT_TRUE(manager->template_count >= 3, "Should have at least 3 templates");

    output_template_t *tmpl1 = manager->templates[0];
    output_template_t *tmpl2 = manager->templates[1];
    output_template_t *tmpl3 = manager->templates[2];

    /* Apply templates to channel */
    bool result1 = channel_apply_template(channel, tmpl1, "key1");
    bool result2 = channel_apply_template(channel, tmpl2, "key2");
    bool result3 = channel_apply_template(channel, tmpl3, "key3");

    ASSERT_TRUE(result1, "Apply template 1 should succeed");
    ASSERT_TRUE(result2, "Apply template 2 should succeed");
    ASSERT_TRUE(result3, "Apply template 3 should succeed");

    /* Verify channel has 3 outputs */
    ASSERT_EQ(channel->output_count, 3, "Channel should have 3 outputs");

    /* Verify each output matches its template */
    ASSERT_STR_EQ(channel->outputs[0].stream_key, "key1", "Output 1 key should match");
    ASSERT_STR_EQ(channel->outputs[1].stream_key, "key2", "Output 2 key should match");
    ASSERT_STR_EQ(channel->outputs[2].stream_key, "key3", "Output 3 key should match");

    channel_manager_destroy(manager);
    return true;
}

/* ========================================================================
 * Test Suite Runner for Integration with test_main.c
 * ======================================================================== */

/* Forward declarations for test_framework compatibility */
extern void test_suite_start(const char *name);
extern void test_suite_end(const char *name, bool result);
extern void test_start(const char *name);
extern void test_end(void);

bool run_channel_templates_tests(void) {
    test_suite_start("Channel Template Management Tests");

    bool result = true;

    /* Template Creation Tests */
    test_start("Create template - success");
    result &= test_create_template_success();
    test_end();

    test_start("Create template - NULL parameters");
    result &= test_create_template_null_params();
    test_end();

    /* Template Deletion Tests */
    test_start("Delete template - success");
    result &= test_delete_template_success();
    test_end();

    test_start("Delete template - built-in fails");
    result &= test_delete_template_builtin_fails();
    test_end();

    test_start("Delete template - not found");
    result &= test_delete_template_not_found();
    test_end();

    test_start("Delete all custom templates");
    result &= test_delete_all_custom_templates();
    test_end();

    /* Template Retrieval Tests */
    test_start("Get template by ID - success");
    result &= test_get_template_success();
    test_end();

    test_start("Get template by ID - not found");
    result &= test_get_template_not_found();
    test_end();

    test_start("Get template by index - success");
    result &= test_get_template_at_success();
    test_end();

    test_start("Get template by index - out of bounds");
    result &= test_get_template_at_out_of_bounds();
    test_end();

    /* Template Application Tests */
    test_start("Apply template - success");
    result &= test_apply_template_success();
    test_end();

    test_start("Apply template - NULL parameters");
    result &= test_apply_template_null_params();
    test_end();

    test_start("Apply multiple templates");
    result &= test_apply_multiple_templates();
    test_end();

    /* Template Persistence Tests */
    test_start("Save and load templates - round trip");
    result &= test_save_and_load_templates();
    test_end();

    test_start("Save templates - only custom");
    result &= test_save_templates_only_custom();
    test_end();

    test_start("Save templates - NULL parameters");
    result &= test_save_templates_null_params();
    test_end();

    test_start("Load templates - NULL parameters");
    result &= test_load_templates_null_params();
    test_end();

    test_start("Load templates - empty array");
    result &= test_load_templates_empty_array();
    test_end();

    test_start("Load templates - missing array");
    result &= test_load_templates_missing_array();
    test_end();

    /* Built-in Template Tests */
    test_start("Built-in templates loaded correctly");
    result &= test_builtin_templates_loaded();
    test_end();

    test_suite_end("Channel Template Management Tests", result);
    return result;
}
