#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonDocument>
#include <QJsonObject>
#include <QMessageBox>
#include <QWebSocket> // 新增：WebSocket 头文件

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // 登录按钮点击
    void onLoginClicked();

    // --- 新增 WebSocket 相关槽函数 ---
    void onConnected();                  // 连接成功时触发
    void onTextMessageReceived(QString message); // 收到消息时触发
    void onSendClicked();                // 点击发送按钮时触发

private:
    Ui::MainWindow *ui;
    QNetworkAccessManager *networkManager;

    // 新增：WebSocket 客户端指针
    QWebSocket *webSocket;
    QString myToken; // 存 Token 用
};
#endif // MAINWINDOW_H
