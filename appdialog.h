#ifndef APPDIALOG_H
#define APPDIALOG_H

#include <QDialog>
#include <QTreeWidgetItem>
#include "Utilities.h"

namespace Ui {
class AppDialog;
}

class AppDialog : public QDialog
{
    Q_OBJECT

public:
    explicit AppDialog(QWidget *parent = nullptr);
    ~AppDialog();

    QString appPath() const;

private:
    Ui::AppDialog *ui;
    QString mAppPath;

    void addApp(const QString &name, const AppDef& app, QTreeWidgetItem *parent);

private slots:
    void on_filterEdit_textChanged();
    void on_appTree_itemDoubleClicked(QTreeWidgetItem *item, int column);
    void on_appTree_currentItemChanged(QTreeWidgetItem *current, QTreeWidgetItem *previous);
    void on_buttonBox_accepted();
    void on_buttonBox_rejected();
};

#endif // APPDIALOG_H
