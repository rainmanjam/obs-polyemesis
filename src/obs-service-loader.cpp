#include "obs-service-loader.h"
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QStandardPaths>

extern "C" {
#include <obs-module.h>
#include <plugin-support.h>
}

OBSServiceLoader::OBSServiceLoader() { loadServices(); }

OBSServiceLoader::~OBSServiceLoader() {}

bool OBSServiceLoader::loadServices() {
  services.clear();
  serviceIndexMap.clear();

  // Try multiple possible locations for services.json
  QStringList possiblePaths;

  // 1. OBS application bundle (macOS)
#ifdef __APPLE__
  possiblePaths << "/Applications/OBS.app/Contents/PlugIns/"
                   "rtmp-services.plugin/Contents/Resources/services.json";
#endif

  // 2. User config directory (custom services)
  QString userConfig =
      QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
  userConfig.replace("obs-polyemesis", "obs-studio");
  possiblePaths << userConfig + "/plugin_config/rtmp-services/services.json";

  // 3. System-wide location (Linux)
#ifdef __linux__
  possiblePaths << "/usr/share/obs/obs-plugins/rtmp-services/services.json";
  possiblePaths
      << "/usr/local/share/obs/obs-plugins/rtmp-services/services.json";
#endif

  // 4. Windows locations
#ifdef _WIN32
  QString programFiles = qEnvironmentVariable("ProgramFiles");
  possiblePaths
      << programFiles +
             "/obs-studio/data/obs-plugins/rtmp-services/services.json";
#endif

  // Try each path until we find one that works
  // cppcheck-suppress useStlAlgorithm
  for (const QString &path : possiblePaths) {
    if (tryLoadFromPath(path)) {
      obs_log(LOG_INFO, "[OBS Service Loader] Loaded services from: %s",
              path.toUtf8().constData());
      return true;
    }
  }

  obs_log(LOG_WARNING, "[OBS Service Loader] Could not find services.json in "
                       "any expected location");
  return false;
}

bool OBSServiceLoader::tryLoadFromPath(const QString &path) {
  QFile file(path);
  if (!file.exists() || !file.open(QIODevice::ReadOnly)) {
    return false;
  }

  QByteArray jsonData = file.readAll();
  file.close();

  json_error_t error;
  json_t *root = json_loads(jsonData.constData(), 0, &error);
  if (!root) {
    obs_log(LOG_WARNING, "[OBS Service Loader] Failed to parse JSON: %s",
            error.text);
    return false;
  }

  json_t *servicesArray = json_object_get(root, "services");
  if (!servicesArray || !json_is_array(servicesArray)) {
    json_decref(root);
    return false;
  }

  size_t index;
  json_t *serviceObj;
  json_array_foreach(servicesArray, index, serviceObj) {
    StreamingService service = parseService(serviceObj);
    if (!service.name.isEmpty() && !service.servers.isEmpty()) {
      serviceIndexMap[service.name] = services.size();
      services.append(service);
    }
  }

  json_decref(root);
  return !services.isEmpty();
}

StreamingService OBSServiceLoader::parseService(json_t *serviceObj) {
  StreamingService service;

  // Parse name
  json_t *nameObj = json_object_get(serviceObj, "name");
  if (nameObj && json_is_string(nameObj)) {
    service.name = QString::fromUtf8(json_string_value(nameObj));
  }

  // Parse common flag
  json_t *commonObj = json_object_get(serviceObj, "common");
  if (commonObj && json_is_boolean(commonObj)) {
    service.common = json_boolean_value(commonObj);
  }

  // Parse stream key link
  json_t *keyLinkObj = json_object_get(serviceObj, "stream_key_link");
  if (keyLinkObj && json_is_string(keyLinkObj)) {
    service.stream_key_link = QString::fromUtf8(json_string_value(keyLinkObj));
  }

  // Parse servers
  json_t *serversArray = json_object_get(serviceObj, "servers");
  if (serversArray && json_is_array(serversArray)) {
    size_t index;
    json_t *serverObj;
    json_array_foreach(serversArray, index, serverObj) {
      StreamingServer server;

      json_t *serverNameObj = json_object_get(serverObj, "name");
      if (serverNameObj && json_is_string(serverNameObj)) {
        server.name = QString::fromUtf8(json_string_value(serverNameObj));
      }

      json_t *urlObj = json_object_get(serverObj, "url");
      if (urlObj && json_is_string(urlObj)) {
        server.url = QString::fromUtf8(json_string_value(urlObj));
      }

      if (!server.name.isEmpty() && !server.url.isEmpty()) {
        service.servers.append(server);
      }
    }
  }

  // Parse supported video codecs
  json_t *codecsArray = json_object_get(serviceObj, "supported video codecs");
  if (codecsArray && json_is_array(codecsArray)) {
    size_t index;
    json_t *codecObj;
    json_array_foreach(codecsArray, index, codecObj) {
      if (json_is_string(codecObj)) {
        service.supported_video_codecs.append(
            QString::fromUtf8(json_string_value(codecObj)));
      }
    }
  }

  return service;
}

QStringList OBSServiceLoader::getServiceNames() const {
  QStringList names;
  for (const StreamingService &service : services) {
    names.append(service.name);
  }
  return names;
}

QStringList OBSServiceLoader::getCommonServiceNames() const {
  QStringList names;
  for (const StreamingService &service : services) {
    if (service.common) {
      names.append(service.name);
    }
  }
  return names;
}

const StreamingService *
OBSServiceLoader::getService(const QString &name) const {
  if (serviceIndexMap.contains(name)) {
    return &services[serviceIndexMap[name]];
  }
  return nullptr;
}
