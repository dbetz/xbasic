#include "xbasicconfig.h"

XBasicConfig::XBasicConfig()
{
    boards = new QList<XBasicBoard*>();
}

XBasicConfig::~XBasicConfig()
{
    boards->clear();
    delete boards;
}

void XBasicConfig::deleteBoardByName(QString name)
{
    for(int n = 0; n < boardNames.count(); n++)
    {
        if(boardNames.at(n) == name)
        {
            boardNames.removeAt(n);
            boards->removeAt(n);
            break;
        }
    }
}

/*
 * This doesn't scale, but the number of boards is small for now.
 * Can easily make boards a QHash later.
 */
XBasicBoard *XBasicConfig::getBoardByName(QString name)
{
    for(int n = 0; n < boardNames.count(); n++)
    {
        if(boardNames.at(n) == name)
            return boards->at(n);
    }
    return NULL;
}

XBasicBoard *XBasicConfig::newBoard(QString name)
{
    boardNames.append(name);
    XBasicBoard *board = new XBasicBoard();
    board->setBoardName(name);
    return board;
}

int XBasicConfig::loadBoards(QString filePath)
{
    QFile fileTest;
    if(!fileTest.exists(filePath))
        return 0;

    QFile fileReader(filePath);
    if (!fileReader.open(QIODevice::ReadOnly))
        return 0;

    /* clear old data */
    for(int n = boards->count()-1; n > -1; n--)
    {
        XBasicBoard *brd = boards->at(n);
        delete brd;
    }
    boards->clear();
    boardNames.clear();

    QString file = fileReader.readAll();

    QString bstr;
    QStringList barr = file.split("[");

    for (int n = 0; n < barr.count(); n++)
    {
        bstr = barr.at(n);
        if (bstr.length() == 0)
            continue;

        //XBasicBoard *board = new XBasicBoard();

        // get board name
        bstr = barr.at(n).mid(0, barr.at(n).indexOf(']'));
        bstr = bstr.trimmed().toUpper();
        XBasicBoard *board = newBoard(bstr);

        // the rest of the board definition parsed here
        bstr = barr.at(n);
        bstr = barr.at(n).mid(barr.at(n).indexOf(']')+1);
        bstr = bstr.trimmed();

        // parse the rest in Board class
        if (board->parseConfig(bstr))
            boards->append(board);
    }
    fileReader.close();
    return boards->count();
}

QStringList XBasicConfig::getBoardNames()
{
    return boardNames;
}

XBasicBoard* XBasicConfig::getBoardData(QString name)
{
    int length = boards->count();
    if(length == 0)
        return NULL;
    for(int n = 0; n < length; n++)
    {
        if(boards->at(n)->getBoardName() == name)
            return boards->at(n);
    }
    return NULL;
}
