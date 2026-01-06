#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QSqlDatabase>
#include <QSqlQuery>
#include <QSqlError>
#include <QMessageBox>
#include <QSqlTableModel>
#include <QSortFilterProxyModel>
#include <QHeaderView>
#include <QLabel>
#include <QDebug>
#include <QMenu>
#include <QPrinter>      // PDF导出
#include <QPainter>      // PDF导出
#include <QFileDialog>   // 文件对话框
#include <QTextStream>   // 文本流
#include <QDate>         // 日期
#include <QDateTime>     // 日期时间
#include "taskeditdialog.h"
#include "remindermanager.h"

// Qt6 兼容性处理
#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
#include <QStringConverter>  // Qt6 的编码转换
#include <QPageSize>        // Qt6 页面尺寸
#include <QPageLayout>      // Qt6 页面布局
#endif

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
    void on_btnAddTask_clicked();
    void on_btnEditTask_clicked();
    void on_btnDeleteTask_clicked();
    void on_btnRefresh_clicked();
    void on_btnExport_clicked();

    void on_comboPriorityFilter_currentIndexChanged(int index);
    void on_comboStatusFilter_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
    QSqlDatabase db;
    QSqlTableModel *m_taskModel;
    QSortFilterProxyModel *m_proxyModel;
    ReminderManager *m_reminderManager;

    // 数据库操作
    void initDatabase();
    bool openDatabase();
    void closeDatabase();
    void createTables();

    // 导出功能
    void exportToCsv();
    void exportToPdf();

    void checkDatabaseConnection();

    // 数据加载/初始化
    void initFilterComboBox();
    void loadTaskData();
    int getCurrentTaskId();
    void updateTaskStats();

    // 辅助函数
    int getPriorityCount(const QString& status, const QString& priority);
};
#endif // MAINWINDOW_H
