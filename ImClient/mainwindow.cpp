#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QUrlQuery>
#include <QDateTime> // 用于显示时间

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // 1. 初始化界面状态
    ui->boxChat->hide();  // 一开始隐藏聊天框
    ui->boxLogin->show(); // 显示登录框

    // 2. 初始化网络
    networkManager = new QNetworkAccessManager(this);

    // 3. 初始化 WebSocket
    webSocket = new QWebSocket();

    // 4. 信号槽连接
    connect(ui->btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginClicked);
    connect(ui->btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked);

    // WebSocket 信号
    connect(webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
}

MainWindow::~MainWindow()
{
    delete ui;
    // webSocket 和 networkManager 指定了 parent，会自动回收，不手动 delete 也行
}

// 登录逻辑
void MainWindow::onLoginClicked()
{
    QString username = ui->editUsername->text().trimmed();
    QString password = ui->editPassword->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "账号密码不能为空");
        return;
    }

    // 这里填你验证通过的那个 URL
    QUrl url("http://localhost:9000/im-auth/auth/login");

    QNetworkRequest request(url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    QUrlQuery params;
    params.addQueryItem("username", username);
    params.addQueryItem("password", password);
    QByteArray data = params.toString(QUrl::FullyEncoded).toUtf8();

    QNetworkReply *reply = networkManager->post(request, data);

    connect(reply, &QNetworkReply::finished, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject jsonObj = jsonDoc.object();

            if (jsonObj.contains("code") && jsonObj["code"].toInt() == 200) {
                // --- 登录成功核心逻辑 ---

                // 1. 保存 Token
                this->myToken = jsonObj["data"].toString();

                // 2. 界面切换
                ui->boxLogin->hide();
                ui->boxChat->show();

                // 3. 开始连接 WebSocket！
                // URL 格式: ws://localhost:9000/im?token=xxxxx
                QUrl wsUrl("ws://localhost:9000/im");
                QUrlQuery query;
                query.addQueryItem("token", myToken); // 把 Token 带在 URL 里
                wsUrl.setQuery(query);

                ui->textLog->append("正在连接服务器...");
                webSocket->open(wsUrl);

            } else {
                QMessageBox::critical(this, "登录失败", jsonObj["message"].toString());
            }
        } else {
            QMessageBox::critical(this, "网络错误", reply->errorString());
        }
        reply->deleteLater();
    });
}

// WebSocket 连接成功
void MainWindow::onConnected()
{
    ui->textLog->append(">> 服务器连接成功！");
}

// 收到消息 (服务器的回声)
void MainWindow::onTextMessageReceived(QString message)
{
    // 显示在日志框里
    ui->textLog->append(message);
}

// 发送按钮点击
void MainWindow::onSendClicked()
{
    QString msg = ui->editMsg->text().trimmed();
    if (msg.isEmpty()) return;

    // 发送给服务器
    webSocket->sendTextMessage(msg);

    // 清空输入框
    ui->editMsg->clear();

    // 自己发的消息也先显示一下（模拟微信的效果）
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->textLog->append("[" + time + "] 我: " + msg);
}
