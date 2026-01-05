#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);
}

MainWindow::~MainWindow()
{
    delete ui;
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

