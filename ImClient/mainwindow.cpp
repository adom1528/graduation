#include "mainwindow.h"
#include "httpmanager.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLineEdit>
#include <QUrl>

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

    //启动WebSocket长连接
    m_webSocket = new QWebSocket();
    connect(m_webSocket, &QWebSocket::connected, this, &MainWindow::onConnected);

}

MainWindow::~MainWindow()
{
}

void MainWindow::onConnected()
{
    qDebug() << "建立websocket连接";
    QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
    m_chatWidget->appendMessage(0, "服务器", "服务器连接成功", time, "", false);
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

                // 创建列表项并存储 ID
                QListWidgetItem* listItem = new QListWidgetItem(nickname, m_friendList);
                listItem->setData(Qt::UserRole, friendId);
                m_friendList->addItem(listItem);

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
                qDebug() << "发送消息用户:"<<fromUserId;
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
    if (!m_webSocket || !m_webSocket->isValid()) {
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

void MainWindow::onTextMessageReceived(QString message)
{
    if (message == "pong") return; // 心跳拦截

    QJsonDocument doc = QJsonDocument::fromJson(message.toUtf8());
    if (doc.isNull() || !doc.isObject()) return;

    QJsonObject obj = doc.object();
    int type = obj["type"].toInt();

    // 路由分发逻辑
    if (type == 1) { // 处理单聊消息
        QString fromUserId = obj["fromUserId"].toVariant().toString();
        QString content = obj["content"].toString();
        QString createTime = obj["createTime"].toString();
        // 从缓存中获取发送者昵称
        QString senderName = m_friendMap.value(fromUserId, "未知用户");

        // 将消息注入 ChatWidget 进行渲染
        m_chatWidget->appendMessage(1, senderName, content, createTime, "", false);
    }
    // ... 其他类型 (图片/文件) 的分发逻辑
}

void MainWindow::onFriendItemClicked(QListWidgetItem *item)
{
    QString friendId = item->data(Qt::UserRole).toString();
    QString nickname = item->text().split("[").first().trimmed(); // 去除 [在线] 标识

    // 1. 切换堆栈至聊天面板
    m_rightStack->setCurrentWidget(m_chatWidget);

    // 2. 设置聊天组件的上下文状态
    m_chatWidget->setCurrentChat(friendId, nickname);

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
    m_btnAvatar = new QPushButton("头像", m_leftNavbar);
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
            color: #000000;
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
