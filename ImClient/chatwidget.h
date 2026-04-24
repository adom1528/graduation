#ifndef CHATWIDGET_H
#define CHATWIDGET_H

#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QTextBrowser>
#include <QTextEdit>
#include <QPushButton>
#include <QQueue>

/**
 * @brief 内部消息包装体，用于排队渲染
 */
struct MessageItem {
    int type;
    QString senderName;
    QString content;
    QString createTime;
    QString fileName;
    bool isSelf;
};


/**
 * @brief 聊天面板组件
 * 负责单一会话的 UI 展示与消息输入，不包含底层网络通信逻辑
 */
class ChatWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ChatWidget(QWidget *parent = nullptr);
    ~ChatWidget();

    /**
     * @brief 切换当前聊天对象
     * @param targetId 对方的用户 ID
     * @param nickname 对方的展示昵称
     */
    void setCurrentChat(const QString& targetId, const QString& nickname);

    /**
     * @brief 向面板追加渲染单条消息
     * @param type 消息类型 0/1：文本消息；3：群聊消息；4：图片；5：文件
     * @param senderName 发送者名称
     * @param content 消息体内容
     * @param createTime 消息创建时间
     * @param isSelf 是否为当前登录用户发送
     */
    void appendMessage(int type, const QString& senderName,
                       const QString& content,
                       const QString& createTime,
                       const QString& fileName,
                       bool isSelf);

signals:
    /**
     * @brief 触发文本消息发送请求
     * @param targetId 目标用户 ID
     * @param content 文本内容
     */
    void textMessageSendRequested(const QString& targetId, const QString& content);

    /**
     * @brief 触发图片发送请求
     */
    void imageSendRequested(const QString& targetId);

    /**
     * @brief 触发文件发送请求
     */
    void fileSendRequested(const QString& targetId);

private:
    /**
     * @brief 初始化内部 UI 布局
     */
    void initUI();

    /**
     * @brief 绑定内部交互事件
     */
    void bindEvents();

    /**
     * @brief processNextMessage 排队渲染引擎核心
     */
    void processNextMessage();

private:
    QString m_currentTargetId;
    QString m_currentNickname;

    QQueue<MessageItem> m_msgQueue;// 消息队列
    bool m_isRendering = false; // 渲染锁：当前是否正在画图

    // UI 组件指针
    QLabel* m_lblHeaderName;        // 顶部聊天对象名称
    QTextBrowser* m_textBrowser;    // 历史消息展示区
    QWidget* m_toolbarWidget;       // 中部工具栏 (包含图片、文件按钮)
    QPushButton* m_btnImage;
    QPushButton* m_btnFile;
    QTextEdit* m_textInput;         // 底部输入区
    QPushButton* m_btnSend;         // 发送按钮
};

#endif // CHATWIDGET_H
