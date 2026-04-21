#include "httpmanager.h"
#include <QNetworkRequest>
#include <QJsonParseError>

HttpManager* HttpManager::instance()
{
    // C++11 线程安全的魔法单例
    static HttpManager ins;
    return &ins;
}

HttpManager::HttpManager(QObject *parent) : QObject(parent)
{
    // 全局唯一的 Manager，极大节省内存
    m_manager = new QNetworkAccessManager(this);
}

HttpManager::~HttpManager() {}

void HttpManager::setToken(const QString& token) { m_token = token; }
QString HttpManager::getToken() const { return m_token; }

void HttpManager::postJson(const QString& url, const QJsonObject& data,
                           std::function<void(QJsonObject)> onSuccess,
                           std::function<void(QString)> onError)
{
    QNetworkRequest request((QUrl(url)));
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    // 核心拦截器：如果 Token 不为空，自动带上
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QJsonDocument doc(data);
    QByteArray postData = doc.toJson();

    QNetworkReply *reply = m_manager->post(request, postData);

    // C++11 Lambda 魔法：原地处理异步回调，不用去 h 文件写一大堆 slots 了
    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater(); // 防止内存泄漏

        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonParseError jsonError;
            QJsonDocument resDoc = QJsonDocument::fromJson(responseData, &jsonError);

            if (jsonError.error == QJsonParseError::NoError && resDoc.isObject()) {
                onSuccess(resDoc.object()); // 成功回调
            } else {
                onError("JSON 解析失败");
            }
        } else {
            // 处理 HTTP 状态码错误
            onError(reply->errorString());
        }
    });
}

void HttpManager::get(const QString& url,
                      std::function<void(QJsonObject)> onSuccess,
                      std::function<void(QString)> onError)
{
    QNetworkRequest request((QUrl(url)));

    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", ("Bearer " + m_token).toUtf8());
    }

    QNetworkReply *reply = m_manager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        reply->deleteLater();
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonParseError jsonError;
            QJsonDocument resDoc = QJsonDocument::fromJson(responseData, &jsonError);

            if (jsonError.error == QJsonParseError::NoError && resDoc.isObject()) {
                onSuccess(resDoc.object());
            } else {
                onError("JSON 解析失败");
            }
        } else {
            onError(reply->errorString());
        }
    });
}
