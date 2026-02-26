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

// 收到消息
void MainWindow::onTextMessageReceived(QString message)
{
    // 尝试解析 JSON
    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());

    if (!doc.isNull() && doc.isObject()) {
        // --- 是正常的聊天消息 ---
        QJsonObject obj = doc.object();

        // 提取信息
        // 注意：后端 Long 转成 JSON 可能是数字，Qt 解析时要注意
        // 建议后端 Message 里 fromUserId 最好是 String 类型兼容性更好，或者这里用 toVariant().toLongLong()
        long long fromUid = obj["fromUserId"].toVariant().toLongLong();
        QString content = obj["content"].toString();

        QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
        ui->textLog->append(QString("[%1] 用户%2: %3")
                                .arg(time)
                                .arg(fromUid)
                                .arg(content));
    } else {
        // --- 可能是系统消息 (比如“对方不在线”这种纯文本) ---
        ui->textLog->append(">> 系统: " + message);
    }
}

// 发送按钮点击
void MainWindow::onSendClicked()
{
    QString msgContent = ui->editMsg->text().trimmed();
    QString targetIdStr = ui->editTargetId->text().trimmed();

    if (msgContent.isEmpty() || targetIdStr.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入内容和对方ID");
        return;
    }

    // --- 构造 JSON ---
    QJsonObject json;
    json["toUserId"] = targetIdStr.toLongLong(); // 必须转成数字，匹配后端的 Long
    json["content"] = msgContent;
    json["type"] = 1; // 单聊

    // 转成字符串
    QJsonDocument doc(json);
    QString jsonString = doc.toJson(QJsonDocument::Compact);

    // 发送
    webSocket->sendTextMessage(jsonString);

    // 清空输入框
    ui->editMsg->clear();

    // 本地显示 (模拟微信，自己发的立刻上屏)
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->textLog->append(QString("[%1] 我 -> %2: %3")
                            .arg(time)
                            .arg(targetIdStr)
                            .arg(msgContent));
}
