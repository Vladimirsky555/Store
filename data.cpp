#include "data.h"

#include <QtSql>


Data::Data(QObject *parent) :
    QObject(parent)
{
    this->Id = QVariant();
    this->Code = "";
    this->Title = "";
    this->From = QDateTime::currentDateTime();
    this->To = QDateTime::currentDateTime();
    this->Comment = "Это примерный текст комментария";

    this->isLocal = true;
    this->pParentItem = NULL;
    this->deleted = false;
    this->changed = false;
}

Data::Data(QObject *parent, QSqlQuery &qry) :
    QObject(parent)
{
    this->Id = qry.value("iid");//не преобразовываем, так как id и так QVariant
    this->Code = qry.value("code").toString();
    this->Title = qry.value("title").toString();
    this->From = qry.value("valid_from").toDateTime();
    this->To = qry.value("valid_to").toDateTime();
    this->isLocal = qry.value("islocal").toBool();
    this->Comment = qry.value("acomment").toString();
    this->pParentItem = NULL;
    this->deleted = false;
    this->changed = false;
}

Data::~Data(){}


//Спецфункции
bool Data::isActive() const
{
    if(From.isValid()){
        if(From > QDateTime::currentDateTime()){// время активности не наступило
            return false;
        }
    }

    if(To.isValid()){
        if(To < QDateTime::currentDateTime()){//время активности прошло
            return false;
        }
    }

    return true;
}

bool Data::isNew() const
{
    //if(!Id.isValid() || Id.isNull())return true;//Id не валидный, значит нулевой
    //if(Id.isNull()) return true;//Указатель нулевой, значит новый элемент
    if(!Id.isValid()) return true;
    if(Id.isNull()) return true;
    return false;
}

//Перебираем элементы дедушки, подставляем родителя
//Если у элемента нет id (новый), то задаём ему динамическое свойство
//У новых данных нет id, поэтому сравнение идет по динамическому свойству
//Эта функция проверяет являются ли два элемента Data* одинаковыми
//Новые сравиваем по динамическим пропертям, старые по id
bool Data::isSameAs(Data *D) const
{
    if(isNew()){
        if(!D->isNew())return false;//Один новый, другой старый
        return property("temp_id") == D->property("temp_id");//Оба новые, вернёт true если они равны
    } else {
        if(D->isNew()) return false;//Один новый, другой старый
        return D->getId() == Id;//Два старых, вернёт true если они равны
    }
}


//Гетеры и сетеры
QVariant Data::getId()
{
    return this->Id;
}

void Data::setId(QVariant value)
{
    this->Id = value;
}

QString Data::getCode()
{
    return this->Code;
}

void Data::setCode(QString value)
{
    this->Code = value;
}

QString Data::getTitle()
{
    return this->Title;
}

void Data::setTitle(QString value)
{
    this->Title = value;
}

QDateTime Data::getFrom()
{
    return this->From;
}

void Data::setFrom(QDateTime value)
{
    this->From = value;
}

QDateTime Data::getTo()
{
    return this->To;
}

void Data::setTo(QDateTime value)
{
    this->To = value;
}

QString Data::getComment()
{
    return this->Comment;
}

void Data::setComment(QString value)
{
    this->Comment = value;
}

bool Data::Local()
{
    return this->isLocal;
}

void Data::setLocal(bool value)
{
    this->isLocal = value;
}

Data* Data::getParentItem()
{
    return this->pParentItem;
}

void Data::setParentItem(Data *value)
{
    this->pParentItem = value;
}

bool Data::getDeleted()
{
    return this->deleted;
}

void Data::setDeleted(bool value)
{
    this->deleted = value;
}

bool Data::Changed()
{
    return this->changed;
}

void Data::setChanged(bool value)
{
    this->changed = value;
}

List Data::Children()
{
    return this->children;
}

void Data::addChild(Data *newChild)
{
    this->children.append(newChild);
}

void Data::deleteChild(int id)
{
    this->children.removeAt(id);
}


//Функции класса List
//По индентификатору находим указатель (рекурсивная операция)
//Цель этой функции - распределить элементы по родителям
Data *List::findPointer(int id) const
{
    Data *D;
    foreach(D,*this){
        bool OK;
        int cId = D->getId().toInt(&OK);//убеждаемся, что id целое число
        if(OK && cId == id)return D;
        Data *R = D->Children().findPointer(id);//Если родитель не находится в корне, то поиск родителя идет среди детей
        if(R)return R;
    }

    return 0;
}
