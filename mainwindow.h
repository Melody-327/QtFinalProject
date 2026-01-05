#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "editview.h"  // 包含编辑窗口头文件

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    void on_btnAdd_clicked();
    void on_btnEdit_clicked();
    void on_btnDelete_clicked();
    void on_btnRefresh_clicked();
    void on_btnExport_clicked();
    void refreshTable();  // 刷新表格数据

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;      // 数据库连接对象
    void initDatabase();  // 初始化数据库
    bool openDatabase();  // 打开数据库
    void closeDatabase(); // 关闭数据库
};
#endif // MAINWINDOW_H
