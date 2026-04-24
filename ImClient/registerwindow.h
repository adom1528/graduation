#ifndef REGISTERWINDOW_H
#define REGISTERWINDOW_H

#include <QDialog>

namespace Ui {
class RegisterWindow;
}

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

private:
    Ui::RegisterWindow *ui;
};

#endif // REGISTERWINDOW_H
