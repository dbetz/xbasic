#ifndef XBASICCONFIG_H
#define XBASICCONFIG_H

#include <QtGui>
#include "xbasicboard.h"

class XBasicConfig
{
public:
    XBasicConfig();
    ~XBasicConfig();

    int         loadBoards(QString filePath);
    XBasicBoard *newBoard(QString name);
    void        deleteBoardByName(QString name);

    QStringList getBoardNames();
    XBasicBoard *getBoardByName(QString name);
    XBasicBoard *getBoardData(QString name);


private:
    QString             filePath;
    QStringList         boardNames;
    QList<XBasicBoard*> *boards;
};

#endif // XBASICCONFIG_H
