#ifndef ADDFRIENDDIALOG_H
#define ADDFRIENDDIALOG_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QListWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>

/**
 * @brief 添加好友弹窗组件
 * 采用独立黑盒设计，内部自行闭环“搜索”与“发送申请”的网络请求
 */
class AddFriendDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AddFriendDialog(QWidget *parent = nullptr);
    ~AddFriendDialog();

private:
    /**
     * @brief 初始化弹窗 UI
     */
    void initUI();

    /**
     * @brief 绑定按钮与网络请求事件
     */
    void bindEvents();

    /**
     * @brief 执行模糊搜索网络请求
     */
    void performSearch();

    /**
     * @brief 发送好友申请网络请求
     * @param targetId 目标用户雪花 ID
     */
    void sendFriendRequest(const QString& targetId);

private:
    // UI 控件
    QLineEdit* m_searchEdit;       // 顶部搜索框
    QPushButton* m_searchBtn;      // 搜索触发按钮
    QListWidget* m_resultList;     // 搜索结果展示列表（可能搜出多个同名/模糊匹配的用户）
};

#endif // ADDFRIENDDIALOG_H
