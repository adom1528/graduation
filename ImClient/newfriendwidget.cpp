#include "newfriendwidget.h"
#include "httpmanager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QHBoxLayout>
#include <QMessageBox>

NewFriendWidget::NewFriendWidget(QWidget *parent) : QWidget(parent)
{
    initUI();
}

void NewFriendWidget::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 1. 顶部标题
    m_lblTitle = new QLabel("新的朋友", this);
    QFont titleFont = m_lblTitle->font();
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    m_lblTitle->setFont(titleFont);

    // 2. 申请列表
    m_requestList = new QListWidget(this);
    m_requestList->setFrameShape(QFrame::NoFrame);
    m_requestList->setStyleSheet(
        "QListWidget { background-color: transparent; outline: none; }"
        "QListWidget::item { background-color: #F9F9F9; border-radius: 8px; margin-bottom: 10px; border: 1px solid #E0E0E0; }"
        "QListWidget::item:hover { background-color: #F0F0F0; }"
        );

    mainLayout->addWidget(m_lblTitle);
    mainLayout->addWidget(m_requestList);
}

void NewFriendWidget::loadPendingRequests()
{
    m_requestList->clear(); // 清空旧数据

    QString url = "http://localhost:9000/im-server/friend/request-list";

    HttpManager::instance()->get(url, [=](QJsonObject res) {
        int code = res["code"].toInt();
        if (code == 200) {
            QJsonArray data = res["data"].toArray();

            if (data.isEmpty()) {
                QListWidgetItem* emptyItem = new QListWidgetItem("暂无新的好友申请", m_requestList);
                emptyItem->setTextAlignment(Qt::AlignCenter);
                emptyItem->setForeground(QBrush(Qt::gray));
                return;
            }

            for (int i = 0; i < data.size(); ++i) {
                QJsonObject reqObj = data[i].toObject();
                // 🌟 注意：这里取的是 VO 里的 requestId！
                QString requestId = reqObj["requestId"].toVariant().toString();
                QString nickname = reqObj["nickname"].toString();
                QString reason = reqObj["reason"].toString();

                QListWidgetItem* item = new QListWidgetItem(m_requestList);
                item->setSizeHint(QSize(0, 80)); // 调高一点，容纳两行文字

                QWidget* customWidget = new QWidget(m_requestList);
                QHBoxLayout* itemLayout = new QHBoxLayout(customWidget);
                itemLayout->setContentsMargins(15, 10, 15, 10);

                // 头像
                QLabel* avatarLabel = new QLabel(nickname.left(1), customWidget);
                avatarLabel->setFixedSize(50, 50);
                avatarLabel->setAlignment(Qt::AlignCenter);
                avatarLabel->setStyleSheet("background-color: #07C160; color: white; border-radius: 25px; font-size: 18px; font-weight: bold;");

                // 名字与申请理由容器 (垂直排列)
                QVBoxLayout* infoLayout = new QVBoxLayout();
                QLabel* nameLabel = new QLabel(nickname, customWidget);
                nameLabel->setStyleSheet("font-size: 16px; font-weight: bold; color: #333333;");
                QLabel* reasonLabel = new QLabel("留言: " + reason, customWidget);
                reasonLabel->setStyleSheet("font-size: 13px; color: #888888;");
                infoLayout->addWidget(nameLabel);
                infoLayout->addWidget(reasonLabel);

                // 操作按钮
                QPushButton* btnAgree = new QPushButton("同意", customWidget);
                btnAgree->setFixedSize(60, 30);
                btnAgree->setStyleSheet("background-color: #07C160; color: white; border: none; border-radius: 4px;");

                QPushButton* btnReject = new QPushButton("拒绝", customWidget);
                btnReject->setFixedSize(60, 30);
                btnReject->setStyleSheet("background-color: #E64340; color: white; border: none; border-radius: 4px;");

                // 绑定按钮事件
                connect(btnAgree, &QPushButton::clicked, this, [=]() { handleRequest(requestId, 1); });
                connect(btnReject, &QPushButton::clicked, this, [=]() { handleRequest(requestId, 2); });

                itemLayout->addWidget(avatarLabel);
                itemLayout->addLayout(infoLayout, 1);
                itemLayout->addWidget(btnAgree);
                itemLayout->addWidget(btnReject);

                m_requestList->setItemWidget(item, customWidget);
            }
        }
    }, [=](QString err) {
                                     QMessageBox::critical(this, "网络错误", "获取申请列表失败: " + err);
                                 });
}

void NewFriendWidget::handleRequest(const QString& requestId, int action)
{
    // 利用我们刚加的 HttpManager::postJson 发送请求，参数拼在 URL 上适配 SpringBoot
    QString url = QString("http://localhost:9000/im-server/friend/handle-request?requestId=%1&action=%2")
                      .arg(requestId).arg(action);

    HttpManager::instance()->postJson(url, QJsonObject(), [=](QJsonObject res) {
        if (res["code"].toInt() == 200) {
            QMessageBox::information(this, "操作成功", action == 1 ? "已添加好友！" : "已拒绝申请。");
            // 重新拉取列表，刚才被处理的人就会消失
            loadPendingRequests();

            // 🌟 如果是同意添加好友，必须发射信号通知主界面刷新！
            if (action == 1) {
                emit friendListChanged();
            }
        } else {
            QMessageBox::warning(this, "操作失败", res["message"].toString());
        }
    }, [=](QString err) {
                                          QMessageBox::critical(this, "网络错误", err);
                                      });
}
