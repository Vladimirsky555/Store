#include "application.h"

#include <QtSql>

Application::Application(int argc, char *argv[]) : QApplication(argc, argv)
{
    QSqlDatabase db = QSqlDatabase::addDatabase("QPSQL");
    db.setHostName("localhost"); //Параметры подключения
    db.setPort(5432);
    db.setDatabaseName("Store");
    db.setUserName("admin");//Так логин не указывать
    db.setPassword("1234");//Так пароль не указывать
    db.open("admin", "1234");
}

Application::~Application(){}


