#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QDockWidget>

#include "catalogue.h"
#include "books.h"
#include "filter.h"

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    setWindowTitle("Список");

    BooksView *B = new BooksView(this);
    setCentralWidget(B);

    ColumnView *W = NULL;
//    TreeView *W = NULL;
    Filter *F = NULL;

    {
        QDockWidget *D = new QDockWidget(this);
        D->setWindowTitle(tr("Каталог"));
        W = new ColumnView(D);
//        W = new TreeView(D);
        D->setWidget(W);
        addDockWidget(Qt::TopDockWidgetArea, D);
    }
    {
        QDockWidget *D = new QDockWidget(this);
        D->setWindowTitle(tr("Фильтр"));
        F = new Filter(D);
        D->setWidget(F);
        addDockWidget(Qt::LeftDockWidgetArea, D);
    }
    //    {
    //        QDockWidget *D = new QDockWidget(this);
    //        D->setWindowTitle(tr("Каталог"));
    //        D->setWidget(new Catalogue::TreeView(D,M));
    //        addDockWidget(Qt::LeftDockWidgetArea, D);
    //    }

    connect(W, SIGNAL(item_selected(QVariant)),
            B->model(), SLOT(cat_item_selected(QVariant)));

    //Передаём данные от фильтра в модель с книгами
    connect(F, SIGNAL(apply_filter(QObject*)),
            B->model(), SLOT(apply_filter(QObject*)));
}

MainWindow::~MainWindow()
{
    delete ui;
}
