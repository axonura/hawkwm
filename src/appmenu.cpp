/*
 * HawkWM Application Menu  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#include "appmenu.h"
#include <QDir>
#include <QFile>
#include <QTextStream>
#include <QProcess>
#include <QVBoxLayout>
#include <QRegularExpression>
#include <QStandardPaths>
#include <QKeyEvent>

HAppMenu::HAppMenu(QWidget *parent) : QDialog(parent) {
    setWindowTitle("HawkWM Application Menu");
    setMinimumSize(420, 520);
    resize(480, 600);

    auto *layout = new QVBoxLayout(this);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText("Search applications...");
    m_filter->setClearButtonEnabled(true);
    layout->addWidget(m_filter);

    m_list = new QListWidget(this);
    layout->addWidget(m_list);

    loadApplications();

    connect(m_list, &QListWidget::itemActivated,
            this, &HAppMenu::launchSelected);
    connect(m_filter, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < m_list->count(); i++) {
            QListWidgetItem *item = m_list->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
    });
}

void HAppMenu::loadApplications() {
    QDir appsDir("/usr/share/applications");
    QStringList files = appsDir.entryList(QStringList("*.desktop"), QDir::Files);

    for (const QString &filePath : files) {
        QFile file(appsDir.filePath(filePath));
        if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
            continue;

        QString name, exec, tryExec;
        bool noDisplay = false;
        bool hidden = false;
        QString type;

        QTextStream in(&file);
        while (!in.atEnd()) {
            QString line = in.readLine().trimmed();
            if (line.startsWith('#') || line.isEmpty())
                continue;

            QString key = line.section('=', 0, 0).trimmed();
            QString val = line.section('=', 1).trimmed();

            if (key == "Type")
                type = val;
            else if (key == "Name" && name.isEmpty())
                name = val;
            else if (key == "Exec" && exec.isEmpty())
                exec = val;
            else if (key == "TryExec")
                tryExec = val;
            else if (key == "NoDisplay")
                noDisplay = (val == "true");
            else if (key == "Hidden")
                hidden = (val == "true");
        }
        file.close();

        if (noDisplay || hidden || type != "Application")
            continue;
        if (name.isEmpty() || exec.isEmpty())
            continue;
        if (!tryExec.isEmpty()) {
            // Skip if the TryExec binary is not in PATH
            QString tryExecPath = QStandardPaths::findExecutable(tryExec);
            if (tryExecPath.isEmpty())
                continue;
        }

        auto *item = new QListWidgetItem(name, m_list);
        item->setData(Qt::UserRole, exec);
    }

    m_list->sortItems();
}

void HAppMenu::launchSelected() {
    QListWidgetItem *item = m_list->currentItem();
    if (!item)
        return;

    QString exec = item->data(Qt::UserRole).toString();
    // Strip field codes: %f, %F, %u, %U, %d, %D, %n, %N, %i, %c, %k, %v, %m
    static const QRegularExpression fieldCodes(R"(%[fFuUdDnNicCkvm%])");
    exec.remove(fieldCodes);
    exec = exec.trimmed();

    if (!exec.isEmpty())
        QProcess::startDetached(exec);

    accept();
}

void HAppMenu::focusSearch() {
    m_filter->setFocus();
    m_filter->selectAll();
}

void HAppMenu::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        reject();
        return;
    }
    if (e->key() == Qt::Key_Return || e->key() == Qt::Key_Enter) {
        launchSelected();
        return;
    }
    QDialog::keyPressEvent(e);
}
