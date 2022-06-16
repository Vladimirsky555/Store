#ifndef POSACTION_H
#define POSACTION_H

#include <QAction>
#include <QModelIndex>


class PosAction : public QAction
{

    Q_OBJECT

public:
    QModelIndex I;
    QWidget *pWidget;

public:
    PosAction(QObject *parent = 0);
    virtual ~PosAction(){}

public slots:
    void was_triggered();

signals:
    //Перехватим сигнал triggerred и дополним его этим сигналом
    void actionOnItem(const QModelIndex &I, QWidget *parent);
};


#endif // POSACTION_H
