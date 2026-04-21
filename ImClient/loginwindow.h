#ifndef LOGINWINDOW_H
#define LOGINWINDOW_H

#include <QDialog>

namespace Ui {
class LoginWindow;
}

class LoginWindow : public QDialog
{
    Q_OBJECT

public:
    explicit LoginWindow(QWidget *parent = nullptr);
    ~LoginWindow();

private slots:
    void onLoginClicked(); // 登录
    void onBtnGoToRegisterClicked(); // 跳转注册

private:
    Ui::LoginWindow *ui;
};

#endif // LOGINWINDOW_H
