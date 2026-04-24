#include "registerwindow.h"
#include "ui_registerwindow.h"
#include "httpmanager.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QDebug>

RegisterWindow::RegisterWindow(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::RegisterWindow)
{
    ui->setupUi(this);
    // 绑定注册按钮点击事件
    connect(ui->btnSubmitRegister, &QPushButton::clicked, this, &RegisterWindow::onBtnSubmitRegisterClicked);
}

RegisterWindow::~RegisterWindow()
{
    delete ui;
}

void RegisterWindow::onBtnSubmitRegisterClicked()
{
    QString username = ui->lineEditUsername->text().trimmed();
    QString nickname = ui->lineEditNickname->text().trimmed();
    QString password = ui->lineEditPassword->text();
    QString confirmPassword = ui->lineEditConfirmPassword->text();

    // 🛡️ 防线 1：非空校验
    if (username.isEmpty() || password.isEmpty() || confirmPassword.isEmpty()) {
        QMessageBox::warning(this, "提示", "账号和密码不能为空");
        return;
    }

    // 🛡️ 防线 2：密码一致性校验 (纯前端拦截，绝不浪费后端资源)
    if (password != confirmPassword) {
        QMessageBox::warning(this, "提示", "两次输入的密码不一致！");
        return;
    }

    // 组装 JSON 弹药
    QJsonObject reqData;
    reqData["username"] = username;
    reqData["nickname"] = nickname; // 允许为空，后端会处理
    reqData["password"] = password;

    //qDebug() << nickname << "kongde";
    // 发射请求 走网关 9000 端口
    HttpManager::instance()->postJson("http://localhost:9000/im-auth/auth/register", reqData,
                                      [=](QJsonObject res) {
                                          int code = res["code"].toInt();
                                          if (code == 200) {
                                              QMessageBox::information(this, "成功", "注册成功，快去登录吧！");

                                              // 🌟 核心联动：发射信号，把账号传出去
                                              emit registerSuccess(username);

                                              // 功成身退，关闭注册窗口
                                              this->accept();

                                          } else {
                                              // 后端返回的业务报错，比如“该账号已被注册”
                                              QMessageBox::warning(this, "注册失败", res["message"].toString());
                                          }
                                      },
                                      [=](QString err) {
                                          QMessageBox::critical(this, "网络错误", err);
                                      }
                                      );
}
