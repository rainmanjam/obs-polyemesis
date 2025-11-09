#include "restreamer-api.h"
#include "restreamer-config.h"
#include "restreamer-multistream.h"
#include <obs-module.h>
#include <obs-output.h>
#include <plugin-support.h>
#include <util/dstr.h>
#include <util/platform.h>
#include <util/threading.h>

struct restreamer_output {
  obs_output_t *output;
  restreamer_api_t *api;
  multistream_config_t *multistream_config;

  char *process_reference;
  bool is_multistream;
  bool active;

  pthread_t status_thread;
  bool status_thread_created;
  volatile bool stop_thread;
};

static const char *restreamer_output_getname(void *unused) {
  UNUSED_PARAMETER(unused);
  return "Restreamer Output";
}

static void *restreamer_output_create(obs_data_t *settings,
                                      obs_output_t *output) {
  struct restreamer_output *context = bzalloc(sizeof(struct restreamer_output));
  context->output = output;

  /* Create API connection */
  context->api = restreamer_config_create_global_api();

  /* Check if multistreaming is enabled */
  context->is_multistream = obs_data_get_bool(settings, "enable_multistream");

  if (context->is_multistream) {
    context->multistream_config = restreamer_multistream_create();
    restreamer_multistream_load_from_settings(context->multistream_config,
                                              settings);
  }

  obs_log(LOG_INFO, "Restreamer output created");

  return context;
}

static void restreamer_output_destroy(void *data) {
  struct restreamer_output *context = data;

  if (context->active) {
    context->stop_thread = true;
    if (context->status_thread_created) {
      pthread_join(context->status_thread, NULL);
      context->status_thread_created = false;
    }
  }

  if (context->multistream_config) {
    restreamer_multistream_destroy(context->multistream_config);
  }

  if (context->api) {
    restreamer_api_destroy(context->api);
  }

  bfree(context->process_reference);
  bfree(context);

  obs_log(LOG_INFO, "Restreamer output destroyed");
}

static bool restreamer_output_start(void *data) {
  struct restreamer_output *context = data;

  if (!context->api) {
    obs_log(LOG_ERROR, "Cannot start output: API connection not initialized");
    return false;
  }

  /* Test connection first */
  if (!restreamer_api_test_connection(context->api)) {
    obs_log(LOG_ERROR, "Cannot connect to restreamer: %s",
            restreamer_api_get_error(context->api));
    return false;
  }

  /* Get OBS output settings to build input URL */
  obs_service_t *service = obs_output_get_service(context->output);
  const char *rtmp_url = NULL;
  const char *stream_key = NULL;

  if (service) {
    obs_data_t *service_settings = obs_service_get_settings(service);
    rtmp_url = obs_data_get_string(service_settings, "server");
    stream_key = obs_data_get_string(service_settings, "key");
    obs_data_release(service_settings);
  }

  if (context->is_multistream && context->multistream_config) {
    /* Start multistreaming */
    struct dstr input_url;
    dstr_init(&input_url);

    /* We need to get the RTMP URL that OBS would normally stream to
       For now, we'll use a local RTMP server or SRT URL
       This is a placeholder - in production you'd set up an RTMP server
       or use SRT/other protocol */
    dstr_copy(&input_url, "rtmp://localhost/live/obs_input");

    /* Detect video orientation from OBS */
    video_t *video = obs_output_video(context->output);
    if (video) {
      const struct video_output_info *voi = video_output_get_info(video);
      stream_orientation_t orientation =
          restreamer_multistream_detect_orientation(voi->width, voi->height);
      context->multistream_config->source_orientation = orientation;
    }

    bool result = restreamer_multistream_start(
        context->api, context->multistream_config, input_url.array);

    dstr_free(&input_url);

    if (!result) {
      obs_log(LOG_ERROR, "Failed to start multistream: %s",
              restreamer_api_get_error(context->api));
      return false;
    }

    context->active = true;
    context->process_reference =
        bstrdup(context->multistream_config->process_reference);

    obs_log(LOG_INFO, "Multistream started successfully");
    obs_output_begin_data_capture(context->output, 0);

    return true;
  } else {
    /* Single stream mode - create a simple passthrough */
    if (!rtmp_url || !stream_key) {
      obs_log(LOG_ERROR, "No streaming service configured for output");
      return false;
    }

    struct dstr full_url;
    dstr_init(&full_url);
    dstr_copy(&full_url, rtmp_url);
    dstr_cat(&full_url, "/");
    dstr_cat(&full_url, stream_key);

    /* Create a single output process */
    const char *output_urls[] = {full_url.array};

    struct dstr reference;
    dstr_init(&reference);
    dstr_printf(&reference, "obs_output_%lld", (long long)os_gettime_ns());

    bool result = restreamer_api_create_process(
        context->api, reference.array, "rtmp://localhost/live/obs_input",
        output_urls, 1, NULL);

    if (result) {
      context->process_reference = bstrdup(reference.array);
      context->active = true;
      obs_output_begin_data_capture(context->output, 0);
    }

    dstr_free(&full_url);
    dstr_free(&reference);

    if (!result) {
      obs_log(LOG_ERROR, "Failed to start output: %s",
              restreamer_api_get_error(context->api));
      return false;
    }

    obs_log(LOG_INFO, "Restreamer output started");
    return true;
  }
}

static void restreamer_output_stop(void *data, uint64_t ts) {
  UNUSED_PARAMETER(ts);

  struct restreamer_output *context = data;

  if (!context->active) {
    return;
  }

  context->stop_thread = true;

  obs_output_end_data_capture(context->output);

  /* Stop the restreamer process */
  if (context->process_reference && context->api) {
    if (context->is_multistream) {
      restreamer_multistream_stop(context->api, context->process_reference);
    } else {
      restreamer_api_stop_process(context->api, context->process_reference);
    }
  }

  context->active = false;

  obs_log(LOG_INFO, "Restreamer output stopped");
}

static void restreamer_output_data(void *data, struct encoder_packet *packet) {
  UNUSED_PARAMETER(data);
  UNUSED_PARAMETER(packet);

  /* In a real implementation, this would send the encoded data
     to a local RTMP server that restreamer can pull from.
     For now, this is a placeholder. */
}

static void restreamer_output_defaults(obs_data_t *settings) {
  obs_data_set_default_bool(settings, "enable_multistream", false);
  obs_data_set_default_bool(settings, "auto_detect_orientation", true);
}

static bool add_destination_clicked(obs_properties_t *props,
                                    obs_property_t *property, void *data) {
  UNUSED_PARAMETER(props);
  UNUSED_PARAMETER(property);
  UNUSED_PARAMETER(data);

  /* This would open a dialog to add a new destination
     For now, it's a placeholder */
  return true;
}

static obs_properties_t *restreamer_output_properties(void *data) {
  UNUSED_PARAMETER(data);

  obs_properties_t *props = obs_properties_create();

  obs_properties_add_bool(props, "enable_multistream", "Enable Multistreaming");

  obs_properties_add_bool(props, "auto_detect_orientation",
                          "Auto-detect Video Orientation");

  obs_property_t *orientation =
      obs_properties_add_list(props, "source_orientation", "Force Orientation",
                              OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);

  obs_property_list_add_int(orientation, "Auto Detect", ORIENTATION_AUTO);
  obs_property_list_add_int(orientation, "Horizontal (Landscape)",
                            ORIENTATION_HORIZONTAL);
  obs_property_list_add_int(orientation, "Vertical (Portrait)",
                            ORIENTATION_VERTICAL);
  obs_property_list_add_int(orientation, "Square", ORIENTATION_SQUARE);

  obs_properties_add_button(props, "add_destination",
                            "Add Streaming Destination",
                            add_destination_clicked);

  obs_properties_add_text(
      props, "destinations_info",
      "Configure destinations in the Restreamer Control Panel", OBS_TEXT_INFO);

  return props;
}

struct obs_output_info restreamer_output_info = {
    .id = "restreamer_output",
    .flags = OBS_OUTPUT_AV | OBS_OUTPUT_ENCODED | OBS_OUTPUT_MULTI_TRACK |
             OBS_OUTPUT_SERVICE,
    .get_name = restreamer_output_getname,
    .create = restreamer_output_create,
    .destroy = restreamer_output_destroy,
    .start = restreamer_output_start,
    .stop = restreamer_output_stop,
    .encoded_packet = restreamer_output_data,
    .get_defaults = restreamer_output_defaults,
    .get_properties = restreamer_output_properties,
};
