#include "console.h"
#include "mainwindow.h"

Console::Console(QWidget *parent) : QPlainTextEdit(parent)
{
}

void Console::keyPressEvent(QKeyEvent *event)
{
    // qDebug() << "keyPressEvent";
    MainWindow *parentMain = (MainWindow *)this->parentWidget()->parentWidget();
    parentMain->keyHandler(event);
}
