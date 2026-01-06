#include "mainwindow.h"

#include <QApplication>
#include <QLocale>
#include <QTranslator>
#include <QDebug>
#include <QMessageBox>
#include <QDir>
#include <QPluginLoader>
#include <QSqlDatabase>

int main(int argc, char *argv[])
{
    qDebug() << "========================================";
    qDebug() << "程序启动...";
    qDebug() << "当前工作目录:" << QDir::currentPath();

    // 1. 首先检查 SQLite 驱动是否可用
    qDebug() << "检查可用数据库驱动:";
    QStringList drivers = QSqlDatabase::drivers();
    for (const QString &driver : drivers) {
        qDebug() << "  驱动:" << driver;
    }

    if (!drivers.contains("QSQLITE")) {
        qDebug() << "错误: QSQLITE 驱动不可用!";

        // 尝试加载插件
        QString pluginPath = QDir::currentPath() + "/sqldrivers";
        qDebug() << "尝试从路径加载插件:" << pluginPath;

        QPluginLoader loader(pluginPath + "/qsqlite.dll");
        if (loader.load()) {
            qDebug() << "成功加载 SQLite 插件";
        } else {
            qDebug() << "加载插件失败:" << loader.errorString();
        }
    }

    try {
        QApplication a(argc, argv);
        qDebug() << "QApplication 创建成功";

        // 2. 添加插件路径
        QStringList pluginPaths;
        pluginPaths << QDir::currentPath() + "/plugins";
        pluginPaths << QDir::currentPath() + "/sqldrivers";
        pluginPaths << "D:/Qt/6.9.2/mingw_64/plugins";
        pluginPaths << "D:/Qt/6.9.2/mingw_64/plugins/sqldrivers";

        for (const QString &path : pluginPaths) {
            if (QDir(path).exists()) {
                QApplication::addLibraryPath(path);
                qDebug() << "添加插件路径:" << path;
            }
        }

        // 3. 再次检查驱动
        qDebug() << "重新检查数据库驱动:";
        drivers = QSqlDatabase::drivers();
        for (const QString &driver : drivers) {
            qDebug() << "  驱动:" << driver;
        }

        if (!drivers.contains("QSQLITE")) {
            qDebug() << "严重错误: 仍然没有 QSQLITE 驱动!";
            QMessageBox::critical(nullptr, "数据库错误",
                                  "无法加载 SQLite 数据库驱动。\n"
                                  "请检查 Qt 安装是否完整。\n"
                                  "需要 sqldrivers 插件。");
            return -1;
        }

        qDebug() << "创建 MainWindow 对象...";
        MainWindow w;
        qDebug() << "MainWindow 创建完成";

        qDebug() << "显示主窗口...";
        w.show();
        qDebug() << "show() 调用完成";

        qDebug() << "进入事件循环...";
        return a.exec();

    } catch (const std::exception& e) {
        qDebug() << "程序异常:" << e.what();
        QMessageBox::critical(nullptr, "程序异常", QString("程序启动异常:\n%1").arg(e.what()));
        return -1;
    } catch (...) {
        qDebug() << "未知异常";
        QMessageBox::critical(nullptr, "程序异常", "程序启动时发生未知异常");
        return -1;
    }
}
