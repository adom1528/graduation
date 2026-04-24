#ifndef NEWFRIENDWIDGET_H
#define NEWFRIENDWIDGET_H

#include <QWidget>
#include <QListWidget>
#include <QVBoxLayout>
#include <QLabel>
#include <QPushButton>

/**
 * @brief 新朋友（好友申请）管理面板
 */
class NewFriendWidget : public QWidget
{
    Q_OBJECT

public:
    explicit NewFriendWidget(QWidget *parent = nullptr);

    // 开放给外部调用的接口：触发网络请求，拉取并刷新列表
    void loadPendingRequests();

signals:
    // 🌟 核心通讯信号：当用户同意了某个申请后，通知主界面刷新左侧的通讯录！
    void friendListChanged();

private:
    void initUI();
    // 发起同意或拒绝的网络请求 (action: 1-同意，2-拒绝)
    void handleRequest(const QString& requestId, int action);

private:
    QLabel* m_lblTitle;
    QListWidget* m_requestList;
};

#endif // NEWFRIENDWIDGET_H
