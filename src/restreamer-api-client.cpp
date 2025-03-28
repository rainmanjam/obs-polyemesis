#include "restreamer-api-client.hpp"

#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QUrlQuery>
#include <QJsonDocument> // For potential JSON parsing
#include <QJsonObject>   // For potential JSON parsing

#include <obs-module.h> // For logging

RestreamerApiClient::RestreamerApiClient(QObject *parent) :
	QObject(parent), network_manager(new QNetworkAccessManager(this))
{
	// Connect the finished signal from the manager to our handler slot
	connect(network_manager, &QNetworkAccessManager::finished, this,
		&RestreamerApiClient::handleReply);
}

RestreamerApiClient::~RestreamerApiClient()
{
	// network_manager is automatically deleted due to parent-child relationship
}

void RestreamerApiClient::setBaseUrl(const QUrl &url)
{
	base_url = url;
	blog(LOG_INFO, "[Polyemesis API] Base URL set to: %s", url.toString().toStdString().c_str());
}

void RestreamerApiClient::setApiKey(const QString &key)
{
	api_key = key;
	// Avoid logging the actual key
	blog(LOG_INFO, "[Polyemesis API] API Key has been set.");
}

// Example API call implementation
void RestreamerApiClient::getStatus()
{
	if (base_url.isEmpty()) {
		emit errorOccurred("Base URL not set.");
		return;
	}

	// Construct the full URL for the status endpoint (adjust path as needed)
	QUrl status_url = base_url;
	status_url.setPath(status_url.path() + "/api/v1/status"); // Example path

	QNetworkRequest request(status_url);

	// Add API Key header (adjust header name based on Restreamer API docs)
	if (!api_key.isEmpty()) {
		request.setRawHeader("X-API-Key", api_key.toUtf8()); // Example header
	}

	blog(LOG_INFO, "[Polyemesis API] Requesting status from: %s", status_url.toString().toStdString().c_str());
	network_manager->get(request);
}

void RestreamerApiClient::startProcess(const QString &processId)
{
	if (base_url.isEmpty()) {
		emit errorOccurred("Base URL not set.", "startProcess");
		return;
	}

	// Construct URL: e.g., /api/v1/process/{processId}/start
	QUrl process_url = base_url;
	process_url.setPath(process_url.path() + QString("/api/v1/process/%1/start").arg(processId));

	QNetworkRequest request(process_url);
	if (!api_key.isEmpty()) {
		request.setRawHeader("X-API-Key", api_key.toUtf8());
	}
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json"); // Assuming POST needs this

	blog(LOG_INFO, "[Polyemesis API] Requesting start for process %s: %s",
	     processId.toStdString().c_str(), process_url.toString().toStdString().c_str());
	network_manager->post(request, QByteArray()); // Empty body for simple start trigger
}

void RestreamerApiClient::stopProcess(const QString &processId)
{
	if (base_url.isEmpty()) {
		emit errorOccurred("Base URL not set.", "stopProcess");
		return;
	}

	// Construct URL: e.g., /api/v1/process/{processId}/stop
	QUrl process_url = base_url;
	process_url.setPath(process_url.path() + QString("/api/v1/process/%1/stop").arg(processId));

	QNetworkRequest request(process_url);
	if (!api_key.isEmpty()) {
		request.setRawHeader("X-API-Key", api_key.toUtf8());
	}
	request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

	blog(LOG_INFO, "[Polyemesis API] Requesting stop for process %s: %s",
	     processId.toStdString().c_str(), process_url.toString().toStdString().c_str());
	network_manager->post(request, QByteArray()); // Empty body
}

void RestreamerApiClient::getMetrics()
{
	if (base_url.isEmpty()) {
		emit errorOccurred("Base URL not set.", "getMetrics");
		return;
	}

	// Construct URL: e.g., /api/v1/metrics or just /metrics
	QUrl metrics_url = base_url;
	metrics_url.setPath(metrics_url.path() + "/metrics"); // Adjust based on actual API

	QNetworkRequest request(metrics_url);
	if (!api_key.isEmpty()) {
		request.setRawHeader("X-API-Key", api_key.toUtf8());
	}

	blog(LOG_INFO, "[Polyemesis API] Requesting metrics from: %s", metrics_url.toString().toStdString().c_str());
	network_manager->get(request);
}


// Slot to handle network replies
void RestreamerApiClient::handleReply(QNetworkReply *reply)
{
	if (!reply) {
		return; // Should not happen
	}

	// Ensure reply is deleted later
	reply->deleteLater();

	QString endpoint = reply->request().url().path(); // Get path for context

	if (reply->error() != QNetworkReply::NoError) {
		QString error_string = QString("Network Error: %1").arg(reply->errorString());
		blog(LOG_WARNING, "[Polyemesis API] %s (URL: %s)",
		     error_string.toStdString().c_str(),
		     reply->request().url().toString().toStdString().c_str());
		emit errorOccurred(error_string, endpoint);
	} else {
		QByteArray response_data = reply->readAll();
		blog(LOG_INFO, "[Polyemesis API] Received reply for: %s", reply->request().url().toString().toStdString().c_str());

		// --- Differentiate responses based on endpoint ---
		// WARNING: This is basic; a better approach might map requests to replies
		if (endpoint.endsWith("/status")) {
			emit statusReceived(response_data);
		} else if (endpoint.endsWith("/start")) {
			// Assuming success if no error, extract processId if possible/needed
			// For simplicity, just emit signal. Real implementation might parse response.
			QString processId = endpoint.split('/').at(endpoint.count('/') - 2); // Basic extraction
			emit processStarted(processId);
		} else if (endpoint.endsWith("/stop")) {
			QString processId = endpoint.split('/').at(endpoint.count('/') - 2);
			emit processStopped(processId);
		} else if (endpoint.endsWith("/metrics")) {
			emit metricsReceived(response_data);
		} else {
			blog(LOG_WARNING, "[Polyemesis API] Received reply for unknown endpoint: %s", endpoint.toStdString().c_str());
		}
		// blog(LOG_DEBUG, "[Polyemesis API] Response Data: %s", response_data.constData());
	}
}
