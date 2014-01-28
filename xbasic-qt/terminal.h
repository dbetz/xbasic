#ifndef TERMINAL_H
#define TERMINAL_H

#include <QtGui>
#include "console.h"
#include "PortListener.h"

class Terminal : public QDialog
{
    Q_OBJECT
public:
    explicit Terminal(QWidget *parent);
    Console *getEditor();
    void setPortListener(PortListener *listener);
    QString getPortName();
    void setPortName(QString name);
    void setPosition(int x, int y);
    void accept();
    void reject();

public slots:
    void clearScreen();

private:
    PortListener *portListener;
    QLabel  portLabel;
    Console *termEditor;
};

#endif // TERMINAL_H
