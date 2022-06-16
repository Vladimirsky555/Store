#include "catalogue.h"

#include <QMenu>
#include <QtSql>
#include <QDebug>
#include <QMessageBox>

#include "posaction.h"

//Создаём локальный макрос, который облегчит написание кода
#define DATA_PTR(D,I,VALUE)                                   \
    Data *D = (Data*)(I.internalPointer());                   \
    if(!D) return VALUE

#define DATE_STR(DATE)                                         \
    (DATE.isValid() ? DATE.toString("dd.MM.yyyy") : QString()) \

/************************************************/
//Конструктор модели
/************************************************/

Model::Model(QObject *parent) :
    QAbstractItemModel(parent),
    LastTempId(1)
{
    QSqlQuery qry;
    qry.prepare(
                "SELECT              \n"
                "   iid,             \n"
                "   code,            \n"
                "   title,           \n"
                "   valid_from,      \n"
                "   valid_to,        \n"
                "   islocal,         \n"
                "   acomment,        \n"
                "   rid_parent,      \n"
                "   alevel           \n"
                "   FROM catalogue   \n"
                "   ORDER BY alevel; \n"
                //чтобы элементы не загружались раньше родителей
                //Когда мы загружаем элементы из базы, нужно заранее отсортировать
                //Чтобы гарантировать, то родители загружаются раньше детей
                //Это единственное назначение этого столбца в базе
                );

    if(qry.exec()){
        while(qry.next()){
            bool OK = false;//OK = true, если правильное число и наоборот
            QVariant PI = qry.value("rid_parent");
            int ParentId = PI.toInt(&OK);

            Data *P = 0;

            if(OK && !PI.isNull()){ //В базе NULL тоже значение
                P = catalog.findPointer(ParentId);

//                qDebug() << "Ребёнок: " <<
//                            qry.value("iid").toInt() << " - " <<
//                            qry.value("title").toString() << " --> " <<
//                            "Родитель: " <<
//                            ParentId << " - " <<
//                            P->getTitle();
            }

            if(P){
                //Добавляем элемент к списку дочерних
                Data *D = new Data(P, qry);
                P->addChild(D);
                D->setParentItem(P);
            } else {
                //Если не нашли родителя, то поступаем как поступали
                Data *D = new Data(this, qry);
                catalog.append(D);
                D->setParentItem(NULL);
            }
        }
    } else {
        qCritical() << "Cannot execute query!";
        QSqlError err = qry.lastError();
        qCritical() << err.nativeErrorCode();
        qCritical() << err.databaseText().toUtf8().data();
        qCritical() << err.driverText().toUtf8().data();
        qDebug() << qry.executedQuery();
    }
}

Model::~Model(){}

Data *Model::at(int k)
{
    return catalog.at(k);
}


//Data *Model::dataDataBlock(const QModelIndex &I) const
//{
//    int R = I.row();
//    if(R < 0 || R >= catalog.size()) return 0;
//    return catalog.at(R);
//}

void Model::addItem(Data *D)
{
    catalog.append(D);
}


void Model::editItem(const QModelIndex &I, QWidget *parent)
{
    //DATA_PTR(D, I, ); //вместо нижних 2-х строчек
    Data *D = (Data*)I.internalPointer();
    if(!D)return;

    Dialog d(D, parent);

    beginResetModel();
    d.exec();
    endResetModel();
}

void Model::newItem(const QModelIndex &parentI, QWidget *parent)
{
    Data *P = 0;

    if(parentI.isValid()){
        P = (Data*)(parentI.internalPointer());
        if(!P){
            qWarning() << "Invalid internal pointer";
            return;
        }
    }

    Data *D = new Data(this);
    if(!D){
        qWarning() << "Can not create new item";
        return;
    }

    Dialog d(D, parent);
    d.exec();

    //Чтобы при нажатии на cancel уничтожить D
//    if(d.exec() == QDialog::Accepted){
        beginResetModel();
        if(P){
            P->addChild(D);//Добавляем D в список детей P
            D->setParentItem(P);//указываем, что P родитель для D
        } else {
            catalog.append(D);
        }
        D->setProperty("temp_id", tempId());//Присваиваем свойство каждому вновь созданному элементу
        endResetModel();
//    } else {
//        delete D;
//    }
}

void Model::delItem(const QModelIndex &I, QWidget *parent)
{
    //DATA_PTR(D, I, ); //вместо нижних 2-х строчек
    Data *D = (Data*)(I.internalPointer());
    if(!D) return;

    beginResetModel();
    //Новый элемент удаляем, старый помечаем
    if(D->isNew()){
        Data *P = D->getParentItem();
        if(P){
            //Если родитель есть, то удаляем из списка родителя элемент
            P->deleteChild(I.row());
        } else {
            //Родителя нет, удаляем корневой элемент
            catalog.removeAt(I.row());
        }
        delete D;//удаляем указатель
    } else {
        //Меняем на противоположное каждый раз, для того чтобы восстанавливать и удалять
        D->setDeleted(!D->getDeleted());//не удаляем, а помечаем
//        D->deleted = !D->deleted;
    }
    endResetModel();
}




/************************************************/
// QAbstractItemModel interface
/************************************************/

//Возвращает индекс элемента, заданного номером строки, столбца,
//индексом родительского элемента
QModelIndex Model::index(int row, int column, const QModelIndex &parentI) const
{
    //qDebug() << row << column << parentI.row();
    if(parentI.isValid()){
        //DATA_PTR(D, index, QModelIndex());
        Data *P = (Data*)(parentI.internalPointer());
        if(!P) return QModelIndex();
        if(row < 0 || row >= P->Children().size()) return QModelIndex();
        //передаём ссылку на текущий элемент, а не на родительский
        //P - ссылка на родителя
        return createIndex(row, column, (void*)P->Children().at(row));
    } else {
        if(row < 0 || row >= catalog.size()) return QModelIndex();
        return createIndex(row, column, (void*)(catalog.at(row)));
    }
}

//Возвращает индекс родительского элемента
//Если родитель отсутствует, то индекс родителя пуст
//Если родитель присутствует, то мы находим индекс родителя в контексте дедушки
//Если дедушки нет, то находим индекс родителя в контексте корневого каталога
QModelIndex Model::parent(const QModelIndex &I) const
{
    //DATA_PTR(D, child, QModelIndex());
    Data *D = (Data*)(I.internalPointer());
    if(!D) return QModelIndex();

    Data *P = D->getParentItem();//получаем родителя

    if(!P){//Если P верхнего уровня, то есть нет родителя, P = NULL;
        return QModelIndex();
        //Если есть родитель, то нужен дедушка
    } else {//Если P != NULL
        int Row = -1;
        Data *GP = P->getParentItem();
        if(GP){
            //Если дедушка есть
            //Ищем индекс родителя в списке детей дедушки
            for(int i = 0; i < GP->Children().size(); i++){
                if(GP->Children().at(i)->isSameAs(P)){
                   Row = i;
                   break;
                }
            }
        } else {
            //Если GP пустой, дедушка отсутствует, значит ищем в корневом каталоге
            //среди каталожных элементов
            for(int i = 0; i < catalog.size(); i++){
                if(catalog[i]->isSameAs(P)){
                   Row = i;
                   break;
                }
            }
        }
        if(Row < 0){
            qWarning() << "Cannot find item";
            return QModelIndex();
        }

        return createIndex(Row, 0, P);//Row - номер строки родителя в контексте дедушки
    }
}

int Model::rowCount(const QModelIndex &parent) const
{
    //Если родитель существует, то кол-во строк 0
    if(!parent.isValid()){
        return catalog.count();
    } else {
        //DATA_PTR(D, index, QVariant());
        Data *P = (Data*)parent.internalPointer();
        if(!P) return 0;
        return P->Children().size();
    }
}

int Model::columnCount(const QModelIndex &) const
{
    //Количество колонок всегда равно 6, независимо валидный родитель или нет
    return 6;
//    if(!parent.isValid())return 6;
//    DATA_PTR(P, parent, 0);
////    Data *P = (Data*)parent.internalPointer();
////    if(!P) return 0;
//    return P->Children().size() > 0 ? 6 : 0;
}

QVariant Model::headerData(int section, Qt::Orientation orientation, int role) const
{
    //Для горизонтальных заголовков оставляем данные по умолчанию
    if(orientation != Qt::Horizontal){
        return QAbstractItemModel::headerData(section,orientation,role);
    }

    switch (role) {
    case Qt::DisplayRole:
        switch (section) {
        case 0: return tr("Название");
        case 1: return tr("Код");
        case 2: return tr("От");
        case 3: return tr("До");
        case 4: return tr("Локаль");
        case 5: return tr("Описание");
        default: return QVariant();
        }
    case Qt::TextAlignmentRole:
        return QVariant(Qt::AlignBaseline | Qt::AlignHCenter);
    case Qt::ForegroundRole:
    {//TODO Сделать шрифт жирным
        QFont F;
        F.setBold(true);
        return F;
    }
    default: return QVariant();
    }
}


QVariant Model::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:       return dataDisplay(index);//Вспомогательная функция
    case Qt::TextAlignmentRole: return dataTextAlignment(index);//Вспомогательная функция
    case Qt::ForegroundRole:    return dataForeground(index);
    case Qt::FontRole:          return dataFont(index);
    case Qt::ToolTipRole:       return dataToolTip(index);
    case Qt::BackgroundRole:    return dataBackground(index);
    case Qt::UserRole:     {//true если элемент удалён
        //DATA_PTR(D, index, false); //вместо нижних 6 строчек
        //return D->getDeleted();
        Data *D = (Data*)index.internalPointer();
        if(!D){
            return false;
        } else {
            return D->getDeleted();
        }
    }
    default: return QVariant();
    }
}
/************************************************/
//Вспомогательные функции
/************************************************/

QVariant Model::dataDisplay(const QModelIndex &index) const
{
    //DATA_PTR(D, index, QVariant());
    Data *D = (Data*)index.internalPointer();

    switch (index.column()) {
    case 0: return D->getTitle();
    case 1: return D->getCode();
        //  case 2: return DATE_STR(D->getFrom());
        //  case 3: return DATE_STR (D->getTo());
    case 2: return D->getFrom().isValid() ? D->getFrom().toString("dd.MM.yyyy") : QString();
    case 3: return D->getTo().isValid() ? D->getTo().toString("dd.MM.yyyy") : QString();
    case 4: return D->Local() ? QString("Local") : QString("not local");
        //  case 5: return D->getComment().isEmpty() ? QString() : tr("CMT");//примечания скрывают в таблицах
    case 5: return D->getComment();
    default: return QVariant();
    }
}


QVariant Model::dataTextAlignment(const QModelIndex &index) const
{
    int Result = Qt::AlignVCenter;
    Result |= index.column() == 0 ? Qt::AlignLeft : Qt::AlignHCenter;
    return Result;
}


QVariant Model::dataForeground(const QModelIndex &index) const
{
    //DATA_PTR(D, index, QVariant()); //макрос вместо нижних 2-х строчек
    Data *D = (Data*)index.internalPointer();
    if(!D) return QVariant();

    if(!D->getTo().isValid()) return QVariant();
    QColor Result = D->Local() ? QColor("blue") : QColor("black");
    if(!D->isActive()) Result.setAlphaF(0.50);//неактивный побледнеет
    if(D->isNew()) Result = QColor("green");
    return Result;
}


QVariant Model::dataBackground(const QModelIndex &I) const
{
    //DATA_PTR(D, I, QVariant());
    Data *D = (Data*)I.internalPointer();
    if(!D) return QVariant();

    if(!D->isNew()) return QVariant();

    QColor Result = QColor("green");
    Result.setAlphaF(0.3);
    return Result;
}


QVariant Model::dataFont(const QModelIndex &index) const
{
    //DATA_PTR(D, index, QVariant()); //макрос вместо нижних 2-х строчек
    Data *D = (Data*)index.internalPointer();
    if(!D) return QVariant();

    QFont F;
    if(D->getDeleted()) F.setStrikeOut(true);
    if(D->Changed()) F.setItalic(true);
    if(D->Local()) F.setBold(true);
    return F;
}

QVariant Model::dataToolTip(const QModelIndex &I) const
{
    //DATA_PTR(D, I, QVariant()); //макрос вместо нижних 2-х строчек
    Data *D = (Data*)I.internalPointer();
    if(!D) return QVariant();

    //Работает в табличном представлении
//    switch (I.column()) {
    //Сработает если дата истечения валидная
//    case 2: {
        if(!D->getTo().isValid())return QVariant();
        return QString("Действителен до: %1").arg(D->getTo().toString("dd.MM.yyyy"));
//    }
//    default: return QVariant();
//    }
}


/************************************************/
/************************************************/
// Внесение изменений в базу данных
/************************************************/
/************************************************/

/************************************************/
// Блок добавления новых элементов в базу
/************************************************/
/*
 *    must_be_saved - промежуточная переменная.
 * 1) В прикладном программировании скорость программы менее критична, чем её ясность
 * 2) Оптимизатор скорее всего эту переменную прибъёт
 */
bool Model::insert_all_to_db(Data *D)
{
    //1) Родительские элементы должны быть сохранены раньше дочерних
    //2) Родительский элемент сохраняем, если он существует и помечен как new
    //D->isNew() может быть вычислено раньше, и тогда ошибка, если D = NULL - случай if(D && D->isNew())
    bool must_be_saved = D ? D->isNew() : false;

    if(must_be_saved){
        QSqlQuery INS;
        INS.setForwardOnly(true);
        INS.prepare(
          "INSERT INTO catalogue (code,title,valid_from,valid_to,islocal,acomment,rid_parent)"
          "VALUES (:CODE,:TITLE,:FROM,:TO,:LOCAL,:COMMENT,:PARENT)"
          "RETURNING iid,code,title,valid_from,valid_to,islocal,acomment,rid_parent,alevel;"
                  );

        INS.bindValue(":CODE",    D->getCode());
        INS.bindValue(":TITLE",   D->getTitle());
        INS.bindValue(":FROM",    D->getFrom());
        INS.bindValue(":TO",      D->getTo());
        INS.bindValue(":LOCAL",   D->Local());
        INS.bindValue(":COMMENT", D->getComment());
        //int L = D->getParentItem()->level + 1;
        /*
        * Данные могут записываться в базу и из других источников, поэтому этот
        * метод поиска уровня иерархии не самый лучший.
        * Нужно чтобы вычислялась данная величина автоматически
        * Лучше реализовать это средствами базы данных
        * Так как запись в базу можно осуществлять из разных источников
        */
        QVariant idParent = QVariant();

        //Если родительский элемент тоже новый, Id у него пока еще нет
        //Мы исходим из того, что у родительского элемента Id появляется только тогда
        //когда все элементы сохранены - либо все, либо ни одного
        if(D->getParentItem()){
            if(D->getParentItem()->isNew()){
                idParent = D->getParentItem()->property("new_id");
            } else {
                idParent = D->getParentItem()->getId();
            }
        }

        INS.bindValue(":PARENT", idParent);

        if(!INS.exec()){
            qCritical() << INS.lastError().databaseText().toUtf8().data();
            qCritical() << INS.lastError().driverText();
            qCritical() << INS.lastError().nativeErrorCode();
            return false;
        }

        //Вероятно здесь из базы мы получаем id и alevel и записываем в модель
        while(INS.next()){
            D->setCode(INS.value("code").toString());
            D->setTitle(INS.value("title").toString());
            D->setFrom(INS.value("valid_from").toDateTime());
            D->setTo(INS.value("valid_to").toDateTime());
            D->setLocal(INS.value("islocal").toBool());
            D->setComment(INS.value("acomment").toString());
            qDebug() << INS.value("iid").isNull() <<
                        INS.value("rid_parent").isNull() <<
                        INS.value("alevel").isNull();
            qDebug() << INS.value("iid") << INS.value("rid_parent") << INS.value("alevel");
            //D->setId(INS.value("iid"));//нехорошо
            D->setProperty("new_id", INS.value("iid"));//сохраняем id во временное свойство
            qDebug() << D->property("new_id");
        }
    }{
        List *Children = D ? &(D->children) : &catalog;
        Data *tmp;
        foreach (tmp, *Children) {
            if(!insert_all_to_db(tmp))
                return false;
        }
    }
    return true;
}

bool Model::ajust_id_for_new(Data *D)
{
    //Присваиваем Id корневому элементу
    bool must_be_saved = D ? D->isNew() : false;
    //Только новым элементам присваивается Id, так как у старых он есть
    if(must_be_saved){
        D->setId(D->property("new_id"));
    }

    //Присваиваем Id дочерним элементам
    List *Children = D ? &(D->children) : &catalog;
    Data *tmp;
    foreach (tmp, *Children) {
        if(!ajust_id_for_new(tmp))
            return false;
    }

    return true;
}

/************************************************/
// Блок сохранения изменений в базу
/************************************************/
bool Model::save_all_to_db(Data *D)
{
    List *Children = D ? &(D->children) : &catalog;
    Data *tmp;
    foreach (tmp, *Children) {
        if(!save_all_to_db(tmp))
            return false;
    }

    if(!D) return true;
    if(!D->Changed()) return true;

    {
        //Исходим из того, что родительский компонент не меняется
        QSqlQuery UPD;
        UPD.setForwardOnly(true);
        UPD.prepare("UPDATE catalogue set          \n"
                    "   code  = :CODE,             \n"
                    "   title  = :TITLE,           \n"
                    "   valid_from  = :FROM,       \n"
                    "   valid_to  = :TO,           \n"
                    "   islocal  = :LOCAL,         \n"
                    "   acomment  = :COMMENT       \n"
                    "WHERE iid = :IID;             \n"
                    );
        UPD.bindValue(":CODE",    D->getCode());
        UPD.bindValue(":TITLE",   D->getTitle());
        UPD.bindValue(":FROM",    D->getFrom());
        UPD.bindValue(":TO",      D->getTo());
        UPD.bindValue(":LOCAL",   D->Local());
        UPD.bindValue(":COMMENT", D->getComment());
        UPD.bindValue(":IID",     D->getId());
        if(UPD.exec()) return true;
        qCritical() << UPD.lastError().databaseText();
        qCritical() << UPD.lastError().driverText();
        qCritical() << UPD.lastError().nativeErrorCode();
    }
    return false;
}

//Сбрасываем метку у элемента и всех дочерних
//Не важно чему она была равна, сбрасываются все метки
bool Model::drop_changed_mark(Data *D)
{
    List *Children = D ? &(D->children) : &catalog;
    Data *tmp;
    foreach (tmp, *Children) {
        //tmp->Changed = false;
        tmp->setChanged(false);
        drop_changed_mark(tmp);
    }
    return true;
}

/************************************************/
// Блок удаления элементов из базы и модели
/************************************************/
//Удаляем элемент Р и все его дочерние элементы
//Сначала удаляем из базы данных и если это удалось, то из модели
bool Model::delete_all_from_db(Data *D)
{
    //Удаляем P и все его дочерние элементы
    List *Children = D ? &(D->children) : &catalog;

    Data *tmp;
    foreach (tmp, *Children) {
        if(!delete_all_from_db(tmp))
            return false;
    }

    if(!D) return true;//Если D не указан, удалять нечего
    if(!D->getDeleted()) return true; //Если D не помечен для удаления, то его не нужно удалять

    //Отдельный блок, чтобы 2 запроса в базе не висели одновременно
    //PostgreSql разрулит ситуацию, но другие базы данных не справятся
    {
        QSqlQuery DEL;
        DEL.setForwardOnly(true);
        DEL.prepare("DELETE FROM catalogue "
                    "WHERE iid = :IID;");//sql-запрос с параметрами
        DEL.bindValue(":IID", D->getId());

        if(DEL.exec()) return true;

        qCritical() << DEL.lastError().databaseText();
        qCritical() << DEL.lastError().driverText();
        qCritical() << DEL.lastError().nativeErrorCode();
    }

    return false;
}

//Пробегаемся по всем элементам и удаляем помеченные на удаление
//Если у элемента нет помеченных, идём в список его детей,
//там ищем помеченные для удаления
bool Model::delete_all_from_model(Data *D)
{
    //Удаляем D и все его дочерние элементы
    List *Children = D ? &(D->children) : &catalog;

    bool Result = true;

    beginResetModel();
    for(int i = Children->size() - 1; i >= 0; i--) {//удаляем с конца в начало
        //qDebug() << Children->at(i)->getDeleted();
        if(Children->at(i)->getDeleted()){//удаляем помеченные как удалённые
            Data *tmp = Children->takeAt(i);//удаляет из списка и возвращает элемент
            delete tmp;
        } else {
            if(!delete_all_from_model(Children->at(i)))
            {
                Result = false;
                break;
            }
        }
    }

    endResetModel();

    return Result;
}

/**************************************************************************/
// Управляющий блок для добавления, сохранения и удаления в базу и модель
/**************************************************************************/
bool Model::insert_all()
{
    QSqlDatabase DB = QSqlDatabase::database();//получаем дескриптор БД по умолчанию
    DB.transaction();//транзакция по базе

    if(insert_all_to_db()){
        DB.commit();
        return ajust_id_for_new();
    } else {
        DB.rollback();
        return false;
    }
}

bool Model::save_all()
{
    QSqlDatabase DB = QSqlDatabase::database();//получаем дескриптор БД по умолчанию
    DB.transaction();//транзакция по базе

    if(save_all_to_db()){
        DB.commit();
        return drop_changed_mark();
    } else {
        DB.rollback();
        return false;
    }
}


//Удаление и из базы и из модели. В противном случае откат.
bool Model::delete_all()
{
    //получаем дескриптор БД по умолчанию, тот
    //который мы создали в Application.cpp
    QSqlDatabase DB = QSqlDatabase::database();
    DB.transaction();//транзакция по базе

    if(delete_all_from_db()){
        DB.commit();//сохраняем транзакцию, в случае успеха
        return delete_all_from_model();//удаляем и из модели, если из базы удалилось
    } else {
        DB.rollback();//откат транзакции
        return false;
    }
}


void Model::save()
{    
    //Механизм локальных исключений
    //В функции, где выброшено исключение
    //Особенность локальных исключений - нужно явно указывать тип
    try
    {
        if(!delete_all()) throw (int)1;//1) Удалить все элементы, помеченные для удаления
        if(!save_all())   throw (int)2;//2) Сохранить все изменённые элементы и сбросить пометку "изменено"
        if(!insert_all()) throw (int)3;//3) Сохранить все новые элементы в базу и выставить им id

        QMessageBox::information(0, "Информация", "Изменения сохранены успешно!");
    }
    catch(int X)
    {
        QMessageBox::critical(0, "Ошибка", "Нет возможности сохранить изменения!");
    }
}










/************************************************/
//Представление табличное
/************************************************/
TableView::TableView(QWidget *parent, Model *xModel) :
    QTableView(parent)
{
    //Model *M = new Model(this);
    Model *M = xModel ? xModel : new Model(this);
    setModel(M);

    //Включаем режим всплывающего меню для редактирования
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequsted(QPoint)));

    {//Создаём Action для вызова диалога для редактирования элемента в представлении
        PosAction *A = actEditItem = new PosAction(this);
        A->setText(tr("Редактировать"));
        //Переносим редактирование в модель
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(editItem(QModelIndex,QWidget*)));
        addAction(A);
    }{//Создаём Action для вызова диалога для добавления элемента в представлении
        PosAction *A = actNewItem = new PosAction(this);
        A->setText(tr("Добавить"));
        //Переносим добавление в модель
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(newItem(QModelIndex,QWidget*)));
        addAction(A);
    }{
        PosAction *A = actDeleteItem = new PosAction(this);
        A->setText(tr("Удалить"));
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(delItem(QModelIndex,QWidget*)));
        addAction(A);
    }{
        PosAction *A = actRootItem = new PosAction(this);
        A->setText(tr("Cписок"));
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                this, SLOT(showChildren(QModelIndex,QWidget*)));
        addAction(A);
    }{
        QAction *A = actParentRootItem = new QAction(this);
        A->setText(tr("Назад"));
        connect(A, SIGNAL(triggered()),this, SLOT(showParent()));
        addAction(A);
    }{
        QAction *A = actSave = new QAction(this);
        A->setText(tr("Сохранить"));
        //Переносим добавление в модель
        connect(A, SIGNAL(triggered()),M, SLOT(save()));
        addAction(A);
    }

    //Выравнивание высоты вертикальных строк под контекст
    {
        QHeaderView *H = verticalHeader();
        H->setSectionResizeMode(QHeaderView::ResizeToContents);
    } {//Выравнивание горизонтальных строк
        QHeaderView *H = horizontalHeader();
        H->setSectionResizeMode(QHeaderView::ResizeToContents);
        H->setSectionResizeMode(5, QHeaderView::Stretch);
    }

    //Чтобы колонка с переменной To не отображалась
    //     setColumnHidden(3, true);
    //     setColumnHidden(4, true);
}


void TableView::contextMenuRequsted(const QPoint &p)
{
    QMenu M(this);
    //Показывать элемент, если есть что редактировать
    QModelIndex I = indexAt(p);
    if(I.isValid())
    {
        actEditItem->I = I;
        actEditItem->pWidget = this;
        M.addAction(actEditItem);

        actDeleteItem->I = I;
        actDeleteItem->pWidget = this;
        if(I.data(Qt::UserRole).toBool()){//Если элемент удалён
            actDeleteItem->setText(tr("Восстановить"));
        } else {
            actDeleteItem->setText(tr("Удалить"));
        }
        M.addAction(actDeleteItem);
    }

    actNewItem->I = rootIndex();
    actNewItem->pWidget = this;
    M.addAction(actNewItem);

    if(I.isValid())
    {
        actRootItem->I = I;
        actRootItem->pWidget = this;//или NULL
        M.addAction(actRootItem);
    }

    if(rootIndex().isValid())
    {
        M.addAction(actParentRootItem);
    }

    M.addAction(actSave);

    //M.exec(p);//p - точка всплыва меню
    //показывается в координатах видимой области окна
    M.exec(mapToGlobal(p)); //тоже промахивается
}

void TableView::showChildren(const QModelIndex &I, QWidget *)
{
    //Берёт индекс и устанавливает этот элемент как корневой
    setRootIndex(I);
}

void TableView::showParent()
{
    if(rootIndex().isValid())
    setRootIndex(rootIndex().parent());
}

/************************************************/
//Представление в виде дерева
/************************************************/

TreeView::TreeView(QWidget *parent, Model *xModel) :
    QTreeView(parent)
{
    //Model *M = new Model(this);
    Model *M = xModel ? xModel : new Model(this);
    setModel(M);

    //Включаем режим всплывающего меню для редактирования
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequsted(QPoint)));

    {//Создаём Action для вызова диалога для редактирования элемента в представлении
        PosAction *A = actEditItem = new PosAction(this);
        A->setText(tr("Редактировать"));
        //Переносим редактирование в модель
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(editItem(QModelIndex,QWidget*)));
        addAction(A);
    }{//Создаём Action для вызова диалога для добавления элемента в представлении
        PosAction *A = actNewItem = new PosAction(this);
        A->setText(tr("Добавить"));
        //Переносим добавление в модель
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(newItem(QModelIndex,QWidget*)));
        addAction(A);
    }{
        PosAction *A = actDeleteItem = new PosAction(this);
        A->setText(tr("Удалить"));
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(delItem(QModelIndex,QWidget*)));
        addAction(A);
    }{
        PosAction *A = actRootItem = new PosAction(this);
        A->setText(tr("Cписок"));
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                this, SLOT(showChildren(QModelIndex,QWidget*)));
        addAction(A);
    }{
        QAction *A = actParentRootItem = new QAction(this);
        A->setText(tr("Назад"));
        connect(A, SIGNAL(triggered()),this, SLOT(showParent()));
        addAction(A);
    }{
        QAction *A = actSave = new QAction(this);
        A->setText(tr("Сохранить"));
        //Переносим добавление в модель
        connect(A, SIGNAL(triggered()),M, SLOT(save()));
        addAction(A);
    }

    {//Выравнивание горизонтальных строк
        QHeaderView *H = header();
        H->setSectionResizeMode(QHeaderView::ResizeToContents);
        H->setSectionResizeMode(1, QHeaderView::Stretch);
    }

//    setColumnHidden(3, true);
//    setColumnHidden(4, true);
}


void TreeView::contextMenuRequsted(const QPoint &p)
{
    QMenu M(this);
    //Показывать элемент, если есть что редактировать
    QModelIndex I = indexAt(p);
    if(I.isValid())
    {
        actEditItem->I = I;
        actEditItem->pWidget = this;
        M.addAction(actEditItem);

        actDeleteItem->I = I;
        actDeleteItem->pWidget = this;
        if(I.data(Qt::UserRole).toBool()){//Если элемент удалён
            actDeleteItem->setText(tr("Восстановить"));
        } else {
            actDeleteItem->setText(tr("Удалить"));
        }
        M.addAction(actDeleteItem);
    }

    actNewItem->I = rootIndex();
    actNewItem->pWidget = this;
    M.addAction(actNewItem);

    if(I.isValid())
    {
        actRootItem->I = I;
        actRootItem->pWidget = this;//или NULL
        M.addAction(actRootItem);
    }

    if(rootIndex().isValid())
    {
        M.addAction(actParentRootItem);
    }

    M.addAction(actSave);

    //M.exec(p);//p - точка всплыва меню
    //показывается в координатах видимой области окна
    M.exec(mapToGlobal(p)); //тоже промахивается
}

void TreeView::showChildren(const QModelIndex &I, QWidget *)
{
    //Берёт индекс и устанавливает этот элемент как корневой
    setRootIndex(I);
}

void TreeView::showParent()
{
    if(rootIndex().isValid())
    setRootIndex(rootIndex().parent());
}

/************************************************/
//Представление колоночное
/************************************************/
ColumnView::ColumnView(QWidget *parent, Model *xModel) :
    QColumnView(parent)
{
    //Model *M = new Model(this);
    Model *M = xModel ? xModel : new Model(this);
    setModel(M);

    //Делаем равными ширину колонок
    QList<int> L;
    L << 150;
    for(int i = 0; i < 10; i++) L << 200;//остальные 200
    setColumnWidths(L);

    //Включаем режим всплывающего меню для редактирования
    setContextMenuPolicy(Qt::CustomContextMenu);
    connect(this, SIGNAL(customContextMenuRequested(QPoint)),
            this, SLOT(contextMenuRequsted(QPoint)));

    {//Создаём Action для вызова диалога для редактирования элемента в представлении
        PosAction *A = actEditItem = new PosAction(this);
        A->setText(tr("Редактировать"));
        //Переносим редактирование в модель
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(editItem(QModelIndex,QWidget*)));
        addAction(A);
    }{//Создаём Action для вызова диалога для добавления элемента в представлении
        PosAction *A = actNewItem = new PosAction(this);
        A->setText(tr("Добавить"));
        //Переносим добавление в модель
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(newItem(QModelIndex,QWidget*)));
        addAction(A);
    }{
        PosAction *A = actDeleteItem = new PosAction(this);
        A->setText(tr("Удалить"));
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                M, SLOT(delItem(QModelIndex,QWidget*)));
        addAction(A);
    }{
        PosAction *A = actRootItem = new PosAction(this);
        A->setText(tr("Cписок"));
        connect(A, SIGNAL(actionOnItem(QModelIndex,QWidget*)),
                this, SLOT(showChildren(QModelIndex,QWidget*)));
        addAction(A);
    }{
        QAction *A = actParentRootItem = new QAction(this);
        A->setText(tr("Назад"));
        connect(A, SIGNAL(triggered()),this, SLOT(showParent()));
        addAction(A);
    }{
        QAction *A = actSave = new QAction(this);
        A->setText(tr("Сохранить"));
        //Переносим добавление в модель
        connect(A, SIGNAL(triggered()),M, SLOT(save()));
        addAction(A);
    }
}


void ColumnView::contextMenuRequsted(const QPoint &p)
{
    QMenu M(this);
    //Показывать элемент, если есть что редактировать
    QModelIndex I = indexAt(p);
    if(I.isValid())
    {
        actEditItem->I = I;
        actEditItem->pWidget = this;
        M.addAction(actEditItem);

        actDeleteItem->I = I;
        actDeleteItem->pWidget = this;
        if(I.data(Qt::UserRole).toBool()){//Если элемент удалён
            actDeleteItem->setText(tr("Восстановить"));
        } else {
            actDeleteItem->setText(tr("Удалить"));
        }
        M.addAction(actDeleteItem);
    }

    actNewItem->I = rootIndex();
    actNewItem->pWidget = this;
    M.addAction(actNewItem);

    if(I.isValid())
    {
        actRootItem->I = I;
        actRootItem->pWidget = this;//или NULL
        M.addAction(actRootItem);
    }

    if(rootIndex().isValid())
    {
        M.addAction(actParentRootItem);
    }

    M.addAction(actSave);

    //M.exec(p);//p - точка всплыва меню
    //показывается в координатах видимой области окна
    M.exec(mapToGlobal(p)); //тоже промахивается
}

void ColumnView::showChildren(const QModelIndex &I, QWidget *)
{
    //Берёт индекс и устанавливает этот элемент как корневой
    setRootIndex(I);
}

void ColumnView::showParent()
{
    if(rootIndex().isValid())
        setRootIndex(rootIndex().parent());
}

//Изменён текущий выбранный элемент
void ColumnView::currentChanged(const QModelIndex &current,
                                const QModelIndex &previous)
{
    //Вызываем функцию базового класса
    QColumnView::currentChanged(current, previous);
//    qDebug() << "предыдущий - " << previous << "текущий - " << current;
//    qDebug() << "";

    if(!current.isValid()){
        emit item_selected(QVariant());
        return;
    }

    Data *D = (Data*)(current.internalPointer());

    if(!D) {
        emit item_selected(QVariant());
        return;
    }

    emit item_selected(D->getId());

}

/************************************************/
/************************************************/

//Это запрос на добавление элементов, можно использовать
//                   "INSERT INTO catalogue (      \n"
//                    "     code, title,           \n"
//                    "     valid_from, valid_to,  \n"
//                    "     islocal, acomment,     \n"
//                    "     rid_parent             \n"
//                    ") VALUES              (     \n"
//                    "     :CODE, :TITLE,         \n"
//                    "     :FROM, :TO,            \n"
//                    "     :LOCAL,:COMMENT,       \n"
//                    "     :PARENT                \n"
//                    ") RETURNING iid,            \n"
//                    "     code,  title,          \n"
//                    "     valid_from, valid_to,  \n"
//                    "     islocal,  acomment,    \n"
//                    "     rid_parent, alevel  ;  \n"
