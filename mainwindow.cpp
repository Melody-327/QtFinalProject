#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "taskeditdialog.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

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
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("D:/Qt/FinalLab.db");
}

// 打开数据库
bool MainWindow::openDatabase()
{
    if (db.open()) {
        qDebug() << "数据库连接成功";
        return true;
    } else {
        qDebug() << "数据库连接失败：" << db.lastError().text();
        return false;
    }
}

// 关闭数据库
void MainWindow::closeDatabase()
{
    if (db.isOpen()) {
        db.close();
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

