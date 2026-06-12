#include "registerwindow.h"
#include "httpmanager.h"
#include <QMessageBox>
#include <QJsonObject>
#include <QRegularExpression>
#include <QDebug>
#include <QFileDialog>
#include <QPainter>
#include <QPainterPath>

// =========================================================================
// 🎨 Qt 绘图魔法：将任意方形 QPixmap 切割为带抗锯齿的高清圆形头像
// =========================================================================
static QPixmap makeCircularAvatar(const QPixmap &src, int size)
{
    if (src.isNull()) return QPixmap();
    QPixmap scaled = src.scaled(size, size, Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    QPixmap result(size, size);
    result.fill(Qt::transparent);
    QPainter painter(&result);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    QPainterPath path;
    path.addEllipse(0, 0, size, size);
    painter.setClipPath(path);
    painter.drawPixmap(0, 0, size, size, scaled);
    return result;
}

RegisterWindow::RegisterWindow(QWidget *parent) :
    QDialog(parent)
{
    // 3. 预设 MinIO 默认头像 (放前面以便 initUI 能立刻使用)
    currentAvatarUrl = "http://10.196.229.92:19000/im-chat/avatar.png";

    // 1. 初始化界面与样式
    initUI();
    initStyle();

    // 2. 窗口基础设置
    this->setWindowTitle("注册新账号");
    this->setFixedSize(400, 600); // 高度拉长到 600，给头像预留优美的排版空间
}

RegisterWindow::~RegisterWindow()
{
}

void RegisterWindow::initUI()
{
    // --- 1. 创建控件 ---
    lblTitle = new QLabel("Create Account", this);
    lblTitle->setAlignment(Qt::AlignCenter);
    lblTitle->setObjectName("lblTitle");

    // 🌟 1. 初始化头像按钮 (80x80 大尺寸)
    btnAvatar = new QPushButton(this);
    btnAvatar->setFixedSize(80, 80);
    btnAvatar->setObjectName("btnAvatarUpload");
    btnAvatar->setCursor(Qt::PointingHandCursor);
    btnAvatar->setToolTip("点击上传自定义头像");

    // 预先加载并渲染成功的默认圆形头像
    HttpManager::instance()->downloadImage(currentAvatarUrl, [=](QPixmap originalImage) {
        btnAvatar->setIcon(QIcon(makeCircularAvatar(originalImage, 80)));
        btnAvatar->setIconSize(QSize(80, 80));
    }, [=](QString err) {
                                               btnAvatar->setText("上传\n头像");
                                           });

    editUsername = new QLineEdit(this);
    editUsername->setPlaceholderText("请输入账号 (4-16位字母或数字)");

    editNickname = new QLineEdit(this);
    editNickname->setPlaceholderText("请输入昵称 (选填)");

    editPassword = new QLineEdit(this);
    editPassword->setPlaceholderText("请输入密码 (至少5位)");
    editPassword->setEchoMode(QLineEdit::Password);

    editConfirmPassword = new QLineEdit(this);
    editConfirmPassword->setPlaceholderText("请确认密码");
    editConfirmPassword->setEchoMode(QLineEdit::Password);

    // 性别组
    radioMale = new QRadioButton("👨 男", this);
    radioFemale = new QRadioButton("👩 女", this);
    radioSecret = new QRadioButton("🔒 保密", this);
    radioSecret->setChecked(true);

    genderGroup = new QButtonGroup(this);
    genderGroup->addButton(radioSecret, 0);
    genderGroup->addButton(radioMale, 1);
    genderGroup->addButton(radioFemale, 2);

    btnRegister = new QPushButton("立即注册", this);
    btnRegister->setObjectName("btnPrimary");

    btnCancel = new QPushButton("返回登录", this);
    btnCancel->setObjectName("btnSecondary");

    // --- 2. 排版布局 (Layout) ---
    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(40, 30, 40, 30);
    mainLayout->setSpacing(15);

    mainLayout->addWidget(lblTitle);

    // 🌟 2. 将头像按钮加入布局 (通过两边加弹簧实现绝对居中)
    QHBoxLayout *avatarLayout = new QHBoxLayout();
    avatarLayout->addStretch();
    avatarLayout->addWidget(btnAvatar);
    avatarLayout->addStretch();
    mainLayout->addLayout(avatarLayout);
    mainLayout->addSpacing(5);

    mainLayout->addWidget(editUsername);
    mainLayout->addWidget(editNickname);
    mainLayout->addWidget(editPassword);
    mainLayout->addWidget(editConfirmPassword);

    // 性别水平布局
    QHBoxLayout *genderLayout = new QHBoxLayout();
    genderLayout->addWidget(new QLabel("性别:", this));
    genderLayout->addWidget(radioMale);
    genderLayout->addWidget(radioFemale);
    genderLayout->addWidget(radioSecret);
    genderLayout->addStretch();
    mainLayout->addLayout(genderLayout);

    mainLayout->addSpacing(10);
    mainLayout->addWidget(btnRegister);
    mainLayout->addWidget(btnCancel);
    mainLayout->addStretch();

    // --- 3. 信号与槽绑定 ---
    connect(btnRegister, &QPushButton::clicked, this, &RegisterWindow::onBtnSubmitRegisterClicked);
    connect(btnCancel, &QPushButton::clicked, this, &RegisterWindow::onBtnCancelClicked);
    connect(btnAvatar, &QPushButton::clicked, this, &RegisterWindow::onBtnAvatarClicked); // 🌟 绑定头像点击
}

void RegisterWindow::initStyle()
{
    QString qss = R"(
        QDialog {
            background-color: #FFFFFF;
        }
        #lblTitle {
            font-size: 28px;
            font-weight: bold;
            color: #333333;
            font-family: "Segoe UI", "Microsoft YaHei";
        }
        #btnAvatarUpload {
            background-color: #F8F9FA;
            border: 1px dashed #CCCCCC;
            border-radius: 40px;
            color: #888888;
        }
        #btnAvatarUpload:hover {
            border: 1px solid #0078D4;
            color: #0078D4;
        }
        QLineEdit {
            height: 40px;
            border: 1px solid #E0E0E0;
            border-radius: 6px;
            padding-left: 15px;
            font-size: 14px;
            background-color: #F8F9FA;
            color: #333333;
        }
        QLineEdit:focus {
            border: 2px solid #0078D4;
            background-color: #FFFFFF;
        }
        #btnPrimary {
            height: 45px;
            background-color: #0078D4;
            color: white;
            border-radius: 6px;
            font-size: 16px;
            font-weight: bold;
        }
        #btnPrimary:hover {
            background-color: #005A9E;
        }
        #btnPrimary:pressed {
            background-color: #004578;
        }
        #btnSecondary {
            height: 45px;
            background-color: transparent;
            color: #666666;
            border: none;
            font-size: 14px;
        }
        #btnSecondary:hover {
            color: #0078D4;
            text-decoration: underline;
        }
        QRadioButton {
            font-size: 14px;
            color: #555555;
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

// 🌟 实现终极武器：头像异步上传与原位切割渲染
void RegisterWindow::onBtnAvatarClicked()
{
    QString filePath = QFileDialog::getOpenFileName(this, "选择炫酷头像", "", "Images (*.png *.jpg *.jpeg)");
    if (filePath.isEmpty()) {
        return;
    }

    btnAvatar->setIcon(QIcon());
    btnAvatar->setText("上传中...");

    // 调用文件网关接口 (使用 127.0.0.1 避坑)
    QString uploadUrl = "http://10.196.229.92:9000/im-server/file/upload";

    HttpManager::instance()->uploadFile(uploadUrl, filePath, [=](QJsonObject res) {
        int code = res["code"].toInt();
        if (code == 200) {
            currentAvatarUrl = res["data"].toString();
            qDebug() << "自定义头像上云成功:" << currentAvatarUrl;

            // 替换成安全的 IP 防止 Qt 拉取挂起
            QString safeUrl = currentAvatarUrl;
            safeUrl.replace("localhost", "127.0.0.1");

            // 在本界面直接拉取并切成圆形预览！
            HttpManager::instance()->downloadImage(safeUrl, [=](QPixmap newAvatar) {
                btnAvatar->setText("");
                btnAvatar->setIcon(QIcon(makeCircularAvatar(newAvatar, 80)));
                btnAvatar->setIconSize(QSize(80, 80));
            }, [=](QString err){ qDebug() << "预览拉取失败:" << err; });

        } else {
            QMessageBox::warning(this, "上传失败", res["msg"].toString());
            btnAvatar->setText("上传\n失败");
        }
    }, [=](QString err) {
                                            QMessageBox::critical(this, "网络错误", "文件上传失败：" + err);
                                            btnAvatar->setText("上传\n失败");
                                        });
}

void RegisterWindow::onBtnSubmitRegisterClicked()
{
    QString username = editUsername->text().trimmed();
    QString nickname = editNickname->text().trimmed();
    QString password = editPassword->text();
    QString confirmPassword = editConfirmPassword->text();
    int sex = genderGroup->checkedId();

    QRegularExpression usernameRegex("^[a-zA-Z0-9]{4,16}$");
    if (!usernameRegex.match(username).hasMatch()) {
        QMessageBox::warning(this, "格式错误", "账号必须是 4-16 位的字母或数字组合！");
        editUsername->setFocus();
        return;
    }

    if (password.length() < 5) {
        QMessageBox::warning(this, "格式错误", "为了您的安全，密码至少需要 5 位！");
        editPassword->setFocus();
        return;
    }

    if (password != confirmPassword) {
        QMessageBox::warning(this, "校验失败", "两次输入的密码不一致！");
        editConfirmPassword->clear();
        editConfirmPassword->setFocus();
        return;
    }

    btnRegister->setEnabled(false);
    btnRegister->setText("注册中...");

    QJsonObject reqData;
    reqData["username"] = username;
    reqData["nickname"] = nickname;
    reqData["password"] = password;
    reqData["sex"] = sex;
    reqData["avatar"] = currentAvatarUrl;

    // 使用 127.0.0.1 避开 IPv6 解析坑
    HttpManager::instance()->postJson("http://10.196.229.92:9000/im-auth/auth/register", reqData,
                                      [=](QJsonObject res) {
                                          int code = res["code"].toInt();
                                          if (code == 200) {
                                              QMessageBox::information(this, "欢迎", "账号注册成功，即将为您跳转登录！");
                                              emit registerSuccess(username);
                                              this->accept();
                                          } else {
                                              QMessageBox::warning(this, "注册失败", res["message"].toString());
                                              btnRegister->setEnabled(true);
                                              btnRegister->setText("立即注册");
                                          }
                                      },
                                      [=](QString err) {
                                          QMessageBox::critical(this, "网络超时", "连接服务器失败：" + err);
                                          btnRegister->setEnabled(true);
                                          btnRegister->setText("立即注册");
                                      }
                                      );
}

void RegisterWindow::onBtnCancelClicked()
{
    this->reject();
}
