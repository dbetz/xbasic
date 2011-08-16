
#include "PortListener.h"
#include <QtDebug>

PortListener::PortListener()
{
    port = NULL;
}

void PortListener::init(const QString & portName, BaudRateType baud)
{
    if(port != NULL) {
         // don't reinitialize port
        if(port->portName() == portName)
            return;
        delete port;
    }
    this->port = new QextSerialPort(portName, QextSerialPort::EventDriven);
    port->setBaudRate(baud);
    port->setFlowControl(FLOW_OFF);
    port->setParity(PAR_NONE);
    port->setDataBits(DATA_8);
    port->setStopBits(STOP_1);
    connect(port, SIGNAL(readyRead()), this, SLOT(onReadyRead())); // never disconnect
}

void PortListener::setDtr(bool enable)
{
    this->port->setDtr(enable);
}

void PortListener::open()
{
    if(!textEditor) // no text editor, no open
        return;

    if(port == NULL)
        return;

    port->open(QIODevice::ReadWrite);
}

void PortListener::close()
{
    if(port == NULL)
        return;

    if(port->isOpen())
        port->close();
}

void PortListener::setTerminalWindow(QPlainTextEdit *editor)
{
    textEditor = editor;
}

void PortListener::send(QByteArray &data)
{
    port->writeData(data.constData(),1);
}

void PortListener::onReadyRead()
{
    const int blen = 1024;
    char buff[blen+1];
    int len = port->bytesAvailable();
    if(len > blen)
        len = blen;
    int ret = port->read(buff, len);
    if(ret > -1) {
        buff[ret] = '\0';
        //QString msg = buff;
        textEditor->setPlainText(textEditor->toPlainText() + buff);
        textEditor->moveCursor(QTextCursor::End);
    }
}

/*
void PortListener::onReadyRead()
{
    QByteArray bytes;
    int len = port->bytesAvailable();
    bytes.resize(len);
    port->read(bytes.data(), bytes.size());
    textEditor->setPlainText(textEditor->toPlainText() + bytes);
    textEditor->moveCursor(QTextCursor::End);
}
*/
void PortListener::onDsrChanged(bool status)
{
    if (status)
        qDebug() << "device was turned on";
    else
        qDebug() << "device was turned off";
}
