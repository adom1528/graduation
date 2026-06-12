#include "addfrienddialog.h"
#include "httpmanager.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include <QLabel>
#include <QMessageBox>
#include <QInputDialog>
#include <QPainter>
#include <QPainterPath>
#include <QPointer>

// =========================================================================
//  Qt 绘图：将任意方形 QPixmap 切割为带抗锯齿的高清圆形头像
// =========================================================================
static QPixmap makeCircularAvatar(const QPixmap &src, int size)
{
    if (src.isNull()) return QPixmap();
    QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, size, size, scaled);
    return result;
}

AddFriendDialog::AddFriendDialog(QWidget *parent) : QDialog(parent)
{
    // 设置弹窗基础属性：模态窗口（阻塞主界面点击）、固定大小
    this->setWindowTitle("添加好友");
    this->setFixedSize(400, 500);
    this->setWindowModality(Qt::ApplicationModal);

    initUI();
    bindEvents();
}

AddFriendDialog::~AddFriendDialog() {}

void AddFriendDialog::initUI()
{
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(20, 20, 20, 20);
    mainLayout->setSpacing(15);

    // 1. 顶部搜索区
    QHBoxLayout* searchLayout = new QHBoxLayout();
    m_searchEdit = new QLineEdit(this);
    m_searchEdit->setPlaceholderText("请输入对方昵称进行模糊搜索...");
    m_searchEdit->setFixedHeight(36);

    m_searchBtn = new QPushButton("搜索", this);
    m_searchBtn->setFixedSize(80, 36);
    m_searchBtn->setStyleSheet("background-color: #07C160; color: white; border: none; border-radius: 4px; font-weight: bold;");

    searchLayout->addWidget(m_searchEdit);
    searchLayout->addWidget(m_searchBtn);

    // 2. 搜索结果列表
    m_resultList = new QListWidget(this);
    m_resultList->setFrameShape(QFrame::NoFrame);
    m_resultList->setStyleSheet("QListWidget { background-color: #F9F9F9; border-radius: 8px; border: 1px solid #E0E0E0; }"
                                "QListWidget::item { padding: 5px; border-bottom: 1px solid #EEEEEE; }");

    mainLayout->addLayout(searchLayout);
    mainLayout->addWidget(m_resultList);
}

void AddFriendDialog::bindEvents()
{
    // 点击搜索按钮或在输入框按回车，触发搜索
    connect(m_searchBtn, &QPushButton::clicked, this, &AddFriendDialog::performSearch);
    connect(m_searchEdit, &QLineEdit::returnPressed, this, &AddFriendDialog::performSearch);
}

void AddFriendDialog::performSearch()
{
    QString keyword = m_searchEdit->text().trimmed();
    if (keyword.isEmpty()) {
        QMessageBox::warning(this, "提示", "搜索内容不能为空！");
        return;
    }

    // 禁用按钮防狂点
    m_searchBtn->setEnabled(false);
    m_searchBtn->setText("搜索中...");
    m_resultList->clear(); // 清空旧数据

    // 组装搜索请求
    QString url = "http://10.196.229.92:9000/im-server/user/search";
    QVariantMap params;
    params["nickname"] = keyword;

    HttpManager::instance()->get(url, params, [=](QJsonObject res) {
        // 恢复按钮状态
        m_searchBtn->setEnabled(true);
        m_searchBtn->setText("搜索");

        int code = res["code"].toInt();
        if (code == 200) {
            QJsonArray data = res["data"].toArray();

            if (data.isEmpty()) {
                QListWidgetItem* emptyItem = new QListWidgetItem("未搜索到相关用户", m_resultList);
                emptyItem->setTextAlignment(Qt::AlignCenter);
                emptyItem->setForeground(QBrush(Qt::gray));
                return;
            }

            // 遍历渲染搜到的用户
            for (int i = 0; i < data.size(); ++i) {
                QJsonObject userObj = data[i].toObject();
                QString userId = userObj["id"].toVariant().toString();
                QString nickname = userObj["nickname"].toString();

                // 尝试从后端提取头像 URL
                QString avatarUrl = userObj["avatar"].toString();

                QListWidgetItem* item = new QListWidgetItem(m_resultList);
                item->setSizeHint(QSize(0, 60)); // 设置行高

                QWidget* customWidget = new QWidget(m_resultList);
                QHBoxLayout* itemLayout = new QHBoxLayout(customWidget);
                itemLayout->setContentsMargins(10, 0, 10, 0);

                //  初始化的头像标签
                QLabel* avatarLabel = new QLabel(customWidget);
                avatarLabel->setFixedSize(40, 40);
                avatarLabel->setAlignment(Qt::AlignCenter);

                //  使用 QPointer 保护我们的 UI 控件！
                // 如果用户频繁搜索导致这个 Label 被提前销毁，QPointer 会自动变成 null，防止程序崩溃
                QPointer<QLabel> safeLabel(avatarLabel);

                if (!avatarUrl.isEmpty()) {
                    // 如果有头像，先给一个低调的灰色加载背景
                    avatarLabel->setStyleSheet("background-color: #E0E0E0; border-radius: 20px;");

                    QString safeUrl = avatarUrl;
                    safeUrl.replace("localhost", "10.196.229.92");

                    // 发起异步拉取
                    HttpManager::instance()->downloadImage(safeUrl, [safeLabel, nickname](QPixmap originalImage) {
                        // 🛡️ 极其关键的安全判断：只有当界面还在时，才去渲染
                        if (safeLabel) {
                            safeLabel->setPixmap(makeCircularAvatar(originalImage, 40));
                            safeLabel->setStyleSheet(""); // 清空背景色
                        }
                    }, [safeLabel, nickname](QString err) {
                                                               // 失败兜底：显示名字首字母
                                                               if (safeLabel) {
                                                                   safeLabel->setText(nickname.left(1));
                                                                   safeLabel->setStyleSheet("background-color: #0052D9; color: white; border-radius: 20px; font-weight: bold; font-size: 16px;");
                                                               }
                                                           });
                } else {
                    // 如果后端连 URL 都没传，直接走兜底逻辑
                    avatarLabel->setText(nickname.left(1));
                    avatarLabel->setStyleSheet("background-color: #0052D9; color: white; border-radius: 20px; font-weight: bold; font-size: 16px;");
                }

                // 名字标签
                QLabel* nameLabel = new QLabel(nickname, customWidget);
                nameLabel->setStyleSheet("font-size: 15px; color: #333333;");

                // 申请按钮
                QPushButton* addBtn = new QPushButton("加好友", customWidget);
                addBtn->setFixedSize(60, 30);
                addBtn->setStyleSheet("QPushButton { background-color: #FFFFFF; color: #07C160; border: 1px solid #07C160; border-radius: 4px; }"
                                      "QPushButton:hover { background-color: #07C160; color: #FFFFFF; }");

                // 点击【加好友】按钮，触发发送申请逻辑
                connect(addBtn, &QPushButton::clicked, this, [=]() {
                    sendFriendRequest(userId);
                });

                itemLayout->addWidget(avatarLabel);
                itemLayout->addWidget(nameLabel, 1); // 名字占据弹簧空间
                itemLayout->addWidget(addBtn);

                m_resultList->setItemWidget(item, customWidget);
            }
        } else {
            QMessageBox::warning(this, "搜索失败", res["message"].toString());
        }
    }, [=](QString err) {
                                     m_searchBtn->setEnabled(true);
                                     m_searchBtn->setText("搜索");
                                     QMessageBox::critical(this, "网络错误", "搜索请求失败：" + err);
                                 });
}

void AddFriendDialog::sendFriendRequest(const QString& targetId)
{
    // 弹出一个输入框，让用户填写验证信息 (对应你后端的 reason 字段)
    bool ok;
    QString reason = QInputDialog::getText(this, "好友申请",
                                           "请输入验证信息：", QLineEdit::Normal,
                                           "你好，我是...", &ok);
    if (!ok) return; // 用户点击了取消

    // 我们巧妙地利用 HttpManager::postJson 发送请求
    // 因为 SpringBoot 的 @RequestParam 支持从 URL 提取 POST 参数，我们直接拼在 URL 后面
    QString url = QString("http://10.196.229.92:9000/im-server/friend/request?targetUserId=%1&reason=%2")
                      .arg(targetId, reason);

    // 发送一个空的 JSON 体即可
    QJsonObject emptyBody;

    HttpManager::instance()->postJson(url, emptyBody, [=](QJsonObject res) {
        int code = res["code"].toInt();
        if (code == 200) {
            QMessageBox::information(this, "成功", "好友请求已发送，请等待对方通过！");
            // 申请成功后可以选择关闭弹窗
            this->accept();
        } else {
            // 这里会完美接住你后端抛出的 RuntimeException ("不能添加自己"、"已经是好友"等)
            QMessageBox::warning(this, "申请失败", res["message"].toString());
        }
    }, [=](QString err) {
                                          QMessageBox::critical(this, "网络错误", "发送申请失败：" + err);
                                      });
}
