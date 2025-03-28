#pragma once

#include <QObject>
#include <QString>
#include <QUrl>

// Forward declarations
class QNetworkAccessManager;
class QNetworkReply;

class RestreamerApiClient : public QObject {
	Q_OBJECT

public:
	explicit RestreamerApiClient(QObject *parent = nullptr);
	~RestreamerApiClient();

	void setBaseUrl(const QUrl &url);
	void setApiKey(const QString &key); // Assuming API key auth for now

	// --- API Calls ---
	void getStatus(); // General status
	void startProcess(const QString &processId);
	void stopProcess(const QString &processId);
	void getMetrics(); // Prometheus metrics

signals:
	// --- API Responses ---
	void statusReceived(const QByteArray &data);
	void processStarted(const QString &processId);
	void processStopped(const QString &processId);
	void metricsReceived(const QByteArray &data);
	void errorOccurred(const QString &errorString, const QString &endpoint = ""); // Add endpoint context

private slots:
	void handleReply(QNetworkReply *reply);

private:
	QUrl base_url;
	QString api_key;
	QNetworkAccessManager *network_manager;
};
