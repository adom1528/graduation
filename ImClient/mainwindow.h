#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "chatwidget.h"
#include <QMainWindow>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QStackedWidget>
#include <QListWidget>
#include <QPushButton>
#include <QWidget>
#include <QFrame>
#include <QTimer>
#include <QWebsocket>


/**
 * @brief 核心主窗口类
 * 采用三段式解耦布局：导航栏(Left) + 列表区(Middle) + 内容区(Right)
 */
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private:
    /**
     * @brief 初始化全局布局结构
     */
    void initGlobalLayout();

    /**
     * @brief 初始化左侧导航栏
     */
    void initLeftNavbar();

    /**
     * @brief 初始化中间列表区
     */
    void initMiddleSidebar();

    /**
     * @brief 初始化右侧容器区
     */
    void initRightContainer();

    /**
     * @brief 初始化全局 QSS 样式表
     * 采用深色导航、浅灰列表、纯白工作区的三段式经典配色
     */
    void initStyleSheet();

    /**
     * @brief fetchFriendList 拉取好友列表
     */
    void fetchFriendList();

    /**
     * @brief fetchChatHistory 拉取某个好友的聊天记录
     * @param friendId 好友ID
     */
    void fetchChatHistory(QString friendId);

    // 责把所有类型的数据转化为富文本上屏
    //void renderMessageToUI(int type, const QString &senderName, const QString &content, const QString &fileName, bool isSelf);

private slots:
    /**
     * @brief 处理来自 ChatWidget 的文本消息发送请求
     * @param targetId 目标用户雪花 ID
     * @param content 消息内容
     */
    void handleSendMessageRequest(const QString& targetId, const QString& content);

    /**
     * @brief 处理好友列表项点击事件
     */
    void onFriendItemClicked(QListWidgetItem *item);

    // ... 其他 WebSocket 相关槽函数 ...
    /**
     * @brief onConnected 初始化 WebSocket 长连接
     */
    void onConnected();

    /**
     * @brief onDisconnected 安全断开 WebSocket 长连接
     */
    void onDisconnected();

    /**
    * @brief WebSocket 接收漏斗：解析收到的数据并路由至对应的 UI 组件
    */
    void onTextMessageReceived(QString message);

private:
    // 全局中心组件与主布局
    QWidget* m_centralWidget;
    QHBoxLayout* m_mainLayout;

    // 第一段：左侧导航栏
    QFrame* m_leftNavbar;
    QVBoxLayout* m_navLayout;
    QPushButton* m_btnAvatar;
    QPushButton* m_btnChat;
    QPushButton* m_btnContact;

    // 第二段：中间侧边栏（包含搜索与列表）
    QFrame* m_middleSidebar;
    QVBoxLayout* m_sidebarLayout;
    QWidget* m_searchHeader;
    QListWidget* m_friendList;

    // 第三段：右侧堆栈式工作区
    QStackedWidget* m_rightStack;
    QWidget* m_emptyPage;

    // 核心聊天组件
    ChatWidget* m_chatWidget;
    QWebSocket* m_webSocket;
    QTimer* m_heartbeatTimer;
    QString m_myToken;
    QMap<QString, QString> m_friendMap; // 用于 ID 到昵称的映射

};

#endif // MAINWINDOW_H
