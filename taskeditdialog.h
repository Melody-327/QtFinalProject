#ifndef TASKEDITDIALOG_H
#define TASKEDITDIALOG_H

#include <QDialog>
#include <QSqlDatabase>  // 添加这行
#include <QSqlQuery>

namespace Ui { class TaskEditDialog; }

class TaskEditDialog : public QDialog
{
    Q_OBJECT

public:
    // 构造函数：添加数据库连接参数
    explicit TaskEditDialog(QWidget *parent = nullptr, int taskId = -1,
                            const QSqlDatabase &parentDb = QSqlDatabase());
    ~TaskEditDialog();

private slots:
    void on_btnConfirm_clicked(); // 确认按钮
    void on_btnCancel_clicked();  // 取消按钮

private:
    Ui::TaskEditDialog *ui;
    int m_taskId; // 编辑模式下的任务ID（-1为添加模式）
    QSqlDatabase m_db;  // 添加数据库成员变量

    // 初始化控件
    void initComboBox();
    // 加载分类下拉框
    void loadCategoryData();
    // 加载任务数据（编辑模式）
    void loadTaskData(int taskId);
    // 保存任务数据
    bool saveTaskData();
};
#endif // TASKEDITDIALOG_H
