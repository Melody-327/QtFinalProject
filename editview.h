#ifndef EDITVIEW_H
#define EDITVIEW_H

#include <QWidget>

namespace Ui {
class EditView;
}

class EditView : public QWidget
{
    Q_OBJECT

public:
    explicit EditView(QWidget *parent = nullptr);
    ~EditView();

private slots:
    void on_btnConfirm_clicked();

    void on_btnCancel_clicked();

private:
    Ui::EditView *ui;
};

#endif // EDITVIEW_H
