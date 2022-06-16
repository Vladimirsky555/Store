#ifndef DIALOG_H
#define DIALOG_H

#include <QDialog>

#include "data.h"

namespace Ui {
class Dialog;
}

class Dialog : public QDialog
{
    Q_OBJECT

    Data *D;

public:
    Dialog(Data *D, QWidget *parent = 0);
    ~Dialog();

    void load();
    bool save();

signals:
    void error_message(const QString &);

private slots:
    void on_btnOK_clicked();
    void on_btnCancel_clicked();

private:
    Ui::Dialog *ui;
};


#endif // DIALOG_H
