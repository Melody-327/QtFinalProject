#include "mainwindow.h"
#include "ui_mainwindow.h"
#include <QFile>
#include <QTextStream>
#include <QDateTime>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    initDatabase();
    if (openDatabase()) {
        refreshTable(); // 初始化时加载数据
    } else {
        QMessageBox::critical(this, "错误", "数据库连接失败！");
    }
}

MainWindow::~MainWindow()
{
    closeDatabase();
    delete ui;
}

// 初始化数据库
void MainWindow::initDatabase()
{
    // 假设数据库名为data.db，表名为info，包含id(主键)、name、age、address字段
    db = QSqlDatabase::addDatabase("QSQLITE");
    db.setDatabaseName("FinalLab.db");
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

