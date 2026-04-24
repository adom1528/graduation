#include "chatwidget.h"
#include "httpmanager.h"
#include <QDateTime>
#include <QMessageBox>

ChatWidget::ChatWidget(QWidget *parent)
    : QWidget(parent)
{
    initUI();
    bindEvents();
}

ChatWidget::~ChatWidget()
{
}

void ChatWidget::initUI()
{
    // 主垂直布局
    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(0);

    // 1. 顶部 Header 区
    QWidget* headerWidget = new QWidget(this);
    headerWidget->setFixedHeight(60);
    headerWidget->setObjectName("chatHeader"); // 预留 QSS 钩子
    QHBoxLayout* headerLayout = new QHBoxLayout(headerWidget);
    headerLayout->setContentsMargins(20, 0, 0, 0);

    m_lblHeaderName = new QLabel("未选择聊天对象", headerWidget);
    QFont headerFont = m_lblHeaderName->font();
    headerFont.setPointSize(14);
    headerFont.setBold(true);
    m_lblHeaderName->setFont(headerFont);
    headerLayout->addWidget(m_lblHeaderName);

    // 一条分割线隔离 Header 和 聊天区
    QFrame* line1 = new QFrame(this);
    line1->setFrameShape(QFrame::HLine);
    line1->setStyleSheet("color: #E0E0E0;");

    // 2. 中间 消息展示区
    m_textBrowser = new QTextBrowser(this);
    m_textBrowser->setFrameShape(QFrame::NoFrame);
    m_textBrowser->setOpenExternalLinks(true); // 允许直接打开超链接
    m_textBrowser->setObjectName("chatHistory");

    // 一条分割线隔离 聊天区 和 工具栏
    QFrame* line2 = new QFrame(this);
    line2->setFrameShape(QFrame::HLine);
    line2->setStyleSheet("color: #E0E0E0;");

    // 3. 工具栏区
    m_toolbarWidget = new QWidget(this);
    m_toolbarWidget->setFixedHeight(40);
    QHBoxLayout* toolbarLayout = new QHBoxLayout(m_toolbarWidget);
    toolbarLayout->setContentsMargins(10, 0, 0, 0);
    toolbarLayout->setAlignment(Qt::AlignLeft);

    m_btnImage = new QPushButton("图片", m_toolbarWidget);
    m_btnImage->setObjectName("btnSendImage");
    m_btnFile = new QPushButton("文件", m_toolbarWidget);
    m_btnFile->setObjectName("btnSendFile");
    m_btnImage->setFixedSize(60, 30);
    m_btnFile->setFixedSize(60, 30);
    toolbarLayout->addWidget(m_btnImage);
    toolbarLayout->addWidget(m_btnFile);

    // 4. 底部 输入区
    m_textInput = new QTextEdit(this);
    m_textInput->setFixedHeight(120);
    m_textInput->setFrameShape(QFrame::NoFrame);
    m_textInput->setObjectName("chatInput");

    // 发送按钮容器 (右对齐)
    QWidget* bottomWidget = new QWidget(this);
    QHBoxLayout* bottomLayout = new QHBoxLayout(bottomWidget);
    bottomLayout->setContentsMargins(0, 0, 20, 20);
    bottomLayout->setAlignment(Qt::AlignRight);

    m_btnSend = new QPushButton("发送(S)", bottomWidget);
    m_btnSend->setFixedSize(100, 36);
    m_btnSend->setObjectName("btnSendMsg");
    bottomLayout->addWidget(m_btnSend);

    // 组装主布局
    mainLayout->addWidget(headerWidget);
    mainLayout->addWidget(line1);
    mainLayout->addWidget(m_textBrowser, 1); // 占据剩余绝大空间
    mainLayout->addWidget(line2);
    mainLayout->addWidget(m_toolbarWidget);
    mainLayout->addWidget(m_textInput);
    mainLayout->addWidget(bottomWidget);
}

void ChatWidget::bindEvents()
{
    connect(m_btnSend, &QPushButton::clicked, this, [=]() {
        QString content = m_textInput->toPlainText().trimmed();
        if (content.isEmpty()) {
            return;
        }
        if (m_currentTargetId.isEmpty()) {
            QMessageBox::warning(this, "提示", "请先选择聊天对象");
            return;
        }

        // 发送信号委托外界处理网络请求
        emit textMessageSendRequested(m_currentTargetId, content);

        // 渲染上屏并清空输入框
        QString time = QDateTime::currentDateTime().toString("HH:mm:ss");
        appendMessage(1, "我", content, time, "", true);
        m_textInput->clear();
    });

    connect(m_btnImage, &QPushButton::clicked, this, [=]() {
        if (!m_currentTargetId.isEmpty()) emit imageSendRequested(m_currentTargetId);
    });

    connect(m_btnFile, &QPushButton::clicked, this, [=]() {
        if (!m_currentTargetId.isEmpty()) emit fileSendRequested(m_currentTargetId);
    });
}

void ChatWidget::setCurrentChat(const QString& targetId, const QString& nickname)
{
    m_currentTargetId = targetId;
    m_currentNickname = nickname;
    m_lblHeaderName->setText(nickname);
    m_textBrowser->clear();

    // 提示信息可在此处追加
    m_textBrowser->append(QString("<div style='color:gray; text-align:center;'>--- 正在与 %1 聊天 ---</div>").arg(nickname));
}

void ChatWidget::appendMessage(int type, const QString& senderName,
                               const QString& content,
                               const QString& createTime,
                               const QString& fileName,
                               bool isSelf)
{
    QString color = isSelf ? "#07C160" : "#0052D9"; // 绿色为己方，蓝色为对方
    QString prefix = isSelf ? "我" : senderName;
    if (type == 1 || type == 0) { // 文本消息 (有些旧系统可能没传 type，默认当文本处理)
        // 基础富文本格式化
        QString html = QString("<div style='margin-bottom: 10px;'>"
                               "<span style='color:%1; font-weight:bold;'>[%2] %3: </span>"
                               "<span>%4</span></div>")
                           .arg(color, createTime, prefix, content);
        m_textBrowser->append(html);
    } else if (type == 4) { // 图片消息
        QString url = content;

        HttpManager::instance()->getBytes(url, [=](QByteArray imgData) {
            // 【成功的回调】在这里处理图片二进制数据
            QString base64Img = QString::fromLatin1(imgData.toBase64());
            QString html = QString("<br><span style='color:%1;'>[%2] %3 (图片):</span><br><img src='data:image/png;base64,%4' width='150'>")
                               .arg(color, createTime, prefix, base64Img);
            m_textBrowser->append(html);

        }, [=](QString errorMsg) {
                                              // 【失败的回调】打印错误信息
                                              m_textBrowser->append(QString("<br><span style='color:gray;'>[%1 的图片加载失败: %2]</span>").arg(prefix, errorMsg));
                                          });
    }
    else if (type == 5) { // 文件消息
        QString fileHtml = QString("<br><span style='color:%1;'>[%2] %3 发送了文件: </span><br><a href='%4'>%5</a>")
                               .arg(color, createTime, prefix, content, fileName);
        m_textBrowser->append(fileHtml);
    }
}

