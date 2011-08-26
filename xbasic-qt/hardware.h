#ifndef HARDWARE_H
#define HARDWARE_H

#include <QDialog>

#include "xbasicconfig.h"

namespace Ui {
    class Hardware;
}

class Hardware : public QDialog
{
    Q_OBJECT

public:
    explicit Hardware(QWidget *parent = 0);
    ~Hardware();

    void loadBoards();
    void saveBoards();

public slots:
    void accept();
    void reject();
    void boardChanged(int index);
    void deleteBoard();

private:
    void setBoardInfo(XBasicBoard *board);
    void setDialogBoardInfo(XBasicBoard *board);
    void setComboCurrent(QComboBox *cb, QString value);

    Ui::Hardware *ui;

    XBasicConfig    *xBasicConfig;
    QString         xBasicCfgFile;
    QString         xBasicSeparator;
};

#endif // HARDWARE_H
