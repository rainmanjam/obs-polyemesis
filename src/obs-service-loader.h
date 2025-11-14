#pragma once

#include <QList>
#include <QMap>
#include <QString>
#include <jansson.h>

/**
 * @brief Represents a streaming server for a service
 */
struct StreamingServer {
  QString name;
  QString url;
};

/**
 * @brief Represents a streaming service from OBS services.json
 */
struct StreamingService {
  QString name;
  bool common;
  QString stream_key_link;
  QList<StreamingServer> servers;
  QStringList supported_video_codecs;

  StreamingService() : common(false) {}
};

/**
 * @brief Loads and manages streaming services from OBS services.json
 */
class OBSServiceLoader {
public:
  OBSServiceLoader();
  ~OBSServiceLoader();

  /**
   * @brief Load services from OBS services.json
   * @return true if services were loaded successfully
   */
  bool loadServices();

  /**
   * @brief Get list of all loaded services
   * @return List of service names
   */
  QStringList getServiceNames() const;

  /**
   * @brief Get list of common services only
   * @return List of common service names
   */
  QStringList getCommonServiceNames() const;

  /**
   * @brief Get service by name
   * @param name Service name
   * @return Pointer to service or nullptr if not found
   */
  const StreamingService *getService(const QString &name) const;

  /**
   * @brief Get all services
   * @return List of all services
   */
  const QList<StreamingService> &getAllServices() const { return services; }

private:
  QList<StreamingService> services;
  QMap<QString, int> serviceIndexMap;

  /**
   * @brief Try to load services from a path
   * @param path Path to services.json
   * @return true if loaded successfully
   */
  bool tryLoadFromPath(const QString &path);

  /**
   * @brief Parse a service object from JSON
   * @param serviceObj JSON object representing a service
   * @return Parsed StreamingService
   */
  StreamingService parseService(json_t *serviceObj);
};
