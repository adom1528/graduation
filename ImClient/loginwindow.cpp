#include "loginwindow.h"
#include "ui_loginwindow.h"
#include "httpmanager.h"
#include "registerwindow.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QUrlQuery>

LoginWindow::LoginWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::LoginWindow)
{
    ui->setupUi(this);

    // 绑定登录按钮点击事件
    connect(ui->btnLogin, &QPushButton::clicked, this, &LoginWindow::onLoginClicked);
    connect(ui->btnGoToRegister,&QPushButton::clicked, this, &::LoginWindow::onBtnGoToRegisterClicked);
}

LoginWindow::~LoginWindow()
{
    delete ui;
}

// 登录
void LoginWindow::onLoginClicked()
{
    QString username = ui->lineEditUsername->text();
    QString password = ui->lineEditPassword->text();

    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "账号密码不能为空");
        return;
    }

    QJsonObject loginData;
    loginData["username"] = username;
    loginData["password"] = password;

    // 一行代码搞定网络请求 + 回调 + Token 存储
    HttpManager::instance()->postJson("http://localhost:9000/im-auth/auth/login", loginData,
                                      [=](QJsonObject res) {
                                          // 成功回调：假设后端返回 { "code": 200, "data": "eyJhb..." }
                                          int code = res["code"].toInt();
                                          if (code == 200) {
                                              QString token = res["data"].toString();
                                              // 1. 把 Token 存入核武器的弹药库，以后所有请求自动带上！
                                              HttpManager::instance()->setToken(token);

                                              // 2. 核心魔法：告诉主程序，登录成功啦，你可以关闭这个窗口了！
                                              this->accept();
                                          } else {
                                              QMessageBox::warning(this, "登录失败", res["message"].toString());
                                          }
                                      },
                                      [=](QString err) {
                                          // 失败回调：网络断开、服务器报错等
                                          QMessageBox::critical(this, "网络错误", err);
                                      }
                                      );
}

// 跳转登录
void LoginWindow::onBtnGoToRegisterClicked()
{
    // 实例化注册弹窗 (传入 this 作为父对象，保证内存安全释放)
    RegisterWindow regDialog(this);

    // 用 Lambda 表达式监听刚才定义的 registerSuccess 信号
    connect(&regDialog, &RegisterWindow::registerSuccess, this, [=](const QString& regUsername) {
        // 自动把刚刚注册成功的账号，填进登录界的账号输入框里！
        ui->lineEditUsername->setText(regUsername);

        // 顺手清空密码框，让用户体验拉满
        ui->lineEditPassword->clear();

        // 让密码框直接获取光标焦点，用户连鼠标都不用点，直接输密码就能登！
        ui->lineEditPassword->setFocus();
    });

    // 弹出独立窗口 (阻塞模式，关掉之前不能点背后的登录框)
    regDialog.exec();
}
