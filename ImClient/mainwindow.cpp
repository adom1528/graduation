#include "mainwindow.h"
#include "httpmanager.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLineEdit>
#include <QUrl>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // 初始化窗口基础属性
    this->setMinimumSize(1000, 700);
    this->setWindowTitle("IM 跨平台客户端");

    // 搭建布局和皮肤
    initGlobalLayout();
    initStyleSheet();

    //拉取好友列表，填充中间侧边栏
    fetchFriendList();

    // 初始化心跳计时器
    m_heartbeatTimer = new QTimer(this);
    // 绑定心跳超时槽函数
    connect(m_heartbeatTimer, &QTimer::timeout, this, &MainWindow::onHeartbeatTimeout);

    //启动WebSocket长连接
    m_webSocket = new QWebSocket();

    connect(m_webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);
    connect(m_webSocket, &QWebSocket::textMessageReceived, this, &MainWindow::onTextMessageReceived);
    // 监听断开事件
    connect(m_webSocket, &QWebSocket::disconnected, this, [=]() {
        qDebug() << "⚠️ WebSocket 连接已断开！错误信息：" << m_webSocket->errorString();
    });
    // 监听底层状态机变化
    connect(m_webSocket, &QWebSocket::stateChanged, this, [=](QAbstractSocket::SocketState state) {
        qDebug() << "🔍 WebSocket 状态切换为：" << state;
    });

    QString token = HttpManager::instance()->getToken();
    QString wsUrl = QString("ws://localhost:9003/im?token=%1").arg(token);
    m_webSocket->open(QUrl(wsUrl));

}

MainWindow::~MainWindow()
{
}

void MainWindow::onConnected()
{
    qDebug() << "建立websocket连接";
    m_heartbeatTimer->start(30000); // 30秒发一次ping
}

void MainWindow::onDisconnected()
{
    // 断开心跳
    if (m_heartbeatTimer->isActive()) {
        m_heartbeatTimer->stop();
    }

    // 断开WebSocket长连接
    if (m_webSocket != nullptr && m_webSocket->isValid()) {
        m_webSocket->close();
    }

    //qDebug() << "断开WebSocket";
}

// 发ping检测心跳
void MainWindow::onHeartbeatTimeout()
{
    // 确保连接活着才发心跳
    if (m_webSocket != nullptr && m_webSocket->isValid()) {
        m_webSocket->sendTextMessage("ping");
        qDebug() << "已发送心跳包: ping";
    }
}

void MainWindow::fetchFriendList() {
    // 确保路径走 9000 网关
    QString url = "http://localhost:9000/im-server/friend/list";

    HttpManager::instance()->get(url, [=](QJsonObject res) {
        int code = res["code"].toInt();
        if (code == 200) {
            QJsonArray data = res["data"].toArray();
            m_friendList->clear(); // 清空旧列表项

            for (int i = 0; i < data.size(); ++i) {
                QJsonObject item = data[i].toObject();
                QString nickname = item["nickname"].toString();
                QString friendId = item["id"].toVariant().toString();

                qDebug() << "🔍 解析好友数据:" << item;

                // 不管后端传的是 true/false 还是 1/0，甚至字符串 "true"，都能正确转换！
                bool isOnline = item["isOnline"].toVariant().toBool();

                QString displayText = nickname;
                if (isOnline) {
                    displayText += " [在线]";
                } else {
                    displayText += " [离线]";
                }

                QListWidgetItem* listItem = new QListWidgetItem(displayText, m_friendList);
                listItem->setData(Qt::UserRole, friendId);
                m_friendList->addItem(listItem);

                // 初始颜色渲染
                if (isOnline) {
                    listItem->setForeground(QBrush(QColor(46, 139, 87))); // 森林绿
                    QFont font = listItem->font();
                    font.setBold(true);
                    listItem->setFont(font);
                } else {
                    listItem->setForeground(QBrush(Qt::gray)); // 离线灰
                }

                // 更新全局好友映射表
                m_friendMap.insert(friendId, nickname);
            }
        }
    }, [=](QString err) {
                                     qDebug() << "获取好友列表失败: " << err;
                                 });
}

void MainWindow::fetchChatHistory(QString friendId)
{
    QString url = "http://localhost:9000/im-server/chat/history";
    QVariantMap params;
    params["friendId"] = friendId;

    HttpManager::instance()->get(url, params, [=](QJsonObject res) {
        int code = res["code"].toInt();
        if (code == 200) {
            QJsonArray responseDate = res["data"].toArray();

            for (int i = 0; i < responseDate.size(); ++i) {
                QJsonObject msgObj = responseDate[i].toObject();
                //qDebug() << msgObj;
                // 拆分信息
                int type = msgObj["type"].toInt();
                QString content = msgObj["content"].toString();
                QString fileName = msgObj["fileName"].toString();
                QString fromUserId = msgObj["fromUserId"].toVariant().toString();
                QString createTime = msgObj["createTime"].toString().replace("T", " ");

                // 判断是谁发的消息
                bool CurrentUserSelf = !(friendId == fromUserId);
                QString senderName = "我";

                if (!CurrentUserSelf) {
                    senderName = m_friendMap.contains(fromUserId) ? m_friendMap.value(fromUserId) : fromUserId;
                }
                m_chatWidget->appendMessage(type, senderName, content, createTime, fileName, CurrentUserSelf);
            }
        }

    }, [=](QString err) {
                                     qDebug() << "获取聊天记录失败：" << err;
                                 });
}

void MainWindow::handleSendMessageRequest(const QString& targetId, const QString& content)
{
    qDebug() << targetId << ":" << content;
    if (!m_webSocket || !m_webSocket->isValid()) {
        qDebug() << "⚠️ 严重警告：WebSocket 未连接或已断开，无法发送数据！";
        QMessageBox::warning(this, "网络错误", "与服务器的实时连接已断开，请重新登录或稍后再试。");
        return;
    }

    // 构造标准的协议 JSON
    QJsonObject json;
    json["type"] = 1; // 单聊类型
    json["toUserId"] = targetId.toLongLong();
    json["content"] = content;

    QJsonDocument doc(json);
    m_webSocket->sendTextMessage(doc.toJson(QJsonDocument::Compact));
}

void MainWindow::handleSendImageRequest(const QString& targetId) {
    if (!m_webSocket || !m_webSocket->isValid()) {
        qDebug() << "⚠️ 严重警告：WebSocket 未连接或已断开，无法发送数据！";
        QMessageBox::warning(this, "网络错误", "与服务器的实时连接已断开，请重新登录或稍后再试。");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "选择想要发送的图片", "", "Images (*.png *.jpg *.jpeg *.bmp *.gif)");
    // 用户取消选择
    if (filePath.isEmpty()) {
        return;
    }

    QString uploadUrl = "http://localhost:9000/im-server/file/upload";

    // 优雅调用 HttpManager
    HttpManager::instance()->uploadFile(uploadUrl, filePath, [=](QJsonObject rootObj) {
        // 【成功回调】
        if (rootObj["code"].toInt() == 200) {
            QString minioUrl = rootObj["data"].toString();
            qDebug() << "文件上传成功，拿到 URL:" << minioUrl;

            // 发送 WebSocket 消息
            if (!targetId.isEmpty() && m_webSocket != nullptr && m_webSocket->isValid()) {
                QJsonObject msgObj;
                msgObj["type"] = 4; // 协议：4代表图片
                msgObj["toUserId"] = targetId.toLongLong();
                msgObj["content"] = minioUrl;
                QString createTime = QDateTime::currentDateTime().toString("HH:mm:ss");
                msgObj["createTime"] = createTime;

                QJsonDocument sendDoc(msgObj);
                m_webSocket->sendTextMessage(sendDoc.toJson(QJsonDocument::Compact));

                // 更新 UI
                m_chatWidget->appendMessage(4, "我",minioUrl, createTime, "", true);
            }
        } else {
            qDebug() << "后端拒绝了上传:" << rootObj["msg"].toString();
        }
    }, [=](QString errorMsg) {
                                            // 【失败回调】处理网络报错
                                            qDebug() << "文件上传网络报错:" << errorMsg;
                                        });


}

void MainWindow::handleSendFileRequest(const QString& targetId) {
    if (!m_webSocket || !m_webSocket->isValid()) {
        qDebug() << "⚠️ 严重警告：WebSocket 未连接或已断开，无法发送数据！";
        QMessageBox::warning(this, "网络错误", "与服务器的实时连接已断开，请重新登录或稍后再试。");
        return;
    }

    QString filePath = QFileDialog::getOpenFileName(this, "选择想要发送的文件", "", "All Files (*.*)");
    // 用户取消选择
    if (filePath.isEmpty()) {
        return;
    }

    // 提取真实文件名
    QFileInfo fileInfo(filePath);
    QString fileName = fileInfo.fileName();

    QString uploadUrl = "http://localhost:9000/im-server/file/upload";

    // 优雅调用 HttpManager
    HttpManager::instance()->uploadFile(uploadUrl, filePath, [=](QJsonObject rootObj) {
        // 【成功回调】
        if (rootObj["code"].toInt() == 200) {
            QString minioUrl = rootObj["data"].toString();
            qDebug() << "文件上传成功，拿到 URL:" << minioUrl;

            // 发送 WebSocket 消息
            if (!targetId.isEmpty() && m_webSocket != nullptr && m_webSocket->isValid()) {
                QJsonObject msgObj;
                msgObj["type"] = 5; // 协议：5代表文件
                msgObj["toUserId"] = targetId.toLongLong();
                msgObj["content"] = minioUrl;
                QString createTime = QDateTime::currentDateTime().toString("HH:mm:ss");
                msgObj["createTime"] = createTime;
                msgObj["fileName"] = fileName;

                QJsonDocument sendDoc(msgObj);
                m_webSocket->sendTextMessage(sendDoc.toJson(QJsonDocument::Compact));

                // 更新 UI
                m_chatWidget->appendMessage(5, "我",minioUrl, createTime, fileName, true);
            }
        } else {
            qDebug() << "后端拒绝了上传:" << rootObj["msg"].toString();
        }
    }, [=](QString errorMsg) {
                                            // 【失败回调】处理网络报错
                                            qDebug() << "文件上传网络报错:" << errorMsg;
                                        });

}

void MainWindow::onTextMessageReceived(QString message)
{
    // 1. 心跳检测拦截
    if (message == "pong") return;

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject obj = doc.object();
    int type = obj["type"].toInt();

    // ============================================================
    // 业务逻辑 A：好友在线状态广播 (Type 3)
    // ============================================================
    if (type == 3) {
        QString friendId = obj["userId"].toVariant().toString();
        QString status = obj["content"].toString(); // "online" 或 "offline"

        if (m_friendMap.contains(friendId)) {
            QString nickname = m_friendMap.value(friendId);

            // 遍历中间侧边栏列表，找到对应的项并更新视觉状态
            for (int i = 0; i < m_friendList->count(); ++i) {
                QListWidgetItem *item = m_friendList->item(i);
                if (item->data(Qt::UserRole).toString() == friendId) {
                    if (status == "online") {
                        item->setText(nickname + " [在线]");
                        item->setForeground(QBrush(QColor(46, 139, 87))); // 森林绿
                    } else {
                        item->setText(nickname + " [离线]");
                        item->setForeground(QBrush(Qt::gray));
                    }
                    break;
                }
            }
        }
        return;
    }

    // ============================================================
    // 业务逻辑 B：聊天消息分发 (Type 1, 4, 5)
    // ============================================================
    // 提取消息通用元数据
    QString fromUserId = obj["fromUserId"].toVariant().toString();
    QString content = obj["content"].toString();
    QString createTime = obj["createTime"].toString();
    QString fileName = obj["fileName"].toString();

    // 容错处理：如果后端没传时间，前端补一个当前时间
    if (createTime.isEmpty()) {
        createTime = QDateTime::currentDateTime().toString("HH:mm:ss");
    }

    // 从全局字典获取发送者昵称
    QString senderName = m_friendMap.value(fromUserId, "未知用户");

    // 核心分发：将数据灌入右侧的 ChatWidget 组件
    // 注意：收到的消息 isSelf 永远为 false
    if (type == 1 || type == 4 || type == 5) {
        m_chatWidget->appendMessage(type, senderName, content, createTime, fileName, false);
    }
}

void MainWindow::onFriendItemClicked(QListWidgetItem *item)
{
    QString friendId = item->data(Qt::UserRole).toString();
    //QString nickname = item->text().split("[").first().trimmed(); // 去除 [在线] 标识

    // 1. 切换堆栈至聊天面板
    m_rightStack->setCurrentWidget(m_chatWidget);

    // 2. 设置聊天组件的上下文状态
    m_chatWidget->setCurrentChat(friendId, item->text().trimmed());

    // 3. 此处可并行调用 HttpManager 拉取历史记录并循环调用 m_chatWidget->appendMessage
    fetchChatHistory(friendId);
}



//************************************** UI初始化 ************************************
void MainWindow::initGlobalLayout()
{
    m_centralWidget = new QWidget(this);
    m_mainLayout = new QHBoxLayout(m_centralWidget);

    // 设置主布局间距与边距为 0，确保三段式无缝衔接
    m_mainLayout->setContentsMargins(0, 0, 0, 0);
    m_mainLayout->setSpacing(0);

    initLeftNavbar();
    initMiddleSidebar();
    initRightContainer();

    setCentralWidget(m_centralWidget);
}

void MainWindow::initLeftNavbar()
{
    m_leftNavbar = new QFrame(this);
    m_leftNavbar->setFixedWidth(60);
    m_leftNavbar->setObjectName("leftNavbar"); // 用于 QSS 样式表定位

    m_navLayout = new QVBoxLayout(m_leftNavbar);
    m_navLayout->setContentsMargins(5, 20, 5, 20);
    m_navLayout->setSpacing(20);

    // 导航栏初始化
    m_btnAvatar = new QPushButton("我", m_leftNavbar);
    m_btnAvatar->setFixedSize(40, 40);
    m_btnAvatar->setObjectName("btnAvatar");

    m_btnChat = new QPushButton("消息", m_leftNavbar);
    m_btnChat->setFixedSize(40, 40);

    m_btnContact = new QPushButton("联系人", m_leftNavbar);
    m_btnContact->setFixedSize(40, 40);

    m_navLayout->addWidget(m_btnAvatar);
    m_navLayout->addWidget(m_btnChat);
    m_navLayout->addWidget(m_btnContact);
    m_navLayout->addStretch(); // 底部弹簧置顶按钮

    m_mainLayout->addWidget(m_leftNavbar);
}

void MainWindow::initMiddleSidebar()
{
    m_middleSidebar = new QFrame(this);
    m_middleSidebar->setFixedWidth(250);
    m_middleSidebar->setObjectName("middleSidebar");

    m_sidebarLayout = new QVBoxLayout(m_middleSidebar);
    m_sidebarLayout->setContentsMargins(0, 0, 0, 0);
    m_sidebarLayout->setSpacing(0);

    // 搜索与添加功能区
    m_searchHeader = new QWidget(m_middleSidebar);
    m_searchHeader->setFixedHeight(60);
    m_searchHeader->setObjectName("searchHeader");

    QHBoxLayout* searchLayout = new QHBoxLayout(m_searchHeader);
    searchLayout->setContentsMargins(10, 15, 10, 15);
    searchLayout->setSpacing(10);

    QLineEdit* searchEdit = new QLineEdit(m_searchHeader);
    searchEdit->setPlaceholderText("搜索");
    searchEdit->setFixedHeight(30);
    searchEdit->setObjectName("searchEdit");

    QPushButton* btnAddFriend = new QPushButton("+", m_searchHeader);
    btnAddFriend->setFixedSize(30, 30);
    btnAddFriend->setObjectName("btnAddFriendTop"); // TODO后续会绑定这个按钮弹出添加好友窗口

    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(btnAddFriend);

    m_friendList = new QListWidget(m_middleSidebar);
    m_friendList->setFrameShape(QFrame::NoFrame);

    m_sidebarLayout->addWidget(m_searchHeader);
    m_sidebarLayout->addWidget(m_friendList);

    m_mainLayout->addWidget(m_middleSidebar);

    // 在 initMiddleSidebar 中添加
    connect(m_friendList, &QListWidget::itemClicked, this, &MainWindow::onFriendItemClicked);
}

void MainWindow::initRightContainer()
{
    m_rightStack = new QStackedWidget(this);
    m_rightStack->setObjectName("rightStack");

    // 1. 初始化聊天组件并嵌入堆栈
    m_chatWidget = new ChatWidget(this);
    m_rightStack->addWidget(m_chatWidget);

    // 2. 默认显示空白页（可保持为 m_emptyPage，或直接默认显示聊天框但内容为空）
    m_emptyPage = new QWidget();
    m_rightStack->addWidget(m_emptyPage);
    m_rightStack->setCurrentWidget(m_emptyPage);

    // 3. 建立信号连接：当 ChatWidget 请求发送消息时，由 MainWindow 代理发送
    connect(m_chatWidget, &ChatWidget::textMessageSendRequested,
            this, &MainWindow::handleSendMessageRequest);

    connect(m_chatWidget, &ChatWidget::imageSendRequested,
            this, &MainWindow::handleSendImageRequest);

    connect(m_chatWidget, &ChatWidget::fileSendRequested,
            this, &MainWindow::handleSendFileRequest);

    m_mainLayout->addWidget(m_rightStack, 1);
}

// QSS 规则集
void MainWindow::initStyleSheet()
{
    /* * 采用 C++11 Raw String (R"(...)") 语法，无需转义换行符。
     * 色彩规范 (Palette):
     * - Left Navbar: 深邃黑灰 (#2E2E2E)
     * - Middle Sidebar: 柔和浅灰 (#F0F0F0)
     * - Right Workspace: 纯白 (#FFFFFF)
     * - 主题高亮色 (Active): 微信绿 (#07C160)
     */
    QString qss = R"(
        /* =========================================
           1. 全局基础重置 (Global Reset)
           ========================================= */
        QWidget {
            font-family: "Microsoft YaHei", "Segoe UI", sans-serif;
            font-size: 14px;
        }

        /* 去除所有 QFrame 自带的边框，实现无缝拼接 */
        QFrame {
            border: none;
        }

        /* =========================================
           2. 左侧导航栏 (Left Navbar)
           ========================================= */
        #leftNavbar {
            background-color: #2E2E2E;
        }

        /* 导航栏按钮的基础样式 */
        #leftNavbar QPushButton {
            background-color: transparent;
            color: #888888;
            border: none;
            border-radius: 4px; /* 轻微圆角 */
        }

        /* 导航栏按钮的悬停交互 (Hover) */
        #leftNavbar QPushButton:hover {
            background-color: #3D3D3D;
        }

        /* 头像占位符美化 */
        #leftNavbar #btnAvatar {
            background-color: #07C160; /* 微信绿 */
            color: #FFFFFF;
            border-radius: 20px; /* 变成正圆形 */
            font-weight: bold;
        }

        /* =========================================
           3. 中间侧边栏 (Middle Sidebar)
           ========================================= */
        #middleSidebar {
            background-color: #F0F0F0;
            /* 右侧添加一条极细的分割线，增强视觉层次 */
            border-right: 1px solid #E0E0E0;
        }

        /* 好友列表控件样式 */
        QListWidget {
            background-color: transparent;
            outline: none; /* 去除点击时产生的虚线框 */
        }

        /* 列表项基础样式 */
        QListWidget::item {
            height: 64px; /* 统一行高 */
            padding-left: 10px;
            /* 🌟 删除了 color: #000000; 把颜色控制权还给 C++ */
        }

        /* 列表项悬停交互 */
        QListWidget::item:hover {
            background-color: #DEDEDE;
        }

        /* 列表项选中状态 (使用微信级高亮灰) */
        QListWidget::item:selected {
            background-color: #C6C6C6;
            color: #000000;
        }

        /* =========================================
           4. 右侧工作区 (Right Workspace)
           ========================================= */
        #rightStack {
            background-color: #FFFFFF;
        }

        /* 搜索区域美化 */
        #searchHeader {
            border-bottom: 1px solid #E0E0E0; /* 增加底部阴影分割线 */
        }
        #searchEdit {
            background-color: #E2E2E2;
            border: none;
            border-radius: 4px;
            padding-left: 10px;
            color: #333333
        }
        #btnAddFriendTop {
            background-color: #E2E2E2;
            border: none;
            border-radius: 4px;
            font-size: 18px;
            color: #333333;
        }
        #btnAddFriendTop:hover {
            background-color: #D2D2D2;
        }

        /* 右侧聊天组件强化 */
        #chatHistory {
            background-color: #F5F5F5; /* 聊天背景设为浅灰，区分于纯白 */
            border: none;
            color: #000000;
            border-bottom: 1px solid #E0E0E0; /* 强化聊天记录和工具栏的分割线 */
        }
        #chatInput {
            color: #000000;             /* 文字颜色为纯黑 */
            background-color: #FFFFFF;  /* 背景颜色为纯白 */
            font-size: 14px;            /* 字体大小 */
            font-family: "Microsoft YaHei"; /* 字体 */
            border: 1px solid #CCCCCC;  /* 边框颜色 */
            border-radius: 4px;         /* 边框圆角，看起来更现代 */
            padding: 5px;
        }

        /* 强化发送按钮 */
        #btnSendMsg {
            background-color: #E9E9E9;
            color: #07C160;
            border: 1px solid #E0E0E0;
            border-radius: 4px;
            font-size: 14px;
        }
        #btnSendMsg:hover {
            background-color: #1AAD19;
            color: #FFFFFF;
        }

        #btnSendImage {
            background-color: #E9E9E9;
            color: #07C160;
            border: 1px solid #E0E0E0;
            border-radius: 4px;
            font-size: 14px;
        }
        #btnSendImage:hover {
            background-color: #1AAD19;
            color: #FFFFFF;
        }

        #btnSendFile {
            background-color: #E9E9E9;
            color: #07C160;
            border: 1px solid #E0E0E0;
            border-radius: 4px;
            font-size: 14px;
        }
        #btnSendFile:hover {
            background-color: #1AAD19;
            color: #FFFFFF;
        }
    )";

    // 将组装好的样式表应用到当前主窗口
    this->setStyleSheet(qss);
}
