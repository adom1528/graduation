#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QWebSocket>
#include <QListWidget>
#include <QMap>
#include <QTimer>
#include <QFileDialog>
#include <QNetworkAccessManager>
#include <QNetworkRequest>
#include <QNetworkReply>
#include <QHttpMultiPart>
#include <QFile>
#include <QFileInfo>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    void fetchFriendList();// 拉去好友列表
    void fetchChatHistory(QString friendId); // 拉取某个好友的记录
    // 责把所有类型的数据转化为富文本上屏
    void renderMessageToUI(int type, const QString &senderName, const QString &content, const QString &fileName, bool isSelf);

private slots:
    void onExitLoginClicked(); // 退出登录
    void onFriendItemClicked(QListWidgetItem *item); // 点击好友列表触发
    void onBtnAddFriendClicked(); // 点击添加好友
    void onSendImageClicked(); // 发送图片
    void onSendFileClicked(); // 发送文件

    // --- 新增 WebSocket 相关槽函数 ---
    void onConnected();                  // 连接成功时触发
    void onDisconnected(); // 断开连接时出触发
    void onTextMessageReceived(QString message); // 收到消息时触发
    void onSendClicked();                // 点击发送按钮时触发
    void onHeartbeatTimeout(); // 处理心跳超时的槽函数

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;

    // 新增：WebSocket 客户端指针
    QWebSocket *webSocket;
    QString myToken; // 存 Token 用
    QMap<QString, QString> friendMap; // 好友缓存字典（key: 雪花id， value: nickname）
    QTimer *heartbeatTimer; // 心跳定时器指针

};
#endif // MAINWINDOW_H
