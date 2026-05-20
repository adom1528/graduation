#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QRadioButton>
#include <QButtonGroup>
#include <QVBoxLayout>
#include <QHBoxLayout>

class RegisterWindow : public QDialog
{
    Q_OBJECT

public:
    explicit RegisterWindow(QWidget *parent = nullptr);
    ~RegisterWindow();

signals:
    // 信号：注册成功后，把账号发射出去，让登录界面去接
    void registerSuccess(const QString& username);

private slots:
    void onBtnSubmitRegisterClicked();
    void onBtnCancelClicked();
    void onBtnAvatarClicked();

private:
    void initUI();    // 纯代码布局初始化
    void initStyle(); // 现代极简 QSS 样式装配

    // --- UI 控件指针 ---
    QLabel *lblTitle;
    QPushButton *btnAvatar;

    // 输入框区
    QLineEdit *editUsername;
    QLineEdit *editNickname;
    QLineEdit *editPassword;
    QLineEdit *editConfirmPassword;

    // 性别单选区
    QRadioButton *radioMale;
    QRadioButton *radioFemale;
    QRadioButton *radioSecret;
    QButtonGroup *genderGroup;

    // 按钮区
    QPushButton *btnRegister;
    QPushButton *btnCancel;

    // 预置默认头像 URL (结合咱们之前 MinIO 的设定)
    QString currentAvatarUrl;
};

#endif // REGISTERWINDOW_H
