#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void onBtnLoginClicked();
    void onBtnGoRegisterClicked();

    // 🌟 接收注册界面发来的成功信号，实现账号自动回填
    void onRegisterSuccess(const QString& username);

private:
    void initUI();
    void initStyle();

    // UI 控件指针
    QLabel *lblTitle;
    QLabel *lblSubtitle;

    QLineEdit *editUsername;
    QLineEdit *editPassword;

    QPushButton *btnLogin;
    QPushButton *btnGoRegister;
};

#endif // LOGINWINDOW_H
