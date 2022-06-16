#ifndef DATA_H
#define DATA_H

#include <QObject>
#include <QDateTime>
#include <QVariant>

class QSqlQuery;
class Data;

//Вспомогательный класс для списка дочерних элементов
class List : public QList<Data*> {
public:
    List() : QList<Data*>(){}
    Data* findPointer(int id) const;
};

class Data : public QObject
{
    Q_OBJECT

    //Пиктограммы (QImage или QPixMap)
    //Библиотечные переменные
    QVariant Id;
    QString Code;     //Код подраздела
    QString Title;    //Наименование
    QDateTime From;   //Действует с ... (дата)
    QDateTime To;     //Закрыт с ... (дата)
    QString Comment;  //Комментарии

    //Встроенные переменные
    bool isLocal;      //Локальный - true внутр для организации
    Data *pParentItem; //Родительский подраздел. Будет пока равен 0, так как список.
    bool deleted;      //Помечаем удалённый элемент
    bool changed;     //Блок данных подвергался редактированию и его необходимо сохранить

    //Пришлось сделать публичной,
    //так как при удалении не получается дать ссылку на функцию
    //возвращающую список дочерних элементов
public:
    List children;     //список дочерних элементов

public:
    //Data(QObject *parent = 0) : QObject(parent), isLocal(true), pParentItem(0){};
    Data(QObject *parent = 0);
    Data(QObject *parent, QSqlQuery &qry);
    virtual ~Data();

public:
    bool isActive() const;
    bool isNew() const;
    bool isSameAs(Data* D) const;

    QVariant getId();
    void setId(QVariant value);
    QString getCode();
    void setCode(QString value);
    QString getTitle();
    void setTitle(QString value);
    QDateTime getFrom();
    void setFrom(QDateTime value);
    QDateTime getTo();
    void setTo(QDateTime value);
    QString getComment();
    void setComment(QString value);
    bool Local();
    void setLocal(bool value);

    Data* getParentItem();
    void setParentItem(Data *value);

    bool getDeleted();
    void setDeleted(bool value);

    bool Changed();
    void setChanged(bool value);

    //Получение списка детей, добавление и удаление
    List Children();
    void addChild(Data *newChild);
    void deleteChild(int id);
};

#endif // DATA_H
