#include "posaction.h"

PosAction::PosAction(QObject *parent) :
    QAction(parent)
{
    connect(this, SIGNAL(triggered()),
            this, SLOT(was_triggered()));
}

void PosAction::was_triggered()
{
    //Добавляем какой элемент редактируем и виджет,
    //на котором показываем диалог
    emit actionOnItem(I, pWidget);
}

