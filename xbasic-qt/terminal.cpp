#include "terminal.h"

Terminal::Terminal(QWidget *parent) : QDialog(parent)
{
    QVBoxLayout *termLayout = new QVBoxLayout(this);
    termEditor = new Console(parent);
    termEditor->setReadOnly(false);
    //connect(this, SIGNAL(accepted()), this, SLOT(keyPressEvent(QKeyEvent *)));
    termLayout->addWidget(termEditor);
    QPushButton *cls = new QPushButton(tr("Clear"),this);
    connect(cls,SIGNAL(clicked()), this, SLOT(clearScreen()));
    QDialogButtonBox *buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok);
    connect(buttonBox, SIGNAL(accepted()), this, SLOT(accept()));
    connect(buttonBox, SIGNAL(rejected()), this, SLOT(reject()));
    QHBoxLayout *butLayout = new QHBoxLayout();
    termLayout->addLayout(butLayout);
    butLayout->addWidget(cls);
    butLayout->addWidget(&portLabel);
    butLayout->addWidget(buttonBox);
    setLayout(termLayout);
    this->setWindowFlags(Qt::Tool);
    portListener = NULL;
    resize(700,500);
}

Console *Terminal::getEditor()
{
    return termEditor;
}

void Terminal::setPortListener(PortListener *listener)
{
    portListener = listener;
    if(listener->port) {
        if(listener->port->portName().isEmpty() == false)
            portLabel.setText(listener->port->portName());
    }
    else {
        portLabel.setText("");
    }
}

QString Terminal::getPortName()
{
    return portLabel.text();
}

void Terminal::setPortName(QString name)
{
    portLabel.setText(name);
}

void Terminal::setPosition(int x, int y)
{
    int width = this->width();
    int height = this->height();
    this->setGeometry(x,y,width,height);
}

void Terminal::accept()
{
/*
    QString text = termEditor->toPlainText();
    QStringList list = text.split("\n");
*/
    done(QDialog::Accepted);
}

void Terminal::reject()
{
    done(QDialog::Rejected);
}

void Terminal::clearScreen()
{
    termEditor->setPlainText("");
}
