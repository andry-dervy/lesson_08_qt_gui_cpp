#ifndef WIDGETTREEDIRS_H
#define WIDGETTREEDIRS_H

#include <QWidget>
#include <QTreeView>
#include <QTreeWidgetItem>
#include <QFileSystemModel>
#include <QEvent>

class FileSystemModel: public QFileSystemModel
{
    Q_OBJECT
public:
    FileSystemModel(QWidget *parent);
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    bool setHeaderData(int section, Qt::Orientation orientation, const QVariant &value, int role = Qt::EditRole) override;
    void retranslate();

private:
    QMap<int, QVariant> headers;
};

class WidgetTreeDirsSingleton : public QWidget
{
    Q_OBJECT
private:
    WidgetTreeDirsSingleton(QWidget *parent = nullptr);
    ~WidgetTreeDirsSingleton() = default;
    WidgetTreeDirsSingleton(const WidgetTreeDirsSingleton& root) = delete;
    WidgetTreeDirsSingleton& operator=(const WidgetTreeDirsSingleton&) = delete;
    WidgetTreeDirsSingleton(WidgetTreeDirsSingleton&& root) = delete;
    WidgetTreeDirsSingleton& operator=(WidgetTreeDirsSingleton&&) = delete;

public:
    static WidgetTreeDirsSingleton* getInstance(QWidget *parent);
    void emitOpenFile(QString& fileName);
    void retranslate();
private:
    QTreeView* treeView;
    FileSystemModel *model;

    void setEventFilter(QWidget* wdgt, QObject* eventFilter);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
signals:
    void openFile(QString& fileName);
};

class TreeDirsEventFilter: public QObject
{
    Q_OBJECT
public:
    TreeDirsEventFilter(){}
    ~TreeDirsEventFilter() override {}
protected:
    bool eventFilter(QObject *obj, QEvent *event) override;

signals:
    void closeDock();
};

#endif // WIDGETTREEDIRS_H
