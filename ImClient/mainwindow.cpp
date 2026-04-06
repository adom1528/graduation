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

    // 初始化心跳定时器
    heartbeatTimer = new QTimer(this);

    // 4. 信号槽连接
    connect(ui->btnLogin, &QPushButton::clicked, this, &MainWindow::onLoginClicked); // 绑定登录按钮
    connect(ui->btnRegister, &QPushButton::clicked, this, &MainWindow::onRegisterClicked); // 绑定注册
    connect(ui->btnExitLogin, &QPushButton::clicked, this, &MainWindow::onExitLoginClicked); //绑定退出登录
    connect(ui->btnSend, &QPushButton::clicked, this, &MainWindow::onSendClicked); // 绑定发送信息
    connect(ui->listFriends, &QListWidget::itemClicked, this, &MainWindow::onFriendItemClicked); // 绑定好友列表点击事件
    connect(ui->btnAddFriend, &QPushButton::clicked, this, &MainWindow::onBtnAddFriendClicked); // 添加好友按钮
    connect(heartbeatTimer, &QTimer::timeout, this, &MainWindow::onHeartbeatTimeout); // 心跳检测
    connect(ui->btnSendImage, &QPushButton::clicked, this, &MainWindow::onSendImageClicked); // 发送图片
    connect(ui->btnSendFile, &QPushButton::clicked, this, &MainWindow::onSendFileClicked); // 发送文件

    // WebSocket 信号
    connect(webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);

    // 允许聊天框直接调用操作系统的默认浏览器打开超链接
    ui->textLog->setOpenExternalLinks(true);
}

MainWindow::~MainWindow()
{
    delete ui;
}

// 点击好友头像（目前是名字）
void MainWindow::onFriendItemClicked(QListWidgetItem *item)
{
    // 1. 提取对方的雪花 ID
    QString friendId = item->data(Qt::UserRole).toString();

    // 2. 填入输入框，方便发消息
    ui->editTargetId->setText(friendId);

    // 3. 切换聊天对象时，先把面板清空
    ui->textLog->clear();
    ui->textLog->append(QString(">> 正在拉取与【%1】的聊天记录...").arg(item->text()));

    // 4. 呼叫刚才写好的函数，去后端搬运历史记录
    fetchChatHistory(friendId);
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

// 退出登录
void MainWindow::onExitLoginClicked() {
    // ================= 1. 物理断开 =================
    onDisconnected();

    // ================= 2. 记忆消除 =================
    // 清空身份令牌和内存里的好友字典，防止“串号”
    myToken.clear();
    friendMap.clear();

    // ================= 3. 现场清理 =================
    // 清空 UI 上的所有历史痕迹
    ui->textLog->clear();           // 清空聊天面板
    ui->listFriends->clear();       // 清空好友列表
    ui->editTargetId->clear();      // 清空当前选中的聊天对象输入框
    ui->editSearchUser->clear();    // 清空搜索框

    // 贴心小细节：清空密码框，但保留账号框，方便用户下次快速重新登录
    ui->editPassword->clear();

    // ================= 4. 场景切换 =================
    ui->boxChat->hide();
    ui->boxLogin->show();
}

// WebSocket 连接成功
void MainWindow::onConnected()
{
    ui->textLog->append(">> 服务器连接成功！");
    heartbeatTimer->start(30000); // 30秒发一次ping
}

// WebSocket 断开
void MainWindow::onDisconnected() {
    //断开心跳
    if (heartbeatTimer->isActive()) {
        heartbeatTimer->stop();
    }
    // 掐断 WebSocket 长连接
    if (webSocket != nullptr && webSocket->isValid()) {
        webSocket->close();
    }
    qDebug() << "webSocket断开";
}

// 发ping检测心跳
void MainWindow::onHeartbeatTimeout()
{
    // 确保连接活着才发心跳
    if (webSocket != nullptr && webSocket->isValid()) {
        webSocket->sendTextMessage("ping");
        qDebug() << "已发送心跳包: ping";
    }
}

// 收到消息
void MainWindow::onTextMessageReceived(QString message)
{
    if (message == "pong") {
        return; // 拦截心跳
    }

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (!doc.isNull() && doc.isObject()) {
        QJsonObject obj = doc.object();
        int type = obj["type"].toInt();

        // 🌟 1. 拦截广播信息 (type == 3)，这段逻辑独立于聊天框，保持原样
        if (type == 3) {
            QString friendId = obj["userId"].toVariant().toString();
            QString status = obj["content"].toString();
            if (friendMap.contains(friendId)) {
                QString nickname = friendMap.value(friendId);
                for (int i = 0; i < ui->listFriends->count(); ++i) {
                    QListWidgetItem *item = ui->listFriends->item(i);
                    if (item->data(Qt::UserRole).toString() == friendId) {
                        if (status == "online") {
                            item->setText(nickname + " [在线]");
                            item->setForeground(QBrush(QColor(46, 139, 87)));
                            QFont font = item->font(); font.setBold(true); item->setFont(font);
                        } else {
                            item->setText(nickname + " [离线]");
                            item->setForeground(QBrush(Qt::gray));
                            QFont font = item->font(); font.setBold(false); item->setFont(font);
                        }
                        break;
                    }
                }
            }
            return;
        }

        // 🌟 2. 处理聊天消息 (type 1, 4, 5) —— 直接送入漏斗！
        QString fromUidStr = obj["fromUserId"].toVariant().toString();
        QString content = obj["content"].toString();
        QString fileName = obj["fileName"].toString();

        // 从字典获取昵称，拿不到就显示ID
        QString nickname = friendMap.value(fromUidStr, fromUidStr);

        // 默认收到 WebSocket 的都是对方发来的（isSelf = false）
        int msgType = (type == 0) ? 1 : type; // 容错处理
        renderMessageToUI(msgType, nickname, content, fileName, false);

    } else {
        // 系统文本消息
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
    renderMessageToUI(1, "我", msgContent, "", true);
}

// 拉取好友列表
void MainWindow::fetchFriendList()
{
    // 1. 组装 URL（走网关）
    QUrl url("http://localhost:9000/im-auth/friend/list");
    QNetworkRequest request(url);

    // 2. 挂载 Token
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
                    bool isOnline = friendObj["isOnline"].toBool();

                    QString displayText = nickname;
                    if (isOnline) {
                        displayText += "[在线]";
                    } else {
                        displayText += "[离线]";
                    }

                    // 创建列表项，显示昵称
                    QListWidgetItem *item = new QListWidgetItem(displayText);
                    // 把雪花 ID 作为“用户数据”悄悄绑定在这个 Item 上，前端看不见，但代码能拿到！
                    item->setData(Qt::UserRole, friendId);

                    if (isOnline) {
                        // 在线状态：绿色
                        item->setForeground(QBrush(QColor(43, 139, 87)));
                        // 加粗字体
                        QFont font = item->font();
                        font.setBold(true);
                        item->setFont(font);
                    } else {
                        item->setForeground(QBrush(Qt::gray));
                    }

                    ui->listFriends->addItem(item);
                    friendMap.insert(friendId, nickname); // 好友信息存入字典
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

                            // 静默刷新好友列表！
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

// 点击好友后显示历史聊天记录
void MainWindow::fetchChatHistory(QString friendId)
{
    // 1. 组装请求 URL
    QUrl url("http://localhost:9000/im-server/chat/history");
    QUrlQuery query;
    query.addQueryItem("friendId", friendId);
    url.setQuery(query);

    QNetworkRequest request(url);
    request.setRawHeader("Authorization", ("Bearer " + myToken).toUtf8());

    // 2. 发射 GET 请求
    QNetworkReply *reply = networkManager->get(request);

    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject rootObj = doc.object();

            if (rootObj["code"].toInt() == 200) {
                // 拿到历史记录数组！
                QJsonArray dataArray = rootObj["data"].toArray();

                // 3. 遍历渲染上屏
                for (int i = 0; i < dataArray.size(); ++i) {
                    QJsonObject msgObj = dataArray[i].toObject();

                    int type = msgObj["type"].toInt();
                    QString content = msgObj["content"].toString();
                    QString fileName = msgObj["fileName"].toString();
                    QString fromUserId = msgObj["fromUserId"].toVariant().toString();
                    // 后端传过来的时间通常是 "2026-04-01T13:39:16"，把 T 换成空格变好看点
                    //QString createTime = msgObj["createTime"].toString().replace("T", " ");

                    // 核心判断
                    bool isSelf = !(fromUserId == friendId);
                    QString senderName = "我";
                    if (!isSelf) {
                        // 如果是对方发的，去字典里把他的名字查出来！
                        senderName = friendMap.contains(fromUserId) ? friendMap.value(fromUserId) : fromUserId;
                    }

                    // 一键渲染上屏！
                    renderMessageToUI(type, senderName, content, fileName, isSelf);
                }
                //ui->textLog->append("---------------- 历史消息分割线 ----------------");
            } else {
                ui->textLog->append(">> 拉取历史记录失败：" + rootObj["message"].toString());
            }
        } else {
            ui->textLog->append(">> 网络错误，无法拉取历史记录");
        }
        reply->deleteLater();
    });
}

// 发送图片
void MainWindow::onSendImageClicked()
{
    // 先检查有没有选中聊天对象
    QString targetId = ui->editTargetId->text();
    if (targetId.isEmpty()) {
        QMessageBox::warning(this, "操作提示", "请先在右侧好友列表中选择一个聊天对象！");
        return; // 拒绝执行后续的所有操作！
    }
    // 1. 唤起系统文件选择框，只允许选图片
    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的图片", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    if (filePath.isEmpty()) {
        return; // 用户取消了选择
    }

    // 2. 准备物理文件
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << filePath;
        delete file;
        return;
    }

    // 3. 组装 multipart/form-data 报文 (就跟刚才 Postman 做的动作一模一样)
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart imagePart;
    // 设置请求头：指明这是一个叫 "file" 的表单字段，并附带原文件名
    imagePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                        QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(QFileInfo(filePath).fileName())));
    imagePart.setBodyDevice(file);
    file->setParent(multiPart); // 让 file 的生命周期跟着 multiPart 走，防止内存泄漏
    multiPart->append(imagePart);

    // 4. 瞄准后端的上传接口
    QNetworkRequest request(QUrl("http://localhost:9002/file/upload"));
    request.setRawHeader("Authorization", "Bearer " + myToken.toUtf8());

    // 5. 扣动扳机，异步发射！
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->post(request, multiPart);
    multiPart->setParent(reply); // 自动释放内存

    // 6. 蹲守上传结果
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            // 读取后端返回的 JSON (里面藏着 MinIO 的 URL)
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject rootObj = doc.object();

            if (rootObj["code"].toInt() == 200) {
                QString minioUrl = rootObj["data"].toString();
                qDebug() << "文件上传成功，拿到 URL:" << minioUrl;

                // 上传成功后，把 URL 打包成 type=4 的 WebSocket 消息发给对方！
                if (!targetId.isEmpty() && webSocket != nullptr && webSocket->isValid()) {
                    QJsonObject msgObj;
                    msgObj["type"] = 4; // 协议：4代表图片/文件
                    msgObj["toUserId"] = targetId.toLongLong();
                    msgObj["content"] = minioUrl;

                    QJsonDocument sendDoc(msgObj);
                    webSocket->sendTextMessage(sendDoc.toJson(QJsonDocument::Compact));

                    QString imgHtml = QString("<br><span style='color:green;'>我 (发了一张图片):</span><br><img src='file:///%1' width='150'>").arg(filePath);
                    ui->textLog->append(imgHtml); // 直接追加带格式的 HTML 代码;
                    //renderMessageToUI(4, "我", minioUrl, "", true);
                }
            } else {
                qDebug() << "上传失败，服务器返回:" << rootObj["msg"].toString();
            }
        } else {
            qDebug() << "网络请求报错:" << reply->errorString();
        }

        reply->deleteLater();
        manager->deleteLater();
    });
}

// 发送文件
void MainWindow::onSendFileClicked()
{
    // 1.
    QString targetId = ui->editTargetId->text();
    if (targetId.isEmpty()) {
        QMessageBox::warning(this, "操作提示", "请先在左侧好友列表中选择一个聊天对象！");
        return;
    }

    // 2. 唤起文件选择框，允许选择所有类型的文件 (*.*)
    QString filePath = QFileDialog::getOpenFileName(this, "选择要发送的文件", "", "All Files (*.*)");
    if (filePath.isEmpty()) {
        return; // 用户取消了选择
    }

    //  提取真实文件名 (比如 "需求文档.docx")
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    // 3. 准备物理文件
    QFile *file = new QFile(filePath);
    if (!file->open(QIODevice::ReadOnly)) {
        qDebug() << "无法打开文件:" << filePath;
        delete file;
        return;
    }

    // 4. 组装 multipart/form-data 报文 (和图片一模一样)
    QHttpMultiPart *multiPart = new QHttpMultiPart(QHttpMultiPart::FormDataType);
    QHttpPart filePart;
    filePart.setHeader(QNetworkRequest::ContentDispositionHeader,
                       QVariant(QString("form-data; name=\"file\"; filename=\"%1\"").arg(fileName)));
    filePart.setBodyDevice(file);
    file->setParent(multiPart);
    multiPart->append(filePart);

    // 5. 瞄准后端上传接口
    QNetworkRequest request(QUrl("http://localhost:9002/file/upload"));
    request.setRawHeader("Authorization", "Bearer " + myToken.toUtf8());

    // 6. 异步发射
    QNetworkAccessManager *manager = new QNetworkAccessManager(this);
    QNetworkReply *reply = manager->post(request, multiPart);
    multiPart->setParent(reply);

    // 7. 蹲守结果
    connect(reply, &QNetworkReply::finished, this, [=]() {
        if (reply->error() == QNetworkReply::NoError) {
            QByteArray responseData = reply->readAll();
            QJsonDocument doc = QJsonDocument::fromJson(responseData);
            QJsonObject rootObj = doc.object();

            if (rootObj["code"].toInt() == 200) {
                QString minioUrl = rootObj["data"].toString();
                qDebug() << "文件上传成功，拿到 URL:" << minioUrl;

                // 组装 type=5 的 WebSocket 消息
                if (webSocket != nullptr && webSocket->isValid()) {
                    QJsonObject msgObj;
                    msgObj["type"] = 5;
                    msgObj["toUserId"] = targetId.toLongLong();
                    msgObj["content"] = minioUrl;
                    msgObj["fileName"] = fileName;

                    QJsonDocument sendDoc(msgObj);
                    webSocket->sendTextMessage(sendDoc.toJson(QJsonDocument::Compact));

                    // 发送方本地渲染：直接用 <a> 标签做个超链接
                    QString fileHtml = QString("<br><span style='color:green;'>我 (发送了文件): </span><a href='%1'>%2</a>")
                                           .arg(minioUrl).arg(fileName);
                    renderMessageToUI(5,"我", minioUrl, fileName, true);
            } else {
                qDebug() << "上传失败:" << rootObj["msg"].toString();
            }
        } else {
            qDebug() << "网络请求报错:" << reply->errorString();
        }

        reply->deleteLater();
        manager->deleteLater();
    }
    });
}

// 富文本编辑
void MainWindow::renderMessageToUI(int type, const QString &senderName, const QString &content, const QString &fileName, bool isSelf)
{
    // 1. 判断颜色、称呼和当前时间
    QString color = isSelf ? "green" : "blue";
    QString prefix = isSelf ? "我" : senderName;
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");

    // 2. 统一分发渲染逻辑
    if (type == 1 || type == 0) { // 文本消息 (有些旧系统可能没传 type，默认当文本处理)
        QString html = QString("<br><span style='color:%1;'>[%2] %3: </span>%4")
                           .arg(color, time, prefix, content);
        ui->textLog->append(html);
    }
    else if (type == 4) { // 图片消息
        QNetworkRequest request((QUrl(content)));
        QNetworkAccessManager *manager = new QNetworkAccessManager(this);
        QNetworkReply *reply = manager->get(request);

        connect(reply, &QNetworkReply::finished, this, [=]() {
            if (reply->error() == QNetworkReply::NoError) {
                QByteArray imgData = reply->readAll();
                QString base64Img = QString::fromLatin1(imgData.toBase64());
                QString html = QString("<br><span style='color:%1;'>[%2] %3 (图片):</span><br><img src='data:image/png;base64,%4' width='150'>")
                                   .arg(color, time, prefix, base64Img);
                ui->textLog->append(html);
            } else {
                ui->textLog->append(QString("<br><span style='color:gray;'>[%1 的图片加载失败]</span>").arg(prefix));
            }
            reply->deleteLater();
            manager->deleteLater();
        });
    }
    else if (type == 5) { // 文件消息
        QString fileHtml = QString("<br><span style='color:%1;'>[%2] %3 发送了文件: </span><br><a href='%4'>%5</a>")
                               .arg(color, time, prefix, content, fileName);
        ui->textLog->append(fileHtml);
    }
}

