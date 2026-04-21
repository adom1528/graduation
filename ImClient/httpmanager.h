#ifndef HTTPMANAGER_H
#define HTTPMANAGER_H

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>
#include <QJsonDocument>
#include <functional>

class HttpManager : public QObject
{
    Q_OBJECT
public:
    // 1. 全局单例模式获取实例
    static HttpManager* instance();

    // 2. Token 的存取（登录成功后存入，后续所有请求自动带上）
    void setToken(const QString& token);
    QString getToken() const;

    // 3. 极其优雅的 POST JSON 封装 (使用 Lambda 回调)
    void postJson(const QString& url, const QJsonObject& data,
                  std::function<void(QJsonObject)> onSuccess,
                  std::function<void(QString)> onError);

    // 4. 极其优雅的 GET 封装
    void get(const QString& url,
             std::function<void(QJsonObject)> onSuccess,
             std::function<void(QString)> onError);

private:
    explicit HttpManager(QObject *parent = nullptr);
    ~HttpManager();

    QNetworkAccessManager *m_manager;
    QString m_token;
};

#endif // HTTPMANAGER_H
