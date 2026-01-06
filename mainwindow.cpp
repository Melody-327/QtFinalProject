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

void MainWindow::on_btnAdd_clicked()
{

}


void MainWindow::on_btnEdit_clicked()
{

}


void MainWindow::on_btnDelete_clicked()
{

}


void MainWindow::on_btnRefresh_clicked()
{

}


void MainWindow::on_btnExport_clicked()
{

}
