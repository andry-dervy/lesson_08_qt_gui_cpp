#include "widgettreedirs.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDockWidget>
#include <QLineEdit>
#include <QPushButton>
#include "toolbarelementsfactory.h"
#include <QDebug>
#include <QListView>
#include <QSplitter>

extern template std::optional<QPushButton *> ToolbarElementsFactory<QPushButton>::create(const QString &&nameObject, QWidget *parent, bool checkable, const QPixmap &&icon);
extern template void ToolbarElementsFactory<QPushButton>::setText(QObject* parent,const QString&& nameObject,const QString&& text);

WidgetTreeDirsSingleton::WidgetTreeDirsSingleton(QWidget *parent)
    : QWidget(parent)
    , isSearching(false)
    , finder(nullptr)
{
    auto vBoxLayout = new QVBoxLayout();
    setLayout(vBoxLayout);

    modelFileSystem = new FileSystemModel(this);
    modelFileSystem->setRootPath(QDir::currentPath());
    QStringList sList;
    sList << "*.htxt";
    modelFileSystem->setNameFilters(sList);
    modelFileSystem->setNameFilterDisables(false);
    modelFileSystem->retranslate();

    treeView = new QTreeView(this);
    treeView->setModel(modelFileSystem);
    treeView->setCurrentIndex(modelFileSystem->index(QDir::currentPath()));
    setEventFilter(treeView,this);
    vBoxLayout->addWidget(treeView);

    auto hBoxLayout = new QHBoxLayout();
    vBoxLayout->addLayout(hBoxLayout);
    leFindPhrase = new QLineEdit(this);
    hBoxLayout->addWidget(leFindPhrase);

    auto pbFind = ToolbarElementsFactory<QPushButton>::create("pbFind",this,false,QPixmap());
    Q_ASSERT(pbFind != nullptr);
    connect(*pbFind,&QPushButton::clicked,this,&WidgetTreeDirsSingleton::pbFindClicked);
    hBoxLayout->addWidget(*pbFind);

    auto listResults = new QListView(this);
    vBoxLayout->addWidget(listResults);
    modelListResults = new QStringListModel(this);
    listResults->setModel(modelListResults);

    retranslate();
}

WidgetTreeDirsSingleton* WidgetTreeDirsSingleton::getInstance(QWidget* parent)
{
    static WidgetTreeDirsSingleton* theInstance
            = new WidgetTreeDirsSingleton(parent);
    return theInstance;
}

void WidgetTreeDirsSingleton::setEventFilter(QWidget* wdgt, QObject* eventFilter)
{
    if(wdgt == nullptr) return;
    wdgt->installEventFilter(eventFilter);
    auto listWdgts = wdgt->children();
    for(auto& obj: listWdgts)
    {
        auto w = qobject_cast<QWidget*>(obj);
        setEventFilter(w,eventFilter);
    }
}

void WidgetTreeDirsSingleton::emitOpenFile(QString& fileName)
{
    emit openFile(fileName);
}

void WidgetTreeDirsSingleton::retranslate()
{
    if(modelFileSystem != nullptr) modelFileSystem->retranslate();

    ToolbarElementsFactory<QPushButton>::setText(this,"pbFind",tr("Поиск"));
}

bool WidgetTreeDirsSingleton::eventFilter(QObject* obj, QEvent* event)
{
    switch(event->type())
    {
    case QEvent::MouseButtonDblClick:
    {
        auto indx = treeView->currentIndex();
        if(indx.isValid() && !modelFileSystem->isDir(indx))
        {
            QString path = modelFileSystem->filePath(indx);
            emit openFile(path);
            return true;
        }
        break;
    }
    default:
        break;
    }
    return QObject::eventFilter(obj, event);
}

void WidgetTreeDirsSingleton::pbFindClicked()
{
    if(isSearching)
    {
        finder->stopSearching();
        finder = nullptr;
        ToolbarElementsFactory<QPushButton>::setText(this,"pbFind",tr("Поиск"));
        isSearching = false;
        return;
    }

    auto indx = treeView->currentIndex();
    if(indx.isValid() && modelFileSystem->isDir(indx))
    {
        modelListResults->removeRows(0,modelListResults->rowCount());

        QString path{modelFileSystem->filePath(indx)};
        QDir dir{path};

        finder = new Finder(dir,leFindPhrase->text());
        QThread* thread = new QThread();
        finder->moveToThread(thread);

        connect(thread, &QThread::started, finder, &Finder::find);
        connect(finder, &Finder::finished, thread, &QThread::quit);
        connect(finder, &Finder::finished, finder, &Finder::deleteLater);
        connect(finder, &Finder::finished, this, &WidgetTreeDirsSingleton::finishedFind);
        connect(thread, &QThread::finished, thread, &QThread::deleteLater);
        connect(finder, &Finder::sendMatched, this, &WidgetTreeDirsSingleton::receiveMatched);

        ToolbarElementsFactory<QPushButton>::setText(this,"pbFind",tr("Стоп"));
        isSearching = true;

        thread->start();
    }
}

void WidgetTreeDirsSingleton::receiveMatched(QString path)
{
    modelListResults->insertRows(0,1);
    QModelIndex indx = modelListResults->index(0);
    modelListResults->setData(indx,path);
}

void WidgetTreeDirsSingleton::finishedFind()
{
    ToolbarElementsFactory<QPushButton>::setText(this,"pbFind",tr("Поиск"));
    isSearching = false;
}

FileSystemModel::FileSystemModel(QWidget *parent)
    :QFileSystemModel(parent)
{
}

bool FileSystemModel::setHeaderData(int section, Qt::Orientation orientation, const QVariant& value, int role) {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        headers[section] = value;
        return true;
    }
    return false;
}

QVariant FileSystemModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        if(headers.find(section) != headers.end())
            return headers[section];
    }

    return QFileSystemModel::headerData(section,orientation,role);
}

void FileSystemModel::retranslate()
{
    setHeaderData(0,Qt::Horizontal,QString(tr("Название")),Qt::DisplayRole);
    setHeaderData(1,Qt::Horizontal,QString(tr("Размер")),Qt::DisplayRole);
    setHeaderData(2,Qt::Horizontal,QString(tr("Тип")),Qt::DisplayRole);
    setHeaderData(3,Qt::Horizontal,QString(tr("Дата изменения")),Qt::DisplayRole);
}

bool TreeDirsEventFilter::eventFilter(QObject *obj, QEvent *event)
{
    auto dock = qobject_cast<QDockWidget*>(obj);
    if(dock == nullptr) return false;

    if(event->type() == QEvent::Type::Close)
    {
        emit closeDock();
    }
    return false;
}

Finder::Finder(QDir aDir, QString aFindText)
    :rootDir(aDir)
    ,findText(aFindText)
    ,isSearching(true)
{}

void Finder::stopSearching()
{
    mtxIsSearching.lock();
    isSearching = false;
    mtxIsSearching.unlock();
}

void Finder::find()
{
    QDirIterator it{rootDir.path(),QStringList() << findText,QDir::Files,QDirIterator::Subdirectories};
    while(isSearching && it.hasNext())
    {
        emit sendMatched(it.next());
        QThread::msleep(1);
    }
    emit finished();
}

void Finder::stop()
{
    stopSearching();
}
