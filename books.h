#ifndef BOOKS_H
#define BOOKS_H

//модель считывается напрямую из базы
//QSqlQueryModel - хорошо подходит для просмотра данных,
//но плохо подходит для их редактирования
#include <QSqlQueryModel>
#include <QSqlTableModel>
#include <QTableView>
#include <QItemDelegate>

/************************************************/
//Класс делегат
/************************************************/
class StatusDelegate : public QItemDelegate {

    Q_OBJECT

    QMap<int, QString> allStatus;

public:
    StatusDelegate(QObject *parent, QMap<int, QString> allStatus);
    virtual ~StatusDelegate(){}


    // QAbstractItemDelegate interface
public:
    //создаёт редактор
    QWidget *createEditor(QWidget *parent,
                          const QStyleOptionViewItem &option,
                          const QModelIndex &index) const;

    //переносит данные с модели на редактор
    void setEditorData(QWidget *editor, const QModelIndex &I) const;

    //перенос данных с редактора в модель
    void setModelData(QWidget *editor, QAbstractItemModel *model,
                      const QModelIndex &I) const;

    //зона на которой рисоватать
    void paint(QPainter *painter,
               const QStyleOptionViewItem &option,
               const QModelIndex &index) const;
};
/************************************************/



/****************************************************************************************/
//Модель для отображения данных
/******************************************************************************************/
class BooksModel : public QSqlTableModel
{
    Q_OBJECT

    //Введём переменные для фильтра
    //Приватные переменные хранят параметры фильтра
    QVariant fAuthor;
    QVariant fTitle;
    QVariant fYear;
    QVariant fFlag;
    QVariant fCatId;

    QAction *actNewRow;
    QAction *actDeleteRow;
    QAction *actUpdateAll;
    QAction *actSaveAll;
    QAction *actTemp;

public:
    QList<QAction*> allActions;
    QMap<int, QString> allStatus;
    int currentId;//текущий индекс в модели
    bool filter;//переключатель по выборке из фильтра

public:
    BooksModel(QObject *parent = 0);
    virtual ~BooksModel(){}

    // слоты для реализации добавления, удаления и редактирования
protected slots:
    void on_new_row();
//    void on_save_one();
    void on_update_one();
    void on_delete_row();


public slots:
    //Слот, который будет ловить сигнал от представления, о том какой каталожный айтем выбран
    void cat_item_selected(QVariant Id);
    void book_item_selected(QModelIndex id);
    //обект содержащий фильтр
    void apply_filter(QObject *F);

private:
    //Дополнительная функция для фильтра
    void adjust_query();
    void adjust_all_query();

    // QAbstractItemModel interface
    //Чтобы скрыть некоторые столбцы, пользуемся этой функцией
public:
    QVariant data(const QModelIndex &I, int role) const;
    bool setData(const QModelIndex &I, const QVariant &val, int role);
    int columnCount(const QModelIndex &) const {return 12;}
    Qt::ItemFlags flags(const QModelIndex &I) const;

//signals:
//    void clearView();
};
/************************************************/



/************************************************/
//Представление
/************************************************/
class BooksView : public QTableView
{
    Q_OBJECT

public:
    BooksView(QWidget *parent = 0);
    virtual ~BooksView(){}

    // QAbstractItemView interface
protected slots:
    void currentChanged(const QModelIndex &current, const QModelIndex &previous);

signals:
    void book_item_selected(QModelIndex id);
};
/************************************************/


#endif // BOOKS_H
