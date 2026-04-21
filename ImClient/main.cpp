#include "mainwindow.h"
#include "loginwindow.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    // 1. 先创建登录窗口
    LoginWindow loginDialog;

    // 2. exec() 会阻塞在这里，直到在 LoginWindow 里调用了 this->accept()
    if (loginDialog.exec() == QDialog::Accepted) {

        // 3. 登录成功
        MainWindow w;
        w.show();

        // 4. 启动 Qt 的主事件循环
        return a.exec();
    }
    return 0;
}
