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
    // 全局单例模式获取实例
    static HttpManager* instance();

    // Token 的存取（登录成功后存入，后续所有请求自动带上）
    void setToken(const QString& token);
    QString getToken() const;

    // 极其优雅的 POST JSON 封装
    void postJson(const QString& url, const QJsonObject& data,
                  std::function<void(QJsonObject)> onSuccess,
                  std::function<void(QString)> onError);

    /**
     * @brief uploadFile 文件上传
     * @param url
     * @param filePath
     * @param onSuccess
     * @param onError
     */
    void uploadFile(const QString& url,
                    const QString& filePath,
                    std::function<void(QJsonObject)> onSuccess,
                    std::function<void(QString)> onError);

    /**
     * @brief get 不带参数的get请求
     * @param url
     * @param onSuccess
     * @param onError
     */
    void get(const QString& url,
             std::function<void(QJsonObject)> onSuccess,
             std::function<void(QString)> onError);

    /**
     * @brief get 带参数的get请求
     * @param url
     * @param params 后续参数,可有可无
     * @param onSuccess 成功处理后执行
     * @param onError 处理失败后执行
     */
    void get(const QString& url,
             const QVariantMap& params = QVariantMap(),
             std::function<void(QJsonObject)> onSuccess = nullptr,
             std::function<void(QString)> onError = nullptr);

    /**
     * @brief getBytes 专门获取图片的请求
     * @param url
     * @param onSuccess
     * @param onError
     */
    void getBytes(const QString& url,
                  std::function<void(QByteArray)> onSuccess,
                  std::function<void(QString)> onError);

private:
    explicit HttpManager(QObject *parent = nullptr);
    ~HttpManager();

    QNetworkAccessManager *m_manager;
    QString m_token;
};

#endif // HTTPMANAGER_H
