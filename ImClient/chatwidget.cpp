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

    // 防御性编程：切换聊天对象时，立刻清空前一个人的渲染队列，并解锁！
    m_msgQueue.clear();
    m_isRendering = false;


    QString tipHtml = QString("<table width='100%' border='0'><tr><td align='center'>"
                              "<span style='color:gray;'>--- 正在与 %1 聊天 ---</span>"
                              "</td></tr></table>").arg(nickname);
    m_textBrowser->append(tipHtml);
}

void ChatWidget::appendMessage(int type, const QString& senderName,
                               const QString& content,
                               const QString& createTime,
                               const QString& fileName,
                               bool isSelf)
{
    // 1. 将收到的消息打包成结构体
    MessageItem item = {type, senderName, content, createTime, fileName, isSelf};

    // 2. 塞入队列
    m_msgQueue.enqueue(item);

    // 3. 呼叫引擎尝试处理
    processNextMessage();
}

/**
 * @brief 消息队列处理引擎
 * 核心逻辑：采用 Qt 最稳定的 Table 布局法，彻底粉碎格式污染，强制实现左收右发
 */
void ChatWidget::processNextMessage()
{
    if (m_msgQueue.isEmpty() || m_isRendering) {
        return;
    }

    m_isRendering = true;
    MessageItem item = m_msgQueue.dequeue();

    // 核心属性
    QString align = item.isSelf ? "right" : "left";
    QString prefixColor = item.isSelf ? "#07C160" : "#0052D9";
    QString prefix = item.isSelf ? "我" : item.senderName;
    QString renderSessionId = m_currentTargetId;

    if (item.type == 1 || item.type == 0) {
        // ============================================================
        // 1. 文本消息
        // ============================================================
        QString html = QString(
                           "<table width='100%' border='0' style='margin-top: 5px; margin-bottom: 5px;'>"
                           "<tr><td align='%1'>"
                           "<span style='color: %2; font-size: 12px;'>[%3] %4</span><br>"
                           "<span style='color: black; font-size: 15px;'>%5</span>"
                           "</td></tr></table>"
                           ).arg(align, prefixColor, item.createTime, prefix, item.content);

        m_textBrowser->append(html);
        m_isRendering = false;
        processNextMessage();
    }
    else if (item.type == 5) {
        // ============================================================
        // 2. 文件消息
        // ============================================================
        QString fileHtml = QString(
                               "<table width='100%' border='0' style='margin-top: 5px; margin-bottom: 5px;'>"
                               "<tr><td align='%1'>"
                               "<span style='color: %2; font-size: 12px;'>[%3] %4 发送了文件</span><br>"
                               "<a href='%5' style='color: #E6A23C; font-size: 15px; text-decoration: underline;'>[📁 %6]</a>"
                               "</td></tr></table>"
                               ).arg(align, prefixColor, item.createTime, prefix, item.content, item.fileName);

        m_textBrowser->append(fileHtml);
        m_isRendering = false;
        processNextMessage();
    }
    else if (item.type == 4) {
        // ============================================================
        // 3. 图片消息
        // ============================================================
        QString url = item.content;
        HttpManager::instance()->getBytes(url, [=](QByteArray imgData) {
            if (m_currentTargetId != renderSessionId) {
                m_isRendering = false;
                processNextMessage();
                return;
            }

            QString base64Img = QString::fromLatin1(imgData.toBase64());
            QString html = QString(
                               "<table width='100%' border='0' style='margin-top: 5px; margin-bottom: 5px;'>"
                               "<tr><td align='%1'>"
                               "<span style='color: %2; font-size: 12px;'>[%3] %4 发送了图片</span><br>"
                               "<img src='data:image/png;base64,%5' width='150'>"
                               "</td></tr></table>"
                               ).arg(align, prefixColor, item.createTime, prefix, base64Img);

            m_textBrowser->append(html);
            m_isRendering = false;
            processNextMessage();

        }, [=](QString errorMsg) {
                                              if (m_currentTargetId == renderSessionId) {
                                                  m_textBrowser->append(QString("<table width='100%'><tr><td align='%1'><span style='color:gray;'>[%2 图片加载失败]</span></td></tr></table>")
                                                                            .arg(align, prefix));
                                              }
                                              m_isRendering = false;
                                              processNextMessage();
                                          });
    }
}
