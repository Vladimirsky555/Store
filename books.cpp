#include "books.h"

#include <QtSql>
#include <QHeaderView>
#include <QAction>
#include <QPainter>
#include <QDebug>

//Один из вариантов в виде макроса
//#define REPORT_ERROR(query)                               \
//    qDebug() << query.executedQuery();                                \
//    qCritical() << query.lastError().databaseText().toUtf8().data();

//закрываем в анонимное пространство имён, чтобы невозмоно
//было использовать эту функцию в других файлах
//Анонимное пространство имён образует блок, в отличие от обычного
namespace {
void REPORT_ERROR(QSqlQuery &QUERY){
    qDebug() << QUERY.executedQuery();
    qCritical() << QUERY.lastError().databaseText().toUtf8().data();
}
}

/************************************************/
//Модель для отображения данных
/************************************************/
BooksModel::BooksModel(QObject *parent) :
    QSqlTableModel(parent)
{
    //Устанавливаем стратегию редактирования
    setEditStrategy(OnFieldChange);

    filter = false;

    {
        QAction *A = actNewRow = new QAction(this);
        A->setText(tr("Добавить"));
        connect(A, SIGNAL(triggered()), this, SLOT(on_new_row()));
        allActions << A;
    }
//    {
//        QAction *A = actSaveAll = new QAction(this);
//        A->setText(tr("Save"));
//        connect(A, SIGNAL(triggered()), this, SLOT(on_save_one()));
//        allActions << A;
//    }
    {
        QAction *A = actUpdateAll = new QAction(this);
        A->setText(tr("Редактировать"));
        connect(A, SIGNAL(triggered()), this, SLOT(on_update_one()));
        allActions << A;
    }{
        QAction *A = actDeleteRow = new QAction(this);
        A->setText(tr("Удалить"));
        connect(A, SIGNAL(triggered()), this, SLOT(on_delete_row()));
        allActions << A;
    }

    {//Загружаем все статусы из базы
        QSqlQuery QUERY;
        QUERY.prepare("select iid, title from status");
        bool OK = QUERY.exec();
        if(!OK){
            REPORT_ERROR(QUERY);
            return;
        } else {
            while(QUERY.next()){
                int id = QUERY.value("iid").toInt(); //Id из базы
                allStatus[id] = QUERY.value("title").toString();//текст статуса из базы
                //                 qDebug() << id << " - status - " << allStatus[id];
            }
        }
    }
}


void BooksModel::adjust_query()
{
    this->clear();

    QString queryText =
            "SELECT                           \n"
            "    b.iid,                       \n"
            "    b.rid_catalogue,             \n"
            "    b.author,                    \n"
            "    b.title,                     \n"
            "    b.year,                      \n"
            "    b.location,                  \n"
            "    b.pablisher,                 \n"
            "    b.pages,                     \n"
            "    b.annote,                    \n"
            "    b.rid_status,                \n"
            "    s.title,                     \n"
            "    b.acomment                   \n"
            "FROM books b                     \n"
            "left outer join status s         \n"
            "on b.rid_status = s.iid          \n"
            "WHERE b.rid_catalogue = :CID     \n";

    //Блок работает для фильтра
    if(filter)
    {
        if(fAuthor.isValid())
            queryText += " and b.author ~ :AUTHOR   \n";
        if(fTitle.isValid())
            queryText += " and b.title ~ :TITLE     \n";
        if(fYear.isValid())
            queryText += " and b.year = :YEAR       \n";
        queryText += " ;                            \n";
    }


    QSqlQuery qry;
    qry.prepare(queryText);
    qry.bindValue(":CID", fCatId);

    //Присваиваем значения для фильтра
    if(filter)
    {
        if(fAuthor.isValid())
            qry.bindValue(":AUTHOR", "^" + fAuthor.toString());
        if(fTitle.isValid())
            qry.bindValue(":TITLE", fTitle);
        if(fYear.isValid())
            qry.bindValue(":YEAR", fYear);
    }


    if(!qry.exec()){//открываем курсор
        qCritical() << qry.lastError().databaseText().toUtf8().data();
    }


    while(qry.next()){
        qDebug() <<  qry.value("iid").toInt() <<
                     qry.value("author").toString() <<
                     qry.value("title").toString() <<
                     qry.value("year").toInt() <<
                     qry.value("location").toString() <<
                     qry.value("pablisher").toString() <<
                     qry.value("pages").toInt() <<
                     qry.value("annote").toString() <<
                     qry.value("rid_status").toInt() <<
                     qry.value("acomment").toString();
    }


    qDebug() << "-------------------------------------";


    setQuery(qry);//всё что нужно для отражения данных в представлении
    //в данном типе модели
}

//void BooksModel::on_save_one()
//{
//    qDebug() << "on_save_all";

//    QSqlQuery INS;
//    INS.setForwardOnly(true);

//    INS.prepare("INSERT INTO books (author, rid_catalogue, title, year, location, pablisher, pages, annote, acomment)"
//                "VALUES (:AUTHOR, :RID_CATALOGUE, :TITLE, :YEAR, :LOCATION, :PABLISHER, :PAGES, :ANNOTE, :ACOMMENT)");

//    INS.bindValue(":AUTHOR", record(0).value("author").toString());
//    INS.bindValue(":RID_CATALOGUE", fCatId.toInt());
//    INS.bindValue(":YEAR", record(0).value("year").toInt());
//    INS.bindValue(":TITLE", record(0).value("title").toString());
//    INS.bindValue(":LOCATION", record(0).value("location").toString());
//    INS.bindValue(":PABLISHER", record(0).value("pablisher").toString());
//    INS.bindValue(":PAGES", record(0).value("pages").toInt());
//    INS.bindValue(":ANNOTE", record(0).value("annote").toString());
//    INS.bindValue(":ACOMMENT", record(0).value("acomment").toString());
////    INS.bindValue(":RID_STATUS", record(0).value("rid_status").toInt());


//    if(INS.exec()) {
//        adjust_query();
//        return;
//    }

//    qCritical() << INS.lastError().databaseText();
//    qCritical() << INS.lastError().driverText();
//    qCritical() << INS.lastError().nativeErrorCode();
//}

void BooksModel::on_update_one()
{
    QSqlQuery UPD;
    UPD.setForwardOnly(true);

    UPD.prepare("UPDATE books SET  "
//                " iid = :ID, "
                " author = :AUTHOR, "
                " title = :TITLE, "
                " year = :YEAR, "
                " location = :LOCATION, "
                " pablisher = :PABLISHER, "
                " pages = :PAGES, "
                " annote = :ANNOTE,"
                " rid_status = :RID_STATUS, "
                " acomment = :ACOMMENT "
                "WHERE iid = :IID; "
                );

    UPD.bindValue(":IID", record(currentId).value("iid").toInt());
    UPD.bindValue(":AUTHOR", record(currentId).value("author").toString());
    UPD.bindValue(":YEAR", record(currentId).value("year").toInt());
    UPD.bindValue(":TITLE", record(currentId).value("title").toString());
    UPD.bindValue(":LOCATION", record(currentId).value("location").toString());
    UPD.bindValue(":PABLISHER", record(currentId).value("pablisher").toString());
    UPD.bindValue(":PAGES", record(currentId).value("pages").toInt());
    UPD.bindValue(":ANNOTE", record(currentId).value("annote").toString());
    UPD.bindValue(":RID_STATUS", record(currentId).value("rid_status").toString());
    UPD.bindValue(":ACOMMENT", record(currentId).value("acomment").toString());

    if(UPD.exec()) return;

    qCritical() << UPD.lastError().databaseText();
    qCritical() << UPD.lastError().driverText();
    qCritical() << UPD.lastError().nativeErrorCode();
}

void BooksModel::on_delete_row()
{
    QSqlQuery DEL;
    DEL.setForwardOnly(true);

    DEL.prepare("DELETE FROM books WHERE iid = :IID;");
    DEL.bindValue(":IID", record(currentId).value("iid").toInt());

    if(DEL.exec()){
        adjust_query();
        return;
    }

    qCritical() << DEL.lastError().databaseText();
    qCritical() << DEL.lastError().driverText();
    qCritical() << DEL.lastError().nativeErrorCode();
}

void BooksModel::on_new_row()
{
    insertRow(0);
//    setData(index(0,1), fCatId, Qt::EditRole);

    QSqlQuery INS;
    INS.setForwardOnly(true);

    INS.prepare("INSERT INTO books (author, rid_catalogue, title, year, location, pablisher, pages, annote, acomment, rid_status)"
                "VALUES (:AUTHOR, :RID_CATALOGUE, :TITLE, :YEAR, :LOCATION, :PABLISHER, :PAGES, :ANNOTE, :ACOMMENT, :RID_STATUS)");

    INS.bindValue(":AUTHOR", record(0).value("author").toString());
    INS.bindValue(":RID_CATALOGUE", fCatId.toInt());
    INS.bindValue(":YEAR", record(0).value("year").toInt());
    INS.bindValue(":TITLE", record(0).value("title").toString());
    INS.bindValue(":LOCATION", record(0).value("location").toString());
    INS.bindValue(":PABLISHER", record(0).value("pablisher").toString());
    INS.bindValue(":PAGES", record(0).value("pages").toInt());
    INS.bindValue(":ANNOTE", record(0).value("annote").toString());
    INS.bindValue(":ACOMMENT", record(0).value("acomment").toString());
    INS.bindValue(":RID_STATUS", -3);


    if(INS.exec()) {
        adjust_query();
        return;
    }

    qCritical() << INS.lastError().databaseText();
    qCritical() << INS.lastError().driverText();
    qCritical() << INS.lastError().nativeErrorCode();
}




void BooksModel::adjust_all_query()
{
    QString queryText =
            "SELECT                           \n"
            "    b.iid,                       \n"
            "    b.rid_catalogue,             \n"
            "    b.author,                    \n"
            "    b.title,                     \n"
            "    b.year,                      \n"
            "    b.location,                  \n"
            "    b.pablisher,                 \n"
            "    b.pages,                     \n"
            "    b.annote,                    \n"
            "    b.rid_status,                \n"
            "    s.title,                     \n"
            "    b.acomment                   \n"
            "FROM books b                     \n"
            "left outer join status s         \n"
            "on b.rid_status = s.iid          \n";


    if(fAuthor.isValid())
        queryText += "     WHERE b.author ~ :AUTHOR   \n";
    //        if(fYear.isValid())
    //            queryText += "     and b.year = :YEAR       \n";
    //        if(fTitle.isValid())
    //            queryText += "     and b.title ~ :TITLE     \n";
    queryText += "   ;                                \n";


    QSqlQuery qry;
    qry.prepare(queryText);

    if(fYear.isValid())
        qry.bindValue(":YEAR", fYear);
    if(fAuthor.isValid())
        qry.bindValue(":AUTHOR", "^" + fAuthor.toString());
    if(fTitle.isValid())
        qry.bindValue(":TITLE", fTitle);


    if(!qry.exec()){//открываем курсор
        qCritical() << qry.lastError().databaseText().toUtf8().data();
    }
    setQuery(qry);//всё что нужно для отражения данных в представлении
    //в данном типе модели
}

QVariant BooksModel::data(const QModelIndex &I, int role) const
{
    if(!I.isValid())return QSqlTableModel::data(I,role);
    if(role != Qt::EditRole) return QSqlTableModel::data(I,role);
    if(I.column() != 10) return QSqlTableModel::data(I,role);

    //В 10-ю колонку устанавливаем значение 9-й
    return QSqlTableModel::data(index(I.row(),9),role);
}

bool BooksModel::setData(const QModelIndex &I, const QVariant &val, int role)
{
    if(!I.isValid())return QSqlTableModel::setData(I,role);
    if(role != Qt::EditRole) return QSqlTableModel::setData(I,val,role);
    if(I.column() != 10) return QSqlTableModel::setData(I,val,role);
    bool Result = true;

    //Всё это только для 10-й колонки
    if(val.isValid()){//Валидный val
        bool OK;
        int status_id = val.toInt(&OK);
        if(!OK){//Если число некорректное
            qWarning() << "Invalid Status - " << val;
            return false;
        } else if(!allStatus.contains(status_id)){
            qWarning() << "Invalid Status - " << val;
            return false;
        } else {
            Result |= QSqlTableModel::setData(index(I.row(),9), val, role);
            Result |= QSqlTableModel::setData(I, allStatus[status_id], role);
        }
    } else {//Невалидный val
        Result |= QSqlTableModel::setData(index(I.row(),9), QVariant(), role);
        Result |= QSqlTableModel::setData(I, QString(), role);
    }

    return Result;
}

Qt::ItemFlags BooksModel::flags(const QModelIndex &I) const
{
    Qt::ItemFlags Result = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if(I.column() != 0)
        Result = Result | Qt::ItemIsEditable;//редактируем колонки, кроме 0

    return Result;
}


void BooksModel::cat_item_selected(QVariant Id)
{
    filter = false;//сбрасываем поиск по фильтру
    fCatId = Id;
    adjust_query();
}

void BooksModel::book_item_selected(QModelIndex id)
{
    this->currentId = id.row();

//    qDebug() <<  record(currentId).value("iid").toInt() <<
//                 record(currentId).value("author").toString() <<
//                 record(currentId).value("title").toString() <<
//                 record(currentId).value("year").toInt() <<
//                 record(currentId).value("location").toString() <<
//                 record(currentId).value("pablisher").toString() <<
//                 record(currentId).value("pages").toInt() <<
//                 record(currentId).value("annote").toString() <<
//                 record(currentId).value("rid_status").toInt() <<
//                 record(currentId).value("acomment").toString();
}

//По клику на фильтр, выдаётся выборка по результатам запроса
//По клику на каталожный элемент, выдаётся выборка с id = D->getId();
void BooksModel::apply_filter(QObject *F)
{
    //Мы пользуемся системой свойств Qt, нам начихать на структуру объекта
    //Элементы динамической типизации
    fAuthor = F->property("author");
    fTitle = F->property("title");
    fYear = F->property("year");
    fFlag = F->property("flag");

    bool flag = fFlag.toBool();
    filter = true;//включаем выборку по фильтру в функции ajust_query()

    if(!flag){
        adjust_query();
    } else {
        adjust_all_query();
    }
}

/************************************************/








/************************************************/
//Представление
/************************************************/
BooksView::BooksView(QWidget *parent) :
    QTableView(parent)
{
    BooksModel *M = new BooksModel(this);
    setModel(M);

    connect(this, SIGNAL(book_item_selected(QModelIndex)),
            M, SLOT(book_item_selected(QModelIndex)));

    //Добавляем наши экшены в представление
    addActions(M->allActions);
    setContextMenuPolicy(Qt::ActionsContextMenu);

    //Не работает, так как при новом запросе колонки перевычисляются
    //Для этого переопределяем виртуальную функцию columnCount модели
    setColumnHidden(0, true);
    setColumnHidden(1, true);
    //    setColumnHidden(9, true);

    setWordWrap(false); //Запрещаем длинному тексту разделяться на несколько строчек
    setAlternatingRowColors(true);//попеременный цвет строчек таблицы, для красоты

    {//Выравнивание высоты вертикальных строк под контекст
        QHeaderView *V = verticalHeader();
        V->setSectionResizeMode(QHeaderView::ResizeToContents);
    } {//Выравнивание горизонтальных строк
        QHeaderView *H = horizontalHeader();
        H->setSectionResizeMode(QHeaderView::ResizeToContents);
        H->setSectionResizeMode(3, QHeaderView::Stretch);//растягиваем заголовок
    }

    //Устанавливаем делегат
    setItemDelegateForColumn(10, new StatusDelegate(this, M->allStatus));
}

void BooksView::currentChanged(const QModelIndex &current, const QModelIndex &previous)
{
    //Вызываем функцию базового класса
    QTableView::currentChanged(current, previous);
    //    qDebug() << "предыдущий - " << previous << "текущий - " << current;
    //    qDebug() << "";

    if(!current.isValid()){
        emit book_item_selected(QModelIndex());
        return;
    }

    emit book_item_selected(current);
}
/************************************************/





/************************************************/
//Конструктор делегата
StatusDelegate::StatusDelegate(QObject *parent, QMap<int, QString> allStatus)
{
    this->allStatus = allStatus;
}

QWidget *StatusDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem &option,
                                      const QModelIndex &index) const
{
    QComboBox *cbx = new QComboBox(parent);
    QMapIterator<int,QString> K(allStatus);
    //cbx->addItem(QString(), QVariant());//чтобы не выбирать никакого статуса
    while(K.hasNext()){
        K.next();
        cbx->addItem(K.value(), K.key());//наименование и идентификатор статуса
    }
    return cbx;
}

//переносит данные с модели на редактор
void StatusDelegate::setEditorData(QWidget *editor, const QModelIndex &I) const
{
    QComboBox *cbx = qobject_cast<QComboBox*>(editor);
    if(!cbx) return;

    QVariant IdStatus = I.data(Qt::EditRole);

    if(!IdStatus.isValid()){
        cbx->setCurrentIndex(0);
        return;
    }

    //Нулевой элемент уже просчитали
    for(int i = 1; i < cbx->count(); i++){
        if(cbx->itemData(i) == IdStatus){
            cbx->setCurrentIndex(i);
            break;
        }
    }

}

//перенос данных с редактора в модель
void StatusDelegate::setModelData(QWidget *editor, QAbstractItemModel *model,
                                  const QModelIndex &I) const
{
    //Преобразовываем эдитор к комбобоксу
    QComboBox *cbx = qobject_cast<QComboBox*>(editor);
    if(!cbx) return;

    model->setData(I, cbx->currentData(), Qt::EditRole);
}

//отрисовка
void StatusDelegate::paint(QPainter *painter, const QStyleOptionViewItem &option,
                           const QModelIndex &I) const
{
    QItemDelegate::paint(painter,option,I);//отрисовка по умолчанию
    if(I.data(Qt::EditRole) != -2) return;

    //Только для значения -2
    painter->setBrush(QBrush(QColor("black"), Qt::FDiagPattern));
    painter->setPen(Qt::NoPen);//чтобы не отрисовывало границу
    painter->drawRect(option.rect);
}
