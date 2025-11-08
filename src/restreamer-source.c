#include "restreamer-api.h"
#include "restreamer-config.h"
#include <obs-module.h>
#include <plugin-support.h>
#include <media-io/video-io.h>
#include <util/platform.h>
#include <util/threading.h>

struct restreamer_source {
	obs_source_t *source;
	restreamer_api_t *api;
	restreamer_connection_t *connection;

	char *process_id;
	char *stream_url;

	pthread_t monitoring_thread;
	bool thread_running;
	volatile bool stop_monitoring;

	obs_source_t *media_source;
};

static const char *restreamer_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return "Restreamer Stream";
}

static void *restreamer_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct restreamer_source *context =
		bzalloc(sizeof(struct restreamer_source));
	context->source = source;

	/* Load connection settings */
	bool use_global = obs_data_get_bool(settings, "use_global_connection");

	if (use_global) {
		context->connection = restreamer_config_get_global_connection();
		context->api = restreamer_config_create_global_api();
	} else {
		context->connection =
			restreamer_config_load_from_settings(settings);
		if (context->connection) {
			context->api =
				restreamer_api_create(context->connection);
		}
	}

	/* Load stream settings */
	const char *process_id = obs_data_get_string(settings, "process_id");
	if (process_id && strlen(process_id) > 0) {
		context->process_id = bstrdup(process_id);
	}

	const char *stream_url = obs_data_get_string(settings, "stream_url");
	if (stream_url && strlen(stream_url) > 0) {
		context->stream_url = bstrdup(stream_url);
	}

	obs_log(LOG_INFO, "Restreamer source created");

	return context;
}

static void restreamer_source_destroy(void *data)
{
	struct restreamer_source *context = data;

	if (context->thread_running) {
		context->stop_monitoring = true;
		pthread_join(context->monitoring_thread, NULL);
	}

	if (context->media_source) {
		obs_source_release(context->media_source);
	}

	if (context->api) {
		restreamer_api_destroy(context->api);
	}

	if (context->connection) {
		restreamer_config_free_connection(context->connection);
	}

	bfree(context->process_id);
	bfree(context->stream_url);
	bfree(context);

	obs_log(LOG_INFO, "Restreamer source destroyed");
}

static void restreamer_source_update(void *data, obs_data_t *settings)
{
	struct restreamer_source *context = data;

	/* Update connection if needed */
	bool use_global = obs_data_get_bool(settings, "use_global_connection");

	if (use_global) {
		if (context->connection) {
			restreamer_config_free_connection(context->connection);
		}
		context->connection = restreamer_config_get_global_connection();

		if (context->api) {
			restreamer_api_destroy(context->api);
		}
		context->api = restreamer_config_create_global_api();
	} else {
		if (context->connection) {
			restreamer_config_free_connection(context->connection);
		}
		context->connection =
			restreamer_config_load_from_settings(settings);

		if (context->api) {
			restreamer_api_destroy(context->api);
		}
		if (context->connection) {
			context->api =
				restreamer_api_create(context->connection);
		}
	}

	/* Update stream settings */
	bfree(context->process_id);
	bfree(context->stream_url);

	const char *process_id = obs_data_get_string(settings, "process_id");
	context->process_id =
		(process_id && strlen(process_id) > 0) ? bstrdup(process_id) : NULL;

	const char *stream_url = obs_data_get_string(settings, "stream_url");
	context->stream_url =
		(stream_url && strlen(stream_url) > 0) ? bstrdup(stream_url) : NULL;
}

static void restreamer_source_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "use_global_connection", true);
	obs_data_set_default_string(settings, "host", "localhost");
	obs_data_set_default_int(settings, "port", 8080);
	obs_data_set_default_bool(settings, "use_https", false);
}

static bool refresh_processes_clicked(obs_properties_t *props, obs_property_t *property,
				      void *data)
{
	UNUSED_PARAMETER(property);
	UNUSED_PARAMETER(data);

	obs_property_t *process_list = obs_properties_get(props, "process_id");
	obs_property_list_clear(process_list);

	obs_property_list_add_string(process_list, "Select a process...", "");

	/* Get global API to fetch processes */
	restreamer_api_t *api = restreamer_config_create_global_api();
	if (api) {
		restreamer_process_list_t list = {0};
		if (restreamer_api_get_processes(api, &list)) {
			for (size_t i = 0; i < list.count; i++) {
				const char *name = list.processes[i].reference
							   ? list.processes[i]
								     .reference
							   : list.processes[i].id;
				obs_property_list_add_string(
					process_list, name,
					list.processes[i].id);
			}
			restreamer_api_free_process_list(&list);
		}
		restreamer_api_destroy(api);
	}

	return true;
}

static obs_properties_t *restreamer_source_get_properties(void *data)
{
	UNUSED_PARAMETER(data);

	obs_properties_t *props = obs_properties_create();

	obs_properties_add_bool(props, "use_global_connection",
				"Use Global Connection Settings");

	obs_properties_add_text(props, "host", "Restreamer Host",
				OBS_TEXT_DEFAULT);
	obs_properties_add_int(props, "port", "Port", 1, 65535, 1);
	obs_properties_add_bool(props, "use_https", "Use HTTPS");
	obs_properties_add_text(props, "username", "Username (optional)",
				OBS_TEXT_DEFAULT);
	obs_properties_add_text(props, "password", "Password (optional)",
				OBS_TEXT_PASSWORD);

	/* Process selection */
	obs_property_t *process_list = obs_properties_add_list(
		props, "process_id", "Restreamer Process",
		OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_STRING);

	obs_property_list_add_string(process_list, "Select a process...", "");

	obs_properties_add_button(props, "refresh_processes",
				  "Refresh Process List",
				  refresh_processes_clicked);

	/* Direct stream URL */
	obs_properties_add_text(props, "stream_url",
				"Or enter stream URL directly",
				OBS_TEXT_DEFAULT);

	return props;
}

static void restreamer_source_video_render(void *data, gs_effect_t *effect)
{
	struct restreamer_source *context = data;

	if (context->media_source) {
		obs_source_video_render(context->media_source);
	} else {
		UNUSED_PARAMETER(effect);
	}
}

static uint32_t restreamer_source_get_width(void *data)
{
	struct restreamer_source *context = data;

	if (context->media_source) {
		return obs_source_get_width(context->media_source);
	}

	return 0;
}

static uint32_t restreamer_source_get_height(void *data)
{
	struct restreamer_source *context = data;

	if (context->media_source) {
		return obs_source_get_height(context->media_source);
	}

	return 0;
}

struct obs_source_info restreamer_source_info = {
	.id = "restreamer_source",
	.type = OBS_SOURCE_TYPE_INPUT,
	.output_flags = OBS_SOURCE_ASYNC_VIDEO | OBS_SOURCE_AUDIO,
	.get_name = restreamer_source_get_name,
	.create = restreamer_source_create,
	.destroy = restreamer_source_destroy,
	.update = restreamer_source_update,
	.get_defaults = restreamer_source_get_defaults,
	.get_properties = restreamer_source_get_properties,
	.video_render = restreamer_source_video_render,
	.get_width = restreamer_source_get_width,
	.get_height = restreamer_source_get_height,
};
