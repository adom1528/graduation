#include "loginwindow.h"
#include "registerwindow.h"
#include "httpmanager.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QDebug>

LoginWindow::LoginWindow(QWidget *parent) :
    QDialog(parent)
{
    // 1. 初始化纯代码 UI 与样式
    initUI();
    initStyle();

    // 2. 窗口基础设置
    this->setWindowTitle("系统登录");
    this->setFixedSize(400, 500);
}

LoginWindow::~LoginWindow()
{
    // 纯代码布局，无需 delete ui，对象树会自动回收内存
}

void LoginWindow::initUI()
{
    // --- 1. 创建控件 ---
    lblTitle = new QLabel("Welcome", this);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setObjectName("lblTitle");

    lblSubtitle = new QLabel("微服务即时通讯系统", this);
    lblSubtitle->setAlignment(Qt::AlignCenter);
    lblSubtitle->setObjectName("lblSubtitle");

    editUsername = new QLineEdit(this);
    editUsername->setPlaceholderText("请输入账号");

    editPassword = new QLineEdit(this);
    editPassword->setPlaceholderText("请输入密码");
    editPassword->setEchoMode(QLineEdit::Password);

    btnLogin = new QPushButton("登 录", this);
    btnLogin->setObjectName("btnPrimary");

    btnGoRegister = new QPushButton("没有账号？立即注册", this);
    btnGoRegister->setObjectName("btnSecondary");

    // --- 2. 排版布局 (嵌套布局，完美解决文字腰斩 Bug) ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 60, 40, 40);
    mainLayout->setSpacing(20);

    // 将主副标题放进一个独立的子布局中，安全地拉近距离
    QVBoxLayout *titleLayout = new QVBoxLayout();
    titleLayout->setSpacing(5);
    titleLayout->addWidget(lblTitle);
    titleLayout->addWidget(lblSubtitle);

    mainLayout->addLayout(titleLayout);
    mainLayout->addSpacing(20);

    mainLayout->addWidget(editUsername);
    mainLayout->addWidget(editPassword);

    mainLayout->addSpacing(20);
    mainLayout->addWidget(btnLogin);
    mainLayout->addWidget(btnGoRegister);
    mainLayout->addStretch(); // 弹簧，把所有控件往上顶

    // --- 3. 信号与槽 ---
    connect(btnLogin, &QPushButton::clicked, this, &LoginWindow::onBtnLoginClicked);
    connect(btnGoRegister, &QPushButton::clicked, this, &LoginWindow::onBtnGoRegisterClicked);
}

void LoginWindow::initStyle()
{
    // 🎨 高级工业风 QSS 皮肤
    QString qss = R"(
        QDialog {
            background-color: #FFFFFF;
        }
        #lblTitle {
            font-size: 36px;
            font-weight: bold;
            color: #333333;
            font-family: "Segoe UI", "Microsoft YaHei";
        }
        #lblSubtitle {
            font-size: 14px;
            color: #888888;
            font-family: "Microsoft YaHei";
        }
        QLineEdit {
            height: 45px;
            border: 1px solid #E0E0E0;
            border-radius: 6px;
            padding-left: 15px;
            font-size: 15px;
            background-color: #F8F9FA;
            color: #333333;
        }
        QLineEdit:focus {
            border: 2px solid #0078D4;
            background-color: #FFFFFF;
        }
        #btnPrimary {
            height: 48px;
            background-color: #0078D4;
            color: white;
            border-radius: 6px;
            font-size: 18px;
            font-weight: bold;
            letter-spacing: 2px;
        }
        #btnPrimary:hover {
            background-color: #005A9E;
        }
        #btnPrimary:pressed {
            background-color: #004578;
        }
        #btnSecondary {
            height: 30px;
            background-color: transparent;
            color: #0078D4;
            border: none;
            font-size: 14px;
        }
        #btnSecondary:hover {
            text-decoration: underline;
        }
        QMessageBox {
            background-color: #FFFFFF;
        }
        QMessageBox QLabel {
            color: #333333; /* 强制深色文字，破解白底白字隐身术 */
            font-size: 14px;
            min-height: 40px; /* 给文字留出呼吸空间 */
        }
        QMessageBox QPushButton {
            background-color: #0078D4;
            color: white;
            border-radius: 4px;
            padding: 5px 20px;
            height: 30px;
            font-weight: bold;
        }
        QMessageBox QPushButton:hover {
            background-color: #005A9E;
        }
    )";
    this->setStyleSheet(qss);
}

void LoginWindow::onBtnGoRegisterClicked()
{
    // 打开注册界面
    RegisterWindow regWin(this);

    // 监听注册成功的信号
    connect(&regWin, &RegisterWindow::registerSuccess, this, &LoginWindow::onRegisterSuccess);

    regWin.exec();
}

void LoginWindow::onRegisterSuccess(const QString& username)
{
    // 自动回填注册账号，并把光标焦点直接给到密码框
    editUsername->setText(username);
    editPassword->clear();
    editPassword->setFocus();
}

void LoginWindow::onBtnLoginClicked()
{
    QString username = editUsername->text().trimmed();
    QString password = editPassword->text();

    // 🛡️ 防线：非空校验
    if (username.isEmpty() || password.isEmpty()) {
        QMessageBox::warning(this, "提示", "账号和密码不能为空！");
        return;
    }

    // 按钮防抖保护
    btnLogin->setEnabled(false);
    btnLogin->setText("登录中...");

    QJsonObject reqData;
    reqData["username"] = username;
    reqData["password"] = password;

    // 发起登录请求 (注意这里使用的是你配置的 9000 端口网关)
    HttpManager::instance()->postJson("http://10.196.229.92:9000/im-auth/auth/login", reqData,
                                      [=](QJsonObject res) {
                                          int code = res["code"].toInt();
                                          if (code == 200) {
                                              // 由于后端返回了VO对象，需要从 data 里拆解成 QJsonObject
                                              QJsonObject dataObj = res["data"].toObject();

                                              QString token = dataObj["token"].toString();
                                              QString avatarUrl = dataObj["avatar"].toString();
                                              QString nickname = dataObj["nickname"].toString();

                                              qDebug() << "登录成功！";
                                              qDebug() << "获取到全局 Token:" << token;
                                              qDebug() << "获取到当前用户真实头像:" << avatarUrl;
                                              qDebug() << "获取到当前用户真实头像:" << nickname;

                                              // 存入单例持久化
                                              HttpManager::instance()->setToken(token);
                                              HttpManager::instance()->setMyAvatarUrl(avatarUrl); // 锁死当前用户头像
                                              HttpManager::instance()->setMyNickname(nickname);  // 锁死当前用户昵称

                                              this->accept();
                                          } else {
                                              // 账号密码错误等业务报错
                                              QMessageBox::warning(this, "登录失败", res["message"].toString());
                                              btnLogin->setEnabled(true);
                                              btnLogin->setText("登 录");
                                          }
                                      },
                                      [=](QString err) {
                                          // 网络层崩溃兜底
                                          QMessageBox::critical(this, "网络错误", "连接服务器失败：" + err);
                                          btnLogin->setEnabled(true);
                                          btnLogin->setText("登 录");
                                      }
                                      );
}
