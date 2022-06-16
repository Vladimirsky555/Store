#-------------------------------------------------
#
# Project created by QtCreator 2020-02-29T18:07:41
#
#-------------------------------------------------

QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = Store
TEMPLATE = app


SOURCES += application.cpp  \
           data.cpp         \
           dialog.cpp       \
           catalogue.cpp    \
           main.cpp         \
           mainwindow.cpp   \
           posaction.cpp    \
           books.cpp        \
           filter.cpp


HEADERS  += application.h   \
            data.h          \
            dialog.h        \
            mainwindow.h    \
            catalogue.h      \
            posaction.h      \
            books.h          \
            filter.h


FORMS +=   dialog.ui     \
           mainwindow.ui \
           filter.ui



