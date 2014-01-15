


#ifndef PORTLISTENER_H_
#define PORTLISTENER_H_

#include <QtGui>
#include <QObject>
#include "qextserialport.h"

class PortListener : public QObject
{
Q_OBJECT
public:
    PortListener();
    void init(const QString & portName, BaudRateType baud);
    void setDtr(bool enable);
    bool open();
    void close();
    void setTerminalWindow(QPlainTextEdit *editor);
    void send(QByteArray &data);

    QextSerialPort  *port;

private:
    QPlainTextEdit  *textEditor;

private slots:
    void onReadyRead();
    void onDsrChanged(bool status);

};


#endif /*PORTLISTENER_H_*/
