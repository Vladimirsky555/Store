#ifndef CATALOGUE_H
#define CATALOGUE_H

#include <QAbstractItemModel>
#include <QHeaderView>

#include <QTableView>
#include <QTreeView>
#include <QColumnView>

#include "data.h"
#include "dialog.h"

class QAction;
class PosAction;

/************************************************/
//Древовидная модель
/************************************************/

class Model : public QAbstractItemModel {

    Q_OBJECT

    //Список указателей на элементы каталога
    List catalog;
    //Переменная для того чтобы при создании экземпляра данных помечать новые данные как новые
    //mubable внутренняя переменная, которая может изменяться внутри константных функций
    mutable int LastTempId;

public:
    Model(QObject *parent = 0);
    virtual ~Model();

    Data *at(int k);
    void addItem(Data *D);

    // QAbstractItemModel interface
public:
    int      rowCount(const QModelIndex &parent) const;
    int      columnCount(const QModelIndex &) const;
    QVariant data(const QModelIndex &index, int role) const;
    QVariant headerData(int section, Qt::Orientation, int role) const;

    //Функции древовидной модели
    QModelIndex index(int row, int column, const QModelIndex &parent) const;
    QModelIndex parent(const QModelIndex &I) const;

    //Вспомогательные функции
protected:
    virtual QVariant dataDisplay(const QModelIndex &index) const;
    virtual QVariant dataTextAlignment(const QModelIndex &index) const;
    virtual QVariant dataForeground(const QModelIndex &index) const;
    virtual QVariant dataFont(const QModelIndex &index) const;
    virtual QVariant dataToolTip(const QModelIndex &I) const;
    virtual QVariant dataBackground(const QModelIndex &I) const;
    //Эта функция была нужна для линейной модели. Сейчас она не нужна,
    //так как мы пользуемся функцией internalPointer класса QModelIndex
    //для иерархических моделей
    //virtual Data    *dataDataBlock(const QModelIndex &I)const;
    int tempId() const {return ++LastTempId;}

    //Вспомогательные функции для сохранения в базу
private:
    //Удаление из базы, а потом модели
    bool delete_all_from_db(Data *D = 0);
    bool delete_all_from_model(Data *D = 0);

    //Сохранение изменений в базу и удаление пометок из модели
    bool save_all_to_db(Data *D = 0);//сохранить в базу
    bool drop_changed_mark(Data *D = 0);//сбросить пометку об изменении

    //Добавление новых элементов в базу данных
    bool insert_all_to_db(Data *D = 0);//сохранение новых элементов в базу
    bool ajust_id_for_new(Data *D = 0);//выставляет правильный id новым элементам

protected:
    bool delete_all();//блок удаления из базы, а потом из модели
    bool save_all();//блок сохранения изменений и удаления пометок
    bool insert_all();//блок сохранения в базу новых элементов

public slots:
    //Мы добавляем в модель редактирование и добавление
    //так как добавлять и редактировать возможно будем с разных представлений. Это нас оправдывает
    //Считается, что добавление в модель и редактирование в модели не лучшая идея
    //Параметры: элемент редактирования и виджет на каком редактировать
    void editItem(const QModelIndex &I, QWidget *parent = 0);
    void newItem(const QModelIndex &parentI, QWidget *parent = 0);
    void delItem(const QModelIndex &I, QWidget *parent = 0);
    void save();//сохранение данных в базу данных
};


/************************************************************/
//Табличное представление
/************************************************************/

class TableView : public QTableView {

    Q_OBJECT

public:
    PosAction *actEditItem;
    PosAction *actNewItem;
    PosAction *actDeleteItem;

    PosAction *actRootItem;//Шаг на ступеньку вниз
    QAction   *actParentRootItem;//Шаг на ступеньку вверх
    QAction   *actSave;//Сохранение данных в базу

public:
    TableView(QWidget *parent = 0, Model *xModel = 0);
    virtual ~TableView(){}

private slots:
    void contextMenuRequsted(const QPoint &p);

    //Слоты для просмотра иерархической модели
    void showChildren(const QModelIndex &I, QWidget*);
    void showParent();
};

/************************************************/
//Древовидное представление
/************************************************/

class TreeView : public QTreeView {

    Q_OBJECT

public:
    PosAction *actEditItem;
    PosAction *actNewItem;
    PosAction *actDeleteItem;
    PosAction *actRootItem;
    QAction   *actParentRootItem;
    QAction   *actSave;

public:
    TreeView(QWidget *parent = 0, Model *xModel = 0);
    virtual ~TreeView(){}

private slots:
    void contextMenuRequsted(const QPoint &p);
    void showChildren(const QModelIndex &I, QWidget*);
    void showParent();
};

/************************************************/
//Колоночное представление
/************************************************/

class ColumnView : public QColumnView {

    Q_OBJECT

public:
    PosAction *actEditItem;
    PosAction *actNewItem;
    PosAction *actDeleteItem;
    PosAction *actRootItem;
    QAction   *actParentRootItem;
    QAction   *actSave;

public:
    ColumnView(QWidget *parent = 0, Model *xModel = 0);
    virtual ~ColumnView(){}

private slots:
    void contextMenuRequsted(const QPoint &p);
    void showChildren(const QModelIndex &I, QWidget*);
    void showParent();

    // QAbstractItemView interface
    //Виртуальная функция, которая есть у всех моделей
    //вызывается тогда, когда меняется элемент один на другой
protected slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    //Сигнал при каждом выборе элемента
    void item_selected(QVariant id);
};

/************************************************/





#endif // CATALOGUE_H
