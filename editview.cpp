#include "editview.h"
#include "ui_editview.h"

EditView::EditView(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::EditView)
{
    ui->setupUi(this);
}

EditView::~EditView()
{
    delete ui;
}

void EditView::on_btnConfirm_clicked()
{

}


void EditView::on_btnCancel_clicked()
{

}

