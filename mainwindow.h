#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include "taskeditdialog.h"

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
    // 按钮事件
    void on_btnAddTask_clicked();
    void on_btnEditTask_clicked();
    void on_btnDeleteTask_clicked();
    void on_btnRefresh_clicked();
    // 筛选条件变化事件
    void on_comboPriorityFilter_currentIndexChanged(const QString &arg1);
    void on_comboStatusFilter_currentIndexChanged(const QString &arg1);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;

    // 数据库操作
    void initDatabase();
    bool openDatabase();
    void closeDatabase();
    void createTables();
    // 数据加载
    void initFilterComboBox(); // 初始化筛选下拉框
    void loadTaskData();       // 加载任务数据（支持筛选）
};
#endif // MAINWINDOW_H
