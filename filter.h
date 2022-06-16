#ifndef FILTER_H
#define FILTER_H

#include <QFrame>
#include <QVariant>

namespace Ui {
class Filter;
}

/*
 * Для подключения QFrame в припаркованное окно главного окна:
 * 1) Создать виджет, унаследованный от QFrame
 * 2) Убрать из деструктора delete *ui
 * 3) Заменить указатель *ui на объект ui
 * 4) Перенести строчку #include "ui_filter.h" в заголовочный файл
*/


class Filter : public QFrame
{
    Q_OBJECT

    //регистрируем свойства объекту
    Q_PROPERTY(QVariant author READ author)
    Q_PROPERTY(QVariant title  READ title)
    Q_PROPERTY(QVariant year   READ year)
    Q_PROPERTY(QVariant flag   READ flag)

public:
     Filter(QWidget *parent = 0);
     virtual ~Filter(){}

     QVariant author()const;
     QVariant title()const;
     QVariant year()const;
     QVariant flag() const;

private slots:
     void apply_filter_triggered();//передаёт указатель на объект, содержащий фильтр
     void clear_filter_triggered();

signals:
     void apply_filter(QObject *pFilter);

private:
    Ui::Filter *ui;
};

#endif


