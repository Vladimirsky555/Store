#include "filter.h"
#include "ui_filter.h"


Filter::Filter(QWidget *parent) :
    QFrame(parent),
    ui(new Ui::Filter)
{
    ui->setupUi(this);

    ui->btnFilter->setDefaultAction(ui->actionApplyFilter);
    ui->btnClear->setDefaultAction(ui->actionClearFilter);

    connect(ui->actionApplyFilter, SIGNAL(triggered()),
            this, SLOT(apply_filter_triggered()));

    connect(ui->actionClearFilter, SIGNAL(triggered()),
            this, SLOT(clear_filter_triggered()));
}

QVariant Filter::author() const
{
    QString T = ui->edtAuthor->text().simplified();
    return T.isEmpty() ? QVariant() : T;
}

QVariant Filter::title() const
{
    QString T = ui->edtTitle->text().simplified();
    return T.isEmpty() ? QVariant() : T;
}

QVariant Filter::year() const
{
    QString T = ui->edtYear->text().simplified();
    if(T.isEmpty()) return QVariant();
    bool OK;
    int Result = T.toInt(&OK);
    return OK ? Result : QVariant();
}

QVariant Filter::flag() const
{
    return ui->cbxAll->isChecked();
}

void Filter::apply_filter_triggered()
{
    emit apply_filter(this);
}

void Filter::clear_filter_triggered()
{
    ui->edtAuthor->clear();
    ui->edtTitle->clear();
    ui->edtYear->clear();
}



