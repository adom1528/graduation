#include "mainwindow.h"
#include "./ui_mainwindow.h"
#include <QUrlQuery>
#include <QDateTime> // 用于显示时间
#include <QJsonArray>
#include <QDebug>

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
    connect(ui->btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginClicked); // 绑定登录按钮
    connect(ui->btnRegister, &QPushButton::clicked, this, &MainWindow::onRegisterClicked);
    connect(ui->btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked); // 绑定发送信息
    connect(ui->listFriends, &QListWidget::itemClicked, this, &MainWindow::onFriendItemClicked); // 绑定好友列表点击事件
    connect(ui->btnAddFriend, &QPushButton::clicked, this, &MainWindow::onBtnAddFriendClicked); // 添加好友按钮

    // WebSocket 信号
    connect(webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
}

// 好友id获取
void MainWindow::onFriendItemClicked(QListWidgetItem *item)
{
    // 从被点击的 item 中提取我们刚才悄悄藏进去的雪花 ID
    QString friendId = item->data(Qt::UserRole).toString();
    //qDebug() << friendId;

    // 自动把它填入你的目标ID输入框
    ui->editTargetId->setText(friendId);

    // 日志里提示
    ui->textLog->append(QString(">> 当前正在与【%1】聊天").arg(item->text()));
}

MainWindow::~MainWindow()
{
    delete ui;
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

                // 1. 保存 Token,拉取好友列表
                this->myToken = jsonObj["data"].toString();
                fetchFriendList();

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

//注册逻辑
void MainWindow::onRegisterClicked()
{
    QString username = ui->editUsername->text().trimmed();
    QString password = ui->editPassword->text().trimmed();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "注册账号和密码不能为空");
        return;
    }

    // 防连点保护
    ui->btnRegister->setEnabled(false);

    // 1. 组装 URL（走网关）
    QUrl url("http://localhost:9000/im-auth/auth/register");
    QNetworkRequest request(url);

    // 2. 设置表单提交格式
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");

    // 3. 组装表单参数
    QUrlQuery params;
    params.addQueryItem("username", username);
    params.addQueryItem("password", password);
    QByteArray data = params.toString(QUrl::FullyEncoded).toUtf8();

    // 4. 发射 POST 请求
    QNetworkReply *reply = networkManager->post(request, data);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument jsonDoc = QJsonDocument::fromJson(responseData);
            QJsonObject jsonObj = jsonDoc.object();

            if (jsonObj.contains("code") && jsonObj["code"].toInt() == 200) {
                // 注册成功！
                QMessageBox::information(this, "注册成功", "账号注册成功，请直接点击登录！");
                // 贴心小细节：不需要清空输入框，这样用户点了“确定”后直接点“登录”就能进系统，丝滑！
            } else {
                // 后端返回的业务错误（比如：用户名已存在）
                QMessageBox::critical(this, "注册失败", jsonObj["message"].toString());
            }
        } else {
            // 网络层面的错误（比如后端没开）
            QMessageBox::critical(this, "网络错误", reply->errorString());
        }

        reply->deleteLater();
        ui->btnRegister->setEnabled(true); // 恢复按钮
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
        // 把提取出来的 fromUid 转成 QString，方便去字典里查
        QString fromUidStr = obj["fromUserId"].toVariant().toString();
        QString content = obj["content"].toString();

        // 🌟 K导师的偷梁换柱：去字典里查这个 ID 对应的名字
        QString displayName = fromUidStr; // 默认先显示 ID
        if (friendMap.contains(fromUidStr)) {
            displayName = friendMap.value(fromUidStr); // 查到了！替换成昵称！
        }

        QString time = QDateTime::currentDateTime().toString("HH:mm:ss");

        // 注意看这里，把原来的 arg(fromUid) 换成了 arg(displayName)
        ui->textLog->append(QString("[%1] %2: %3")
                                .arg(time)
                                .arg(displayName)
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
    QString displayTargetName = targetIdStr; // 默认显示目标id
    if (friendMap.contains(targetIdStr)) {
        displayTargetName = friendMap.value(targetIdStr); // 替换名称
    }
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    ui->textLog->append(QString("[%1] 我 -> %2: %3")
                            .arg(time)
                            .arg(displayTargetName)
                            .arg(msgContent));
}

// 拉取好友列表
void MainWindow::fetchFriendList()
{
    // 1. 组装 URL（走网关）
    QUrl url("http://localhost:9000/im-auth/friend/list");
    QNetworkRequest request(url);

    // 2. 挂载 Token（K导师核心划重点：Bearer 后面有空格！）
    QString authHeader = "Bearer " + myToken;
    request.setRawHeader("Authorization", authHeader.toUtf8());

    // 3. 发射 GET 请求
    QNetworkReply *reply = networkManager->get(request);

    // 4. 处理响应
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject rootObj = doc.object();

            if (rootObj["code"].toInt() == 200) {
                QJsonArray dataArray = rootObj["data"].toArray();
                ui->listFriends->clear(); // 清空旧列表

                // 遍历 JSON 数组
                for (int i = 0; i < dataArray.size(); ++i) {
                    QJsonObject friendObj = dataArray[i].toObject();

                    // 提取昵称和雪花ID（注意雪花ID在JSON里由于太大，通常建议当字符串处理，Qt 用 toVariant().toString() 最稳）
                    QString nickname = friendObj["nickname"].toString();
                    QString friendId = friendObj["id"].toVariant().toString();
                    friendMap.insert(friendId, nickname); // 好友信息存入字典

                    // 创建列表项，显示昵称
                    QListWidgetItem *item = new QListWidgetItem(nickname);
                    // 把雪花 ID 作为“用户数据”悄悄绑定在这个 Item 上，前端看不见，但代码能拿到！
                    item->setData(Qt::UserRole, friendId);

                    ui->listFriends->addItem(item);
                }
            } else {
                ui->textLog->append(">> 获取好友列表失败：" + rootObj["message"].toString());
            }
        }
        reply->deleteLater();
    });
}

// 实现添加好友
void MainWindow::onBtnAddFriendClicked()
{
    QString targetUsername = ui->editSearchUser->text().trimmed();
    if (targetUsername.isEmpty()) {
        QMessageBox::warning(this, "提示", "请输入要添加的好友账号");
        return;
    }

    // 禁用按钮防止狂点
    ui->btnAddFriend->setEnabled(false);

    // GET 搜索用户
    QUrl searchUrl("http://localhost:9000/im-auth/friend/search");
    QUrlQuery query;
    query.addQueryItem("username", targetUsername);
    searchUrl.setQuery(query);

    QNetworkRequest searchReq(searchUrl);
    searchReq.setRawHeader("Authorization", ("Bearer " + myToken).toUtf8());

    QNetworkReply *searchReply = networkManager->get(searchReq);

    connect(searchReply, &QNetworkReply::finished, this, [=]() {
        if (searchReply->error() == QNetworkReply::NoError) {
            QJsonDocument doc = QJsonDocument::fromJson(searchReply->readAll());
            QJsonObject rootObj = doc.object();

            if (rootObj["code"].toInt() == 200) {
                // 搜到人了！提取他的雪花 ID
                QString targetId = rootObj["data"].toObject()["id"].toVariant().toString();

                // 第二发子弹：POST 添加好友
                QUrl addUrl("http://localhost:9000/im-auth/friend/add");
                QNetworkRequest addReq(addUrl);
                addReq.setHeader(QNetworkRequest::ContentTypeHeader, "application/x-www-form-urlencoded");
                addReq.setRawHeader("Authorization", ("Bearer " + myToken).toUtf8());

                QUrlQuery addParams;
                addParams.addQueryItem("targetUserId", targetId);
                QByteArray addData = addParams.toString(QUrl::FullyEncoded).toUtf8();

                QNetworkReply *addReply = networkManager->post(addReq, addData);

                connect(addReply, &QNetworkReply::finished, this, [=]() {
                    if (addReply->error() == QNetworkReply::NoError) {
                        QJsonDocument addDoc = QJsonDocument::fromJson(addReply->readAll());
                        QJsonObject addRoot = addDoc.object();

                        if (addRoot["code"].toInt() == 200) {
                            QMessageBox::information(this, "成功", "添加好友成功！");
                            ui->editSearchUser->clear(); // 清空输入框

                            // 🌟 核心大招：静默刷新好友列表！
                            fetchFriendList();
                        } else {
                            QMessageBox::critical(this, "添加失败", addRoot["message"].toString());
                        }
                    } else {
                        QMessageBox::critical(this, "网络错误", "添加请求发送失败");
                    }
                    addReply->deleteLater();
                    ui->btnAddFriend->setEnabled(true); // 恢复按钮
                });

            } else {
                QMessageBox::critical(this, "搜索失败", rootObj["message"].toString());
                ui->btnAddFriend->setEnabled(true);
            }
        } else {
            QMessageBox::critical(this, "网络错误", "搜索请求发送失败");
            ui->btnAddFriend->setEnabled(true);
        }
        searchReply->deleteLater();
    });
}
