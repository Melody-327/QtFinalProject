#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "taskeditdialog.h"
#include <QInputDialog>
#include <QFileDialog>
#include <QTextStream>
#include <QDir>
#include <QDebug>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("任务管理系统");

    // 初始化数据库
    initDatabase();
    if (openDatabase()) {
        createTables();
        initFilterComboBox(); // 初始化筛选下拉框
        loadTaskData();       // 加载初始任务数据
    } else {
        QMessageBox::critical(this, "数据库错误", "连接FinalLab.db失败！\n" + db.lastError().text());
    }
}

MainWindow::~MainWindow()
{
    closeDatabase();
    delete ui;
}

// 初始化数据库连接
void MainWindow::initDatabase()
{
    // 设置数据库文件路径
    QString dbPath = "D:/Qt/FinalLab.db";  // 首选路径
    QFileInfo dbFile(dbPath);

    // 如果首选路径不存在，使用当前目录
    if (!dbFile.exists()) {
        dbPath = QDir::currentPath() + "/FinalLab.db";
        dbFile.setFile(dbPath);
    }

    qDebug() << "[MainWindow] 数据库路径:" << dbFile.absoluteFilePath();

    // 创建唯一的数据库连接名称（避免冲突）
    static int connectionCount = 0;
    QString connectionName = QString("task_management_connection_%1").arg(++connectionCount);

    // 移除可能存在的同名连接
    if (QSqlDatabase::contains(connectionName)) {
        QSqlDatabase::removeDatabase(connectionName);
    }

    // 添加SQLite数据库连接
    db = QSqlDatabase::addDatabase("QSQLITE", connectionName);
    db.setDatabaseName(dbFile.absoluteFilePath());  // 设置数据库文件路径
}

// 打开数据库连接
bool MainWindow::openDatabase()
{
    if (db.open()) {
        qDebug() << "[MainWindow] 数据库打开成功";
        return true;
    } else {
        qDebug() << "[MainWindow] 数据库打开失败:" << db.lastError().text();
        return false;
    }
}

// 关闭数据库连接
void MainWindow::closeDatabase()
{
    if (db.isOpen()) {
        db.close();
    }
}

// 创建数据表（如果不存在）
void MainWindow::createTables()
{
    QSqlQuery query(db);

    // 创建分类表
    QString createCategorySql = "CREATE TABLE IF NOT EXISTS category ("
                                "id INTEGER PRIMARY KEY AUTOINCREMENT,"  // 自增主键
                                "name TEXT NOT NULL UNIQUE)";  // 分类名称，唯一约束
    query.exec(createCategorySql);

    // 创建任务表
    QString createTaskSql = "CREATE TABLE IF NOT EXISTS task ("
                            "id INTEGER PRIMARY KEY AUTOINCREMENT,"      // 自增主键
                            "name TEXT NOT NULL,"                        // 任务名称
                            "desc TEXT,"                                 // 任务描述
                            "priority TEXT NOT NULL,"                    // 优先级
                            "deadline TEXT,"                             // 截止日期
                            "status TEXT NOT NULL,"                      // 状态
                            "category_id INTEGER)";                      // 分类ID（外键）
    query.exec(createTaskSql);

    // 插入默认分类数据
    QStringList defaultCategories = {"工作", "生活", "学习", "健康", "社交"};
    for (const QString &category : defaultCategories) {
        // 使用INSERT OR IGNORE避免重复插入
        query.exec(QString("INSERT OR IGNORE INTO category (name) VALUES ('%1')").arg(category));
    }
}

// 检查数据库连接详情
void MainWindow::checkDatabaseConnection()
{
    if (!db.isOpen()) return;

    QSqlQuery checkQuery(db);
    // 使用PRAGMA命令获取数据库信息
    if (checkQuery.exec("PRAGMA database_list")) {
        qDebug() << "[MainWindow] 数据库连接详情:";
        while (checkQuery.next()) {
            QString file = checkQuery.value(2).toString();  // 获取数据库文件路径
            qDebug() << "  文件:" << file;
        }
    }
}

// 初始化过滤器下拉框
void MainWindow::initFilterComboBox()
{
    // 初始化优先级过滤器
    ui->comboPriorityFilter->clear();
    ui->comboPriorityFilter->addItems({"全部", "高", "中", "低"});

    // 初始化状态过滤器
    ui->comboStatusFilter->clear();
    ui->comboStatusFilter->addItems({"全部", "未开始", "进行中", "已完成"});
}

// 加载任务数据
void MainWindow::loadTaskData()
{
    if (!m_taskModel || !db.isOpen()) return;

    // 获取当前过滤条件
    QString priorityFilter = ui->comboPriorityFilter->currentText();
    QString statusFilter = ui->comboStatusFilter->currentText();

    QString filterStr;  // 过滤条件字符串

    // 构建优先级过滤条件
    if (priorityFilter != "全部" && !priorityFilter.isEmpty()) {
        filterStr = QString("priority = '%1'").arg(priorityFilter);
    }

    // 构建状态过滤条件
    if (statusFilter != "全部" && !statusFilter.isEmpty()) {
        if (!filterStr.isEmpty()) filterStr += " AND ";
        filterStr += QString("status = '%1'").arg(statusFilter);
    }

    // 应用过滤条件到模型
    if (!filterStr.isEmpty()) {
        m_taskModel->setFilter(filterStr);
    } else {
        m_taskModel->setFilter("");  // 清空过滤条件，显示所有数据
    }

    m_taskModel->select();  // 从数据库重新加载数据
    updateTaskStats();      // 更新统计信息
}

// 获取当前选中任务的ID
int MainWindow::getCurrentTaskId()
{
    QModelIndex currentIndex = ui->tableViewTask->currentIndex();
    if (!currentIndex.isValid()) return -1;  // 没有选中行

    // 将代理模型索引转换为源模型索引
    QModelIndex sourceIndex = m_proxyModel->mapToSource(currentIndex);
    int idCol = m_taskModel->fieldIndex("id");  // 获取ID列的索引
    return m_taskModel->data(m_taskModel->index(sourceIndex.row(), idCol)).toInt();  // 返回ID值
}

// 更新任务统计信息
void MainWindow::updateTaskStats()
{
    if (!db.isOpen()) return;

    // 获取当前过滤条件
    QString priorityFilter = ui->comboPriorityFilter->currentText();
    QString statusFilter = ui->comboStatusFilter->currentText();

    // 构建基础WHERE子句
    QString baseWhereClause = "";
    if (priorityFilter != "全部" && !priorityFilter.isEmpty()) {
        baseWhereClause = QString("priority='%1'").arg(priorityFilter);
    }
    if (statusFilter != "全部" && !statusFilter.isEmpty()) {
        if (!baseWhereClause.isEmpty()) baseWhereClause += " AND ";
        baseWhereClause += QString("status='%1'").arg(statusFilter);
    }

    // 查询总任务数
    QString totalQueryStr = "SELECT COUNT(*) FROM task";
    if (!baseWhereClause.isEmpty()) totalQueryStr += " WHERE " + baseWhereClause;

    QSqlQuery totalQuery(db);
    int totalTasks = 0;
    if (totalQuery.exec(totalQueryStr) && totalQuery.next()) {
        totalTasks = totalQuery.value(0).toInt();
    }

    // 查询已完成任务数
    QString completedQueryStr = "SELECT COUNT(*) FROM task WHERE status='已完成'";
    if (!baseWhereClause.isEmpty()) {
        completedQueryStr = "SELECT COUNT(*) FROM task WHERE " + baseWhereClause + " AND status='已完成'";
    }

    QSqlQuery completedQuery(db);
    int completedTasks = 0;
    if (completedQuery.exec(completedQueryStr) && completedQuery.next()) {
        completedTasks = completedQuery.value(0).toInt();
    }

    // 查询进行中任务数
    QString inProgressQueryStr = "SELECT COUNT(*) FROM task WHERE status='进行中'";
    if (!baseWhereClause.isEmpty()) {
        inProgressQueryStr = "SELECT COUNT(*) FROM task WHERE " + baseWhereClause + " AND status='进行中'";
    }

    QSqlQuery inProgressQuery(db);
    int inProgressTasks = 0;
    if (inProgressQuery.exec(inProgressQueryStr) && inProgressQuery.next()) {
        inProgressTasks = inProgressQuery.value(0).toInt();
    }

    // 查询未开始任务数
    QString pendingQueryStr = "SELECT COUNT(*) FROM task WHERE status='未开始'";
    if (!baseWhereClause.isEmpty()) {
        pendingQueryStr = "SELECT COUNT(*) FROM task WHERE " + baseWhereClause + " AND status='未开始'";
    }

    QSqlQuery pendingQuery(db);
    int pendingTasks = 0;
    if (pendingQuery.exec(pendingQueryStr) && pendingQuery.next()) {
        pendingTasks = pendingQuery.value(0).toInt();
    }

    // 格式化统计信息文本
    QString statsText = QString("总计: %1 | 已完成: %2 | 进行中: %3 | 未开始: %4")
                            .arg(totalTasks)
                            .arg(completedTasks)
                            .arg(inProgressTasks)
                            .arg(pendingTasks);

    // 更新状态标签显示
    if (ui->labelStatusInfo) {
        ui->labelStatusInfo->setText(statsText);
    }
}

// 获取特定状态和优先级的任务数量（辅助函数）
int MainWindow::getPriorityCount(const QString& status, const QString& priority)
{
    QString queryStr = QString("SELECT COUNT(*) FROM task WHERE status='%1' AND priority='%2'")
    .arg(status).arg(priority);
    QSqlQuery query(db);
    if (query.exec(queryStr) && query.next()) {
        return query.value(0).toInt();
    }
    return 0;
}

// 添加任务按钮点击事件
void MainWindow::on_btnAddTask_clicked()
{
    // 创建任务编辑对话框（taskId=-1表示添加模式）
    TaskEditDialog dlg(this, -1, db);
    dlg.setWindowTitle("添加任务");
    if (dlg.exec() == QDialog::Accepted) {  // 显示模态对话框
        loadTaskData();  // 重新加载数据
        QMessageBox::information(this, "成功", "任务添加成功！");
    }
}

// 编辑任务按钮点击事件
void MainWindow::on_btnEditTask_clicked()
{
    int taskId = getCurrentTaskId();
    if (taskId < 0) {
        QMessageBox::warning(this, "提示", "请先选中要编辑的任务！");
        return;
    }

    // 创建任务编辑对话框（taskId>0表示编辑模式）
    TaskEditDialog dlg(this, taskId, db);
    dlg.setWindowTitle("编辑任务");
    if (dlg.exec() == QDialog::Accepted) {
        loadTaskData();  // 重新加载数据
        QMessageBox::information(this, "成功", "任务修改成功！");
    }
}

// 删除任务按钮点击事件
void MainWindow::on_btnDeleteTask_clicked()
{
    int taskId = getCurrentTaskId();
    if (taskId < 0) {
        QMessageBox::warning(this, "提示", "请先选中要删除的任务！");
        return;
    }

    // 确认删除对话框
    if (QMessageBox::question(this, "确认", "确定删除该任务吗？",
                              QMessageBox::Yes|QMessageBox::No) != QMessageBox::Yes) {
        return;
    }

    // 执行删除操作
    QSqlQuery query(db);
    query.prepare("DELETE FROM task WHERE id = :id");  // 使用参数化查询防止SQL注入
    query.bindValue(":id", taskId);

    if (query.exec()) {
        loadTaskData();  // 重新加载数据
        QMessageBox::information(this, "成功", "任务删除成功！");
    } else {
        QMessageBox::critical(this, "失败", "删除失败：" + query.lastError().text());
    }
}

// 刷新按钮点击事件
void MainWindow::on_btnRefresh_clicked()
{
    loadTaskData();  // 重新加载数据
    QMessageBox::information(this, "提示", "数据已刷新！");
}

void MainWindow::on_btnExport_clicked()
{

}
