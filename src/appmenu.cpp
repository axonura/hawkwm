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
#include <QTimer>

HAppMenu::HAppMenu(const QString &socketName, QWidget *parent) : QWidget(parent), m_socketName(socketName) {
    setFixedSize(420, 520);

    auto *layout = new QVBoxLayout(this);

    m_filter = new QLineEdit(this);
    m_filter->setPlaceholderText("Search applications...");
    m_filter->setClearButtonEnabled(true);
    layout->addWidget(m_filter);

    m_list = new QListWidget(this);
    layout->addWidget(m_list);

    QTimer::singleShot(0, this, &HAppMenu::loadApplications);

    connect(m_list, &QListWidget::itemActivated,
            this, &HAppMenu::launchSelected);
    connect(m_list, &QListWidget::itemClicked,
            this, &HAppMenu::launchSelected);
    connect(m_filter, &QLineEdit::textChanged, this, [this](const QString &text) {
        for (int i = 0; i < m_list->count(); i++) {
            QListWidgetItem *item = m_list->item(i);
            item->setHidden(!item->text().contains(text, Qt::CaseInsensitive));
        }
        for (int i = 0; i < m_list->count(); i++) {
            QListWidgetItem *item = m_list->item(i);
            if (!item->isHidden()) {
                m_list->setCurrentItem(item);
                break;
            }
        }
        emit contentChanged();
    });
    connect(m_filter, &QLineEdit::returnPressed, this, &HAppMenu::launchSelected);
    connect(m_list, &QListWidget::currentRowChanged, this, &HAppMenu::contentChanged);

    m_filter->installEventFilter(this);
    m_list->installEventFilter(this);
}

HAppMenu::~HAppMenu() {}

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
    if (!item) {
        for (int i = 0; i < m_list->count(); ++i) {
            QListWidgetItem *candidate = m_list->item(i);
            if (!candidate->isHidden()) {
                item = candidate;
                break;
            }
        }
    }
    if (!item)
        return;

    QString exec = item->data(Qt::UserRole).toString();
    // Strip field codes: %f, %F, %u, %U, %d, %D, %n, %N, %i, %c, %k, %v, %m
    static const QRegularExpression fieldCodes(R"(%[fFuUdDnNicCkvm%])");
    exec.remove(fieldCodes);
    exec = exec.trimmed();

    if (!exec.isEmpty()) {
        m_process = new QProcess(this);
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert("WAYLAND_DISPLAY", m_socketName);
        m_process->setProcessEnvironment(env);
        m_process->setProcessChannelMode(QProcess::ForwardedChannels);
        connect(m_process, &QProcess::finished, this, [this]() {
            m_process->deleteLater();
            m_process = nullptr;
        });
        m_process->setProgram("/bin/sh");
        m_process->setArguments({"-c", exec});
        m_process->start();
    }

    hide();
    emit closed();
}

void HAppMenu::focusSearch() {
    m_filter->setFocus();
    m_filter->selectAll();
}

bool HAppMenu::eventFilter(QObject *obj, QEvent *event) {
    if (event->type() == QEvent::KeyPress) {
        auto *key = static_cast<QKeyEvent*>(event);
        if (obj == m_filter) {
            if (key->key() == Qt::Key_Down) {
                m_list->setFocus();
                if (m_list->currentItem() == nullptr) {
                    for (int i = 0; i < m_list->count(); ++i) {
                        QListWidgetItem *item = m_list->item(i);
                        if (!item->isHidden()) {
                            m_list->setCurrentItem(item);
                            break;
                        }
                    }
                }
                return true;
            }
            if (key->key() == Qt::Key_Up && m_list->currentItem()) {
                int row = m_list->row(m_list->currentItem()) - 1;
                while (row >= 0) {
                    QListWidgetItem *item = m_list->item(row);
                    if (!item->isHidden()) {
                        m_list->setCurrentItem(item);
                        return true;
                    }
                    --row;
                }
            }
        }
        if (obj == m_list) {
            if (key->key() == Qt::Key_Up || key->key() == Qt::Key_Down) {
                int row = m_list->currentRow();
                int step = (key->key() == Qt::Key_Down) ? 1 : -1;
                row += step;
                while (row >= 0 && row < m_list->count()) {
                    QListWidgetItem *item = m_list->item(row);
                    if (!item->isHidden()) {
                        m_list->setCurrentItem(item);
                        return true;
                    }
                    row += step;
                }
                return true;
            }
            if (key->key() == Qt::Key_Left || key->key() == Qt::Key_Backtab ||
                (key->key() == Qt::Key_Tab && (key->modifiers() & Qt::ShiftModifier))) {
                m_filter->setFocus();
                return true;
            }
            if (key->key() == Qt::Key_Tab) {
                m_filter->setFocus();
                return true;
            }
        }
    }
    return QWidget::eventFilter(obj, event);
}

void HAppMenu::keyPressEvent(QKeyEvent *e) {
    if (e->key() == Qt::Key_Escape) {
        e->accept();
        hide();
        emit closed();
        return;
    }
    QWidget::keyPressEvent(e);
}
