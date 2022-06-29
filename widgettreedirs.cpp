#include "widgettreedirs.h"
#include <QVBoxLayout>
#include <QMouseEvent>
#include <QDockWidget>
#include <QLineEdit>
#include <QPushButton>
#include "toolbarelementsfactory.h"

WidgetTreeDirsSingleton::WidgetTreeDirsSingleton(QWidget *parent)
    : QWidget(parent)
{
    auto vBoxLayout = new QVBoxLayout();
    setLayout(vBoxLayout);

    model = new FileSystemModel(this);
    model->setRootPath(QDir::currentPath());
    QStringList sList;
    sList << "*.htxt";
    model->setNameFilters(sList);
    model->setNameFilterDisables(false);
    model->retranslate();

    treeView = new QTreeView(this);
    treeView->setModel(model);
    treeView->setCurrentIndex(model->index(QDir::currentPath()));
    setEventFilter(treeView,this);

    vBoxLayout->addWidget(treeView);

    auto hBoxLayout = new QHBoxLayout();
    vBoxLayout->addLayout(hBoxLayout);
    auto leFindPhrase = new QLineEdit(this);
    hBoxLayout->addWidget(leFindPhrase);

    auto pbFind = ToolbarElementsFactory<QPushButton>::create("pbFind",this,false,QPixmap());
    Q_ASSERT(pbFind != nullptr);
    hBoxLayout->addWidget(*pbFind);

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
    if(model != nullptr) model->retranslate();

    ToolbarElementsFactory<QPushButton>::setText(this,"pbFind",tr("Поиск"));
}

bool WidgetTreeDirsSingleton::eventFilter(QObject* obj, QEvent* event)
{
    switch(event->type())
    {
    case QEvent::MouseButtonDblClick:
    {
        auto indx = treeView->currentIndex();
        if(indx.isValid() && !model->isDir(indx))
        {
            QString path = model->filePath(indx);
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
