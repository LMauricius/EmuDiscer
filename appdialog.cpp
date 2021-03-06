#include "appdialog.h"
#include "ui_appdialog.h"

#include <QPushButton>
#include <QIcon>
#include <QtConcurrent>
#include <QFuture>
#include <QFutureWatcher>

namespace
{
    struct IconLoaderResult
    {
        QTreeWidgetItem *item;
        QIcon icon;
    };
}

AppDialog::AppDialog(QWidget *parent) :
    QDialog(parent),
    ui(new Ui::AppDialog)
{
    ui->setupUi(this);

    AppDef root = getApps();

    for (auto &nameAppPair : root.subApps)
    {
        addApp(nameAppPair.first, nameAppPair.second, nullptr);
    }
    ui->appTree->hideColumn(1);

    ui->appTree->setCurrentItem(nullptr);
    ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(false);
}

AppDialog::~AppDialog()
{
    delete ui;
}

QString AppDialog::appPath() const
{
    return mAppPath;
    if (ui->appTree->currentItem())
        return ui->appTree->currentItem()->text(1);
    else
        return "";
}

void AppDialog::addApp(const QString &name, const AppDef& app, QTreeWidgetItem *parent)
{
    QTreeWidgetItem *item = new QTreeWidgetItem((QTreeWidget*)nullptr, {name, app.path});

    if (parent)
    {
        parent->addChild(item);
    }
    else
    {
        ui->appTree->addTopLevelItem(item);
    }

    // Load icon concurrently
    if (app.iconLoader)
    {
        std::function<QIcon()> iconLoader = app.iconLoader;
        auto future = QtConcurrent::run([item, iconLoader](){
            IconLoaderResult res;
            res.item = item;
            res.icon = iconLoader();
            return res;
        });
        auto watcher = new QFutureWatcher<IconLoaderResult>(this);
        connect(watcher, SIGNAL(finished()), this, SLOT(on_iconLoadedByFuture()));
        connect(watcher, SIGNAL(finished()), watcher, SLOT(deleteLater()));
        watcher->setFuture(future);
    }

    // add children
    for (auto& nameAppPair : app.subApps)
    {
        addApp(nameAppPair.first, nameAppPair.second, item);
    }
}

void AppDialog::on_iconLoadedByFuture()
{
    auto *watcher = (QFutureWatcher<IconLoaderResult>*)sender();
    IconLoaderResult res = watcher->result();
    res.item->setIcon(0, res.icon);
}

void AppDialog::on_filterEdit_textChanged()
{
    QString filter = ui->filterEdit->text();

    if (filter.size() > 0)
    {
        for (QTreeWidgetItem *item : ui->appTree->findItems("", Qt::MatchContains | Qt::MatchRecursive, 0))
        {
            item->setHidden(true);
        }

        for (QTreeWidgetItem *item : ui->appTree->findItems(filter, Qt::MatchContains | Qt::MatchRecursive, 0))
        {
            if (item->text(1).length())// does it have exec?
            {
                item->setHidden(false);

                QTreeWidgetItem *parent = item;
                while ((parent = parent->parent()))
                {
                    parent->setHidden(false);
                }
            }
        }
    }
    else
    {
        for (QTreeWidgetItem *item : ui->appTree->findItems("", Qt::MatchContains | Qt::MatchRecursive, 0))
        {
            item->setHidden(false);
        }
    }

    // expand items if not many
    const int smallNum = 3;
    int numVisibles = 0;
    for (int i=0; i < ui->appTree->topLevelItemCount(); i++)
    {
        QTreeWidgetItem *item = ui->appTree->topLevelItem(i);
        if (!item->isHidden())
        {
            numVisibles++;
        }
    }

    if (numVisibles <= smallNum)
    {
        ui->appTree->expandAll();
    }
    else
    {
        ui->appTree->collapseAll();
    }
    if (ui->appTree->currentItem())
    {
        QTreeWidgetItem *parent = ui->appTree->currentItem();
        while ((parent = parent->parent()))
        {
            ui->appTree->expandItem(ui->appTree->currentItem());
        }
    }
}

void AppDialog::on_appTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous)
{
    ui->buttonBox->button(QDialogButtonBox::StandardButton::Ok)->setEnabled(current != nullptr);
}

void AppDialog::on_appTree_itemDoubleClicked(QTreeWidgetItem *item, int column)
{
    if (ui->appTree->currentItem()->text(1).size())
    {
        ui->appTree->setCurrentItem(item);
        mAppPath = ui->appTree->currentItem()->text(1);
        close();
    }
}

void AppDialog::on_buttonBox_accepted()
{
    mAppPath = ui->appTree->currentItem()->text(1);
    close();
}
void AppDialog::on_buttonBox_rejected()
{
    mAppPath = "";
    close();
}
