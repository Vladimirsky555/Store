#include "dialog.h"
#include "ui_dialog.h"


Dialog::Dialog(Data *D, QWidget *parent) :
    QDialog(parent),
    ui(new Ui::Dialog)
{
    ui->setupUi(this);
    setWindowTitle("Добавление элемента");
    this->D = D;
    load();
}

Dialog::~Dialog()
{
    delete ui;
}

void Dialog::on_btnOK_clicked()
{
    save();
    close();
}

void Dialog::on_btnCancel_clicked()
{
    close();
}

void Dialog::load()
{
    ui->edtCode->setText(D->getCode());
    ui->edtComment->setText(D->getComment());
    ui->edtTitle->setText(D->getTitle());
    ui->edtFrom->setDateTime(D->getFrom());
    ui->edtTo->setDateTime(D->getTo());
    ui->cbxLocal->setChecked(D->Local());

}

bool Dialog::save()
{
    D->setCode(ui->edtCode->text().simplified());
    D->setTitle(ui->edtTitle->text().simplified());
    D->setComment(ui->edtComment->toPlainText().trimmed());
    D->setLocal(ui->cbxLocal->isChecked());

    if(ui->cbxFromEnable->isChecked()){
       D->setFrom(ui->edtFrom->dateTime());
    } else {
       D->setFrom(QDateTime());
    }

    if(ui->cbxToEnabled->isChecked()){
       D->setTo(ui->edtTo->dateTime());
    } else {
       D->setTo(QDateTime());
    }

    D->setChanged(true);//Блок был изменён, поэтому будет сохранён
    return true;
}

