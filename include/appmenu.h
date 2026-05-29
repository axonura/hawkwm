/*
 * HawkWM Application Menu  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#ifndef APPMENU_H
#define APPMENU_H

#include <QWidget>
#include <QLineEdit>
#include <QListWidget>
#include <QKeyEvent>
#include <QProcess>

class HAppMenu : public QWidget {
    Q_OBJECT
public:
    explicit HAppMenu(const QString &socketName, QWidget *parent = nullptr);
    ~HAppMenu();
    void focusSearch();

signals:
    void contentChanged();
    void closed();

protected:
    void keyPressEvent(QKeyEvent *e) override;
    bool eventFilter(QObject *obj, QEvent *event) override;

private:
    QListWidget *m_list;
    QLineEdit *m_filter;
    QProcess *m_process = nullptr;
    QString m_socketName;

    void loadApplications();
    void launchSelected();
};

#endif // APPMENU_H
