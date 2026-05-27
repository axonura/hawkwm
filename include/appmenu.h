/*
 * HawkWM Application Menu  Copyright (C) 2026  First Person
 * This program comes with ABSOLUTELY NO WARRANTY; for details type `show w'.
 * This is free software, and you are welcome to redistribute it
 * under certain conditions; type `show c' for details.
 */

#ifndef APPMENU_H
#define APPMENU_H

#include <QDialog>
#include <QLineEdit>
#include <QListWidget>
#include <QKeyEvent>

class HAppMenu : public QDialog {
    Q_OBJECT
public:
    explicit HAppMenu(QWidget *parent = nullptr);
    ~HAppMenu() = default;
    void focusSearch();

protected:
    void keyPressEvent(QKeyEvent *e) override;

private:
    QListWidget *m_list;
    QLineEdit *m_filter;

    void loadApplications();
    void launchSelected();
};

#endif // APPMENU_H
