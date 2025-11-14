#pragma once

#include <obs-frontend-api.h>
#include <obs.h>

/**
 * RAII wrapper for obs_source_t
 * Automatically releases source when going out of scope
 */
class OBSSourceAutoRelease {
public:
  obs_source_t *source;

  explicit OBSSourceAutoRelease(obs_source_t *src) : source(src) {}

  ~OBSSourceAutoRelease() {
    if (source)
      obs_source_release(source);
  }

  // Prevent copy to avoid double-free
  OBSSourceAutoRelease(const OBSSourceAutoRelease &) = delete;
  OBSSourceAutoRelease &operator=(const OBSSourceAutoRelease &) = delete;

  // Allow move semantics
  OBSSourceAutoRelease(OBSSourceAutoRelease &&other) noexcept
      : source(other.source) {
    other.source = nullptr;
  }

  OBSSourceAutoRelease &operator=(OBSSourceAutoRelease &&other) noexcept {
    if (this != &other) {
      if (source)
        obs_source_release(source);
      source = other.source;
      other.source = nullptr;
    }
    return *this;
  }

  operator obs_source_t *() { return source; }
  operator bool() const { return source != nullptr; }
  obs_source_t *get() { return source; }
};

/**
 * RAII wrapper for obs_data_t
 * Automatically releases data when going out of scope
 */
class OBSDataAutoRelease {
public:
  obs_data_t *data;

  explicit OBSDataAutoRelease(obs_data_t *d) : data(d) {}

  ~OBSDataAutoRelease() {
    if (data)
      obs_data_release(data);
  }

  OBSDataAutoRelease(const OBSDataAutoRelease &) = delete;
  OBSDataAutoRelease &operator=(const OBSDataAutoRelease &) = delete;

  OBSDataAutoRelease(OBSDataAutoRelease &&other) noexcept : data(other.data) {
    other.data = nullptr;
  }

  OBSDataAutoRelease &operator=(OBSDataAutoRelease &&other) noexcept {
    if (this != &other) {
      if (data)
        obs_data_release(data);
      data = other.data;
      other.data = nullptr;
    }
    return *this;
  }

  operator obs_data_t *() { return data; }
  operator bool() const { return data != nullptr; }
  obs_data_t *get() { return data; }
};

/**
 * RAII wrapper for obs_data_array_t
 * Automatically releases array when going out of scope
 */
class OBSDataArrayAutoRelease {
public:
  obs_data_array_t *array;

  explicit OBSDataArrayAutoRelease(obs_data_array_t *arr) : array(arr) {}

  ~OBSDataArrayAutoRelease() {
    if (array)
      obs_data_array_release(array);
  }

  OBSDataArrayAutoRelease(const OBSDataArrayAutoRelease &) = delete;
  OBSDataArrayAutoRelease &operator=(const OBSDataArrayAutoRelease &) = delete;

  OBSDataArrayAutoRelease(OBSDataArrayAutoRelease &&other) noexcept
      : array(other.array) {
    other.array = nullptr;
  }

  OBSDataArrayAutoRelease &operator=(OBSDataArrayAutoRelease &&other) noexcept {
    if (this != &other) {
      if (array)
        obs_data_array_release(array);
      array = other.array;
      other.array = nullptr;
    }
    return *this;
  }

  operator obs_data_array_t *() { return array; }
  operator bool() const { return array != nullptr; }
  obs_data_array_t *get() { return array; }
};

/**
 * RAII wrapper for obs_output_t
 * Automatically releases output when going out of scope
 */
class OBSOutputAutoRelease {
public:
  obs_output_t *output;

  explicit OBSOutputAutoRelease(obs_output_t *out) : output(out) {}

  ~OBSOutputAutoRelease() {
    if (output)
      obs_output_release(output);
  }

  OBSOutputAutoRelease(const OBSOutputAutoRelease &) = delete;
  OBSOutputAutoRelease &operator=(const OBSOutputAutoRelease &) = delete;

  OBSOutputAutoRelease(OBSOutputAutoRelease &&other) noexcept
      : output(other.output) {
    other.output = nullptr;
  }

  OBSOutputAutoRelease &operator=(OBSOutputAutoRelease &&other) noexcept {
    if (this != &other) {
      if (output)
        obs_output_release(output);
      output = other.output;
      other.output = nullptr;
    }
    return *this;
  }

  operator obs_output_t *() { return output; }
  operator bool() const { return output != nullptr; }
  obs_output_t *get() { return output; }
};

/**
 * RAII wrapper for obs_encoder_t
 * Automatically releases encoder when going out of scope
 */
class OBSEncoderAutoRelease {
public:
  obs_encoder_t *encoder;

  explicit OBSEncoderAutoRelease(obs_encoder_t *enc) : encoder(enc) {}

  ~OBSEncoderAutoRelease() {
    if (encoder)
      obs_encoder_release(encoder);
  }

  OBSEncoderAutoRelease(const OBSEncoderAutoRelease &) = delete;
  OBSEncoderAutoRelease &operator=(const OBSEncoderAutoRelease &) = delete;

  OBSEncoderAutoRelease(OBSEncoderAutoRelease &&other) noexcept
      : encoder(other.encoder) {
    other.encoder = nullptr;
  }

  OBSEncoderAutoRelease &operator=(OBSEncoderAutoRelease &&other) noexcept {
    if (this != &other) {
      if (encoder)
        obs_encoder_release(encoder);
      encoder = other.encoder;
      other.encoder = nullptr;
    }
    return *this;
  }

  operator obs_encoder_t *() { return encoder; }
  operator bool() const { return encoder != nullptr; }
  obs_encoder_t *get() { return encoder; }
};

/**
 * RAII wrapper for obs_service_t
 * Automatically releases service when going out of scope
 */
class OBSServiceAutoRelease {
public:
  obs_service_t *service;

  explicit OBSServiceAutoRelease(obs_service_t *svc) : service(svc) {}

  ~OBSServiceAutoRelease() {
    if (service)
      obs_service_release(service);
  }

  OBSServiceAutoRelease(const OBSServiceAutoRelease &) = delete;
  OBSServiceAutoRelease &operator=(const OBSServiceAutoRelease &) = delete;

  OBSServiceAutoRelease(OBSServiceAutoRelease &&other) noexcept
      : service(other.service) {
    other.service = nullptr;
  }

  OBSServiceAutoRelease &operator=(OBSServiceAutoRelease &&other) noexcept {
    if (this != &other) {
      if (service)
        obs_service_release(service);
      service = other.service;
      other.service = nullptr;
    }
    return *this;
  }

  operator obs_service_t *() { return service; }
  operator bool() const { return service != nullptr; }
  obs_service_t *get() { return service; }
};

/**
 * RAII wrapper for obs_properties_t
 * Automatically destroys properties when going out of scope
 */
class OBSPropertiesAutoDestroy {
public:
  obs_properties_t *props;

  explicit OBSPropertiesAutoDestroy(obs_properties_t *p) : props(p) {}

  ~OBSPropertiesAutoDestroy() {
    if (props)
      obs_properties_destroy(props);
  }

  OBSPropertiesAutoDestroy(const OBSPropertiesAutoDestroy &) = delete;
  OBSPropertiesAutoDestroy &
  operator=(const OBSPropertiesAutoDestroy &) = delete;

  OBSPropertiesAutoDestroy(OBSPropertiesAutoDestroy &&other) noexcept
      : props(other.props) {
    other.props = nullptr;
  }

  OBSPropertiesAutoDestroy &
  operator=(OBSPropertiesAutoDestroy &&other) noexcept {
    if (this != &other) {
      if (props)
        obs_properties_destroy(props);
      props = other.props;
      other.props = nullptr;
    }
    return *this;
  }

  operator obs_properties_t *() { return props; }
  operator bool() const { return props != nullptr; }
  obs_properties_t *get() { return props; }
};
