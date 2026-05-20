#include "mainwindow.h"
#include "httpmanager.h"
#include "addfrienddialog.h"
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonArray>
#include <QLineEdit>
#include <QUrl>
#include <QFileDialog>
#include <QFile>
#include <QMessageBox>
#include <QPainter>
#include <QPainterPath>

// QT绘图
static QPixmap makeCircularAvatar(const QPixmap &src, int size)
{
    if (src.isNull()) {
        return QPixmap();
    }

    // 1. 将原图按比例缩放并裁剪，确保图片铺满我们指定的 size，不留黑边
    QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);

    // 2. 创建一张完全透明的画布
    QPixmap result(size, size);
    result.fill(Qt::transparent);

    // 3. 请出 QPainter 在透明画布上作画
    QPainter painter(&result);
    // 极其关键：开启抗锯齿，否则圆的边缘全是马赛克一样的狗牙
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);

    // 4. 勾勒一个完美的圆形路径
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path); // 所有的绘制只在这个圆形范围内生效

    // 5. 决定性的一笔：把刚才缩放好的图，“贴”到这个圆形的裁剪框里
    painter.drawPixmap(0, 0, size, size, scaled);

    return result;
}

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

            m_friendList->clear();
            // 🌟 核心修复 1：必须清空旧的字典缓存！防止内存泄漏和幽灵数据
            m_friendMap.clear();

            // 暴力置顶 “新朋友” 项
            QListWidgetItem* newFriendItem = new QListWidgetItem("⭐ 新朋友", m_friendList);
            newFriendItem->setData(Qt::UserRole, "SYSTEM_NEW_FRIEND");
            newFriendItem->setForeground(QBrush(QColor(255, 140, 0)));
            QFont f = newFriendItem->font();
            f.setBold(true);
            newFriendItem->setFont(f);
            m_friendList->addItem(newFriendItem);

            for (int i = 0; i < data.size(); ++i) {
                QJsonObject item = data[i].toObject();
                QString nickname = item["nickname"].toString();
                QString friendId = item["id"].toVariant().toString();

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
                    listItem->setForeground(QBrush(QColor(46, 139, 87)));
                    QFont font = listItem->font();
                    font.setBold(true);
                    listItem->setFont(font);
                } else {
                    listItem->setForeground(QBrush(Qt::gray));
                }

                // 重新构建健康的全局好友映射表
                m_friendMap.insert(friendId, nickname);
            }

            // ==========================================================
            // 🌟 核心修复 2：上帝视角的防呆校验 (绝杀)
            // 如果我们当前正在聊天，就去检查这个人还是不是我们的好友
            // ==========================================================
            if (!m_currentChatFriendId.isEmpty() && m_currentChatFriendId != "SYSTEM_NEW_FRIEND") {
                // 如果最新的好友列表里没有这个人的 ID 了
                if (!m_friendMap.contains(m_currentChatFriendId)) {
                    qDebug() << "⚠️ 警告：当前聊天对象已非好友，强制清空右侧聊天视图！";
                    // 🌟 核心修复 3：强制切换到空白页，并清空当前聊天 ID
                    m_rightStack->setCurrentWidget(m_emptyPage);
                    m_currentChatFriendId = "";
                }
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

    // ============================================================
    // 🌟 新增业务逻辑 ：系统强制刷新指令 (Type 6)
    // ============================================================
    if (type == 6) {
        qDebug() << "🎯 收到系统通知：强制刷新好友列表！";
        fetchFriendList(); // 重新向服务器拉取最新的通讯录
        return;
    }
}

void MainWindow::onFriendItemClicked(QListWidgetItem *item)
{
    QString friendId = item->data(Qt::UserRole).toString();

    // ==========================================================
    // 🌟 核心遗漏点修复：必须在这里把点中的好友 ID 记录到系统状态里！
    // 这样删除逻辑才知道你现在到底在看着谁的聊天框！
    // ==========================================================
    m_currentChatFriendId = friendId;

    // 如果是新朋友，切换到管理面版
    if (friendId == "SYSTEM_NEW_FRIEND") {
        m_rightStack->setCurrentWidget(m_newFriendWidget);
        m_newFriendWidget->loadPendingRequests(); // 触发网络请求拉取列表
        return;
    }

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
    m_leftNavbar->setFixedWidth(75);
    m_leftNavbar->setObjectName("leftNavbar");

    m_navLayout = new QVBoxLayout(m_leftNavbar);
    m_navLayout->setContentsMargins(5, 30, 5, 20);
    m_navLayout->setSpacing(15);

    // 头像
    m_btnAvatar = new QPushButton(this);
    m_btnAvatar->setFixedSize(45, 45);
    m_btnAvatar->setObjectName("btnAvatar");

    // 昵称标签
    m_lblNickname = new QLabel(HttpManager::instance()->getMyNickname(), this);
    m_lblNickname->setAlignment(Qt::AlignCenter);
    m_lblNickname->setObjectName("lblMyNickname");

    // 导航按钮
    m_btnChat = new QPushButton("消息", m_leftNavbar);
    m_btnChat->setFixedSize(55, 35);
    m_btnChat->setCheckable(true); // 开启锁定状态支持

    m_btnContact = new QPushButton("联系人", m_leftNavbar);
    m_btnContact->setFixedSize(55, 35);
    m_btnContact->setCheckable(true); // 开启锁定状态支持

    // 按钮互斥组：保证一次只能锁定一个按钮
    m_navButtonGroup = new QButtonGroup(this);
    m_navButtonGroup->addButton(m_btnChat, 0);
    m_navButtonGroup->addButton(m_btnContact, 1);

    // 核心联动：点击左侧按钮，切换中间栏的堆栈页面
    connect(m_navButtonGroup, QOverload<int>::of(&QButtonGroup::idClicked), this, [=](int id){
        m_middleStack->setCurrentIndex(id);
    });

    m_navLayout->addWidget(m_btnAvatar, 0, Qt::AlignHCenter);
    m_navLayout->addWidget(m_lblNickname, 0, Qt::AlignHCenter);
    m_navLayout->addSpacing(20);
    m_navLayout->addWidget(m_btnChat, 0, Qt::AlignHCenter);
    m_navLayout->addWidget(m_btnContact, 0, Qt::AlignHCenter);
    m_navLayout->addStretch();

    // 默认锁定“消息”频道
    m_btnChat->setChecked(true);

    // 动态头像拉取
    QString myAvatarUrl = HttpManager::instance()->getMyAvatarUrl();
    myAvatarUrl.replace("localhost", "127.0.0.1");
    HttpManager::instance()->downloadImage(myAvatarUrl, [=](QPixmap originalImage) {
        QPixmap circularAvatar = makeCircularAvatar(originalImage, 45);
        m_btnAvatar->setText("");
        m_btnAvatar->setIcon(QIcon(circularAvatar));
        m_btnAvatar->setIconSize(QSize(45, 45));
    }, [=](QString err) {
                                               m_btnAvatar->setText("我");
                                           });

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

    // 搜索区 (保持不变)
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
    btnAddFriend->setObjectName("btnAddFriendTop");
    connect(btnAddFriend, &QPushButton::clicked, this, [=]() {
        AddFriendDialog dialog(this);
        dialog.exec();
    });

    searchLayout->addWidget(searchEdit);
    searchLayout->addWidget(btnAddFriend);

    // 中间栏的堆栈容器
    m_middleStack = new QStackedWidget(m_middleSidebar);

    // 页面 0：消息会话列表 (目前用一个空白提示占位，后续可开发)
    m_sessionList = new QListWidget(m_middleSidebar);
    m_sessionList->setFrameShape(QFrame::NoFrame);
    QListWidgetItem* emptyItem = new QListWidgetItem("暂无新消息", m_sessionList);
    emptyItem->setTextAlignment(Qt::AlignCenter);
    emptyItem->setForeground(QBrush(Qt::gray));
    m_sessionList->addItem(emptyItem);

    // 页面 1：联系人好友列表
    m_friendList = new QListWidget(m_middleSidebar);
    m_friendList->setFrameShape(QFrame::NoFrame);
    // 开启自定义右键菜单策略
    m_friendList->setContextMenuPolicy(Qt::CustomContextMenu);

    connect(m_friendList, &QListWidget::itemClicked, this, &MainWindow::onFriendItemClicked);
    // 监听右键点击事件，呼出菜单
    connect(m_friendList, &QListWidget::customContextMenuRequested, this, &MainWindow::showFriendListContextMenu);

    // 将两个列表塞入堆栈
    m_middleStack->addWidget(m_sessionList); // Index 0 (对应消息按钮)
    m_middleStack->addWidget(m_friendList);  // Index 1 (对应联系人按钮)

    m_sidebarLayout->addWidget(m_searchHeader);
    m_sidebarLayout->addWidget(m_middleStack); // 塞入的是 Stack 而不是单一列表了

    m_mainLayout->addWidget(m_middleSidebar);
}

void MainWindow::initRightContainer()
{
    m_rightStack = new QStackedWidget(this);
    m_rightStack->setObjectName("rightStack");

    // 1. 初始化聊天组件并嵌入堆栈
    m_chatWidget = new ChatWidget(this);
    m_rightStack->addWidget(m_chatWidget);

    m_newFriendWidget = new NewFriendWidget(this);
    m_rightStack->addWidget(m_newFriendWidget);

    // 监听同意好友的信号，静默刷新联系人列表！
    connect(m_newFriendWidget, &NewFriendWidget::friendListChanged, this, &MainWindow::fetchFriendList);

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

// =========================================================================
// 🌟 右键菜单拦截与渲染
// =========================================================================
void MainWindow::showFriendListContextMenu(const QPoint &pos)
{
    // 1. 获取当前鼠标点中的是哪个列表项
    QListWidgetItem *item = m_friendList->itemAt(pos);
    if (!item) return; // 点在空白处不响应

    // 2. 提取绑定的雪花 ID
    QString friendId = item->data(Qt::UserRole).toString();

    // 防御：如果是顶部的“新朋友”系统按钮，绝对不允许删除
    if (friendId == "SYSTEM_NEW_FRIEND") return;

    // 3. 构建现代风格的右键菜单
    QMenu menu(this);
    menu.setStyleSheet(
        "QMenu { background-color: #FFFFFF; border: 1px solid #CCCCCC; border-radius: 4px; padding: 5px; }"
        "QMenu::item { padding: 8px 25px; font-size: 14px; color: #333333; }"
        "QMenu::item:selected { background-color: #F0F0F0; color: #E64340; font-weight: bold; border-radius: 4px; }" // 悬停时变红警告
        );

    QAction *deleteAction = menu.addAction("删除该好友");

    // 4. 阻塞式弹出菜单，并捕获用户的选择
    QAction *selectedAction = menu.exec(m_friendList->mapToGlobal(pos));

    if (selectedAction == deleteAction) {
        // 剥离掉 "[在线]" 等状态文本，还原真实昵称
        QString cleanNickname = item->text().split(" [").first();
        handleDeleteFriend(friendId, cleanNickname);
    }
}

// =========================================================================
// 🌟 终极删除大杀器
// =========================================================================
void MainWindow::handleDeleteFriend(const QString& friendId, const QString& nickname)
{
    // 1. 二次确认防手抖防线（这是涉及删除操作的客户端金科玉律）
    int ret = QMessageBox::warning(this, "严重警告",
                                   QString("确定要将好友【%1】删除吗？\n此操作将清空你们所有的聊天记录且不可恢复！").arg(nickname),
                                   QMessageBox::Yes | QMessageBox::No,
                                   QMessageBox::No); // 默认焦点放在 No 上

    if (ret != QMessageBox::Yes) {
        return; // 用户怂了，终止操作
    }

    // 2. 发起夺命网络请求 (⚠️ 依然使用 127.0.0.1 避开 Qt IPv6 解析挂起陷阱)
    QString url = QString("http://127.0.0.1:9000/im-server/friend/delete?friendId=%1").arg(friendId);

    HttpManager::instance()->postJson(url, QJsonObject(), [=](QJsonObject res) {
        if (res["code"].toInt() == 200) {
            QMessageBox::information(this, "执行成功", "已彻底切断与该用户的羁绊。");

            // 🌟 战后重建 1：静默刷新左侧列表，让这人瞬间消失
            fetchFriendList();

            // 🌟 战后重建 2：如果此时你恰好停留在和这个人的聊天界面，立刻切回空白页！
            // 防止你还在对着一个不存在的人发消息导致底层系统崩溃
            if (m_currentChatFriendId == friendId) {
                m_rightStack->setCurrentWidget(m_emptyPage);
                m_currentChatFriendId = "";
            }

            // 🌟 战后重建 3：清理本地字典缓存
            m_friendMap.remove(friendId);

        } else {
            QMessageBox::warning(this, "删除失败", res["message"].toString());
        }
    }, [=](QString err) {
                                          QMessageBox::critical(this, "网络崩溃", "请求失败，请检查网络：" + err);
                                      });
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
        /* 头像占位符美化 */
        #leftNavbar #btnAvatar {
        background-color: #07C160; /* 微信绿 */
        color: #FFFFFF;
        border-radius: 20px; /* 变成正圆形 */
        font-weight: bold;
        }
        /* 导航栏按钮的悬停交互 (Hover) */
        #leftNavbar QPushButton:hover {
            background-color: #3D3D3D;
        }
        #lblMyNickname {
            color: #AAAAAA;
            font-size: 12px;
        }

        /* 核心：左侧导航按钮被“锁定”时的激活状态 */
        #leftNavbar QPushButton:checked {
            background-color: #3D3D3D;
            color: #07C160; /* 微信绿高亮 */
            border-radius: 4px;
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

