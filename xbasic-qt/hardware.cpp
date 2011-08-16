#include "hardware.h"
#include "ui_hardware.h"
#include "properties.h"
#include "xbasicconfig.h"

Hardware::Hardware(QWidget *parent) : QDialog(parent), ui(new Ui::Hardware)
{
    xBasicConfig = new XBasicConfig();

    ui->setupUi(this);

    connect(ui->comboBoxBoard,SIGNAL(activated(int)),this,SLOT(boardChanged(int)));
    connect(ui->pushButtonDelete,SIGNAL(clicked()),this,SLOT(deleteBoard()));

    ui->comboBoxBoard->setEditable(true);

    ui->tabWidget->setCurrentIndex(0);

    ui->comboBoxClkMode->addItem(tr("RCFAST"));
    ui->comboBoxClkMode->addItem(tr("RCSLOW"));
    ui->comboBoxClkMode->addItem(tr("XINPUT"));
    ui->comboBoxClkMode->addItem(tr("XTAL1+PLL1X"));
    ui->comboBoxClkMode->addItem(tr("XTAL1+PLL2X"));
    ui->comboBoxClkMode->addItem(tr("XTAL1+PLL4X"));
    ui->comboBoxClkMode->addItem(tr("XTAL1+PLL8X"));
    ui->comboBoxClkMode->addItem(tr("XTAL1+PLL16X"));
    ui->comboBoxClkMode->addItem(tr("XTAL2+PLL1X"));
    ui->comboBoxClkMode->addItem(tr("XTAL2+PLL2X"));
    ui->comboBoxClkMode->addItem(tr("XTAL2+PLL4X"));
    ui->comboBoxClkMode->addItem(tr("XTAL2+PLL8X"));
    ui->comboBoxClkMode->addItem(tr("XTAL2+PLL16X"));
    ui->comboBoxClkMode->addItem(tr("XTAL3+PLL1X"));
    ui->comboBoxClkMode->addItem(tr("XTAL3+PLL2X"));
    ui->comboBoxClkMode->addItem(tr("XTAL3+PLL4X"));
    ui->comboBoxClkMode->addItem(tr("XTAL3+PLL8X"));
    ui->comboBoxClkMode->addItem(tr("XTAL3+PLL16X"));

    ui->comboBoxBaudrate->addItem(tr("9600"));
    ui->comboBoxBaudrate->addItem(tr("19200"));
    ui->comboBoxBaudrate->addItem(tr("38400"));
    ui->comboBoxBaudrate->addItem(tr("56000"));
    ui->comboBoxBaudrate->addItem(tr("115200"));

    ui->comboBoxDataMemory->addItem(tr("HUB"));
    ui->comboBoxDataMemory->addItem(tr("RAM"));

    ui->comboBoxCodeMemory->addItem(tr("HUB"));
    ui->comboBoxCodeMemory->addItem(tr("RAM"));
    ui->comboBoxCodeMemory->addItem(tr("FLASH"));

    ui->comboBoxCacheSize->addItem(tr(""));
    ui->comboBoxCacheSize->addItem(tr("2K"));
    ui->comboBoxCacheSize->addItem(tr("4K"));
    ui->comboBoxCacheSize->addItem(tr("8K"));

    ui->comboBoxExternalFlashSize->addItem(tr(""));
    ui->comboBoxExternalFlashSize->addItem(tr("16M"));
    ui->comboBoxExternalFlashSize->addItem(tr("8M"));
    ui->comboBoxExternalFlashSize->addItem(tr("4M"));
    ui->comboBoxExternalFlashSize->addItem(tr("2M"));
    ui->comboBoxExternalFlashSize->addItem(tr("1M"));
    ui->comboBoxExternalFlashSize->addItem(tr("512K"));
    ui->comboBoxExternalFlashSize->addItem(tr("256K"));
    ui->comboBoxExternalFlashSize->setEditable(true);

    ui->comboBoxExternalRamSize->addItem(tr(""));
    ui->comboBoxExternalRamSize->addItem(tr("32M"));
    ui->comboBoxExternalRamSize->addItem(tr("16M"));
    ui->comboBoxExternalRamSize->addItem(tr("8M"));
    ui->comboBoxExternalRamSize->addItem(tr("4M"));
    ui->comboBoxExternalRamSize->addItem(tr("2M"));
    ui->comboBoxExternalRamSize->addItem(tr("1M"));
    ui->comboBoxExternalRamSize->addItem(tr("512K"));
    ui->comboBoxExternalRamSize->addItem(tr("256K"));
    ui->comboBoxExternalRamSize->addItem(tr("64K"));
    ui->comboBoxExternalRamSize->addItem(tr("32K"));
    ui->comboBoxExternalRamSize->setEditable(true);

    ui->lineEditClkFreq->setText(tr("80000000"));
    ui->lineEditRxPin->setText(tr("31"));
    ui->lineEditTxPin->setText(tr("30"));
    ui->lineEditTvPin->setText(tr("12"));
}

Hardware::~Hardware()
{
    delete ui;
}

/*
 * load board from xbasic config file using xBasicConfig
 */
void Hardware::loadBoards()
{
    QSettings settings(publisherKey, xBasicGuiKey, this);
    QVariant sv = settings.value(configFileKey);
    if(sv.canConvert(QVariant::String))
        xBasicCfgFile = sv.toString();
    sv = settings.value(separatorKey);
    if(sv.canConvert(QVariant::String))
        xBasicSeparator = sv.toString();
    if(xBasicConfig->loadBoards(xBasicCfgFile) == 0)
        return;

    /* get board types */
    QStringList boards = xBasicConfig->getBoardNames();
    ui->comboBoxBoard->clear();
    for(int n = 0; n < boards.count(); n++) {
        ui->comboBoxBoard->addItem(boards.at(n));
    }

    /* load first board info */
    XBasicBoard *brd = xBasicConfig->getBoardData(ui->comboBoxBoard->itemText(0));
    setDialogBoardInfo(brd);
}

/*
 * 1. copy old config file to backup
 * 2. save boards to new config file
 * 3. then copy new to old config file
 */
void Hardware::saveBoards()
{
    QString back = +".backup";
    QMessageBox mbox(QMessageBox::Critical,tr("xBasic.cfg File Error"),"", QMessageBox::Ok);
    QFile file;
    if(!file.exists(xBasicCfgFile)) {
        mbox.setInformativeText(tr("Can't find file: ")+xBasicCfgFile);
        mbox.exec();
        return;
    }
    QFile cfg(xBasicCfgFile);
    QFile bup(this->xBasicCfgFile+back);
    if(bup.exists()) bup.remove();
    if(!cfg.copy(this->xBasicCfgFile+back)) {
        mbox.setInformativeText(tr("Can't backup file: ")+xBasicCfgFile);
        mbox.exec();
        return;
    }

    if(cfg.open(QIODevice::WriteOnly))
    {
        QByteArray barry = "";
        QString currentName = ui->comboBoxBoard->currentText().toUpper();
        XBasicBoard *board = xBasicConfig->getBoardByName(currentName);
        if(board == NULL)
            ui->comboBoxBoard->addItem(currentName);
        for(int n = 0; n < ui->comboBoxBoard->count(); n++)
        {
            QString name = ui->comboBoxBoard->itemText(n);
            XBasicBoard *board = xBasicConfig->getBoardByName(name);
            if(board == NULL && name == currentName)
            {
                board = xBasicConfig->newBoard(name);
                setBoardInfo(board);
            }
            QString ba = board->getFormattedConfig();
            qDebug() << ba;
            barry.append(ba);
        }


        QMessageBox sbox(QMessageBox::Question,tr("Saving xBasic.cfg File"),"",
                         QMessageBox::Save | QMessageBox::Cancel);
        sbox.setInformativeText(tr("Save new ")+xBasicCfgFile+tr("?"));
        if(sbox.exec() == QMessageBox::Save) {
            cfg.write(barry);
            cfg.flush();
            cfg.close();
        }
    }
}

void Hardware::accept()
{
    saveBoards();
    done(QDialog::Accepted);
}

void Hardware::reject()
{
    done(QDialog::Rejected);
}

void Hardware::deleteBoard()
{
    xBasicConfig->deleteBoardByName(ui->comboBoxBoard->currentText());
    ui->comboBoxBoard->removeItem(ui->comboBoxBoard->currentIndex());
}

void Hardware::boardChanged(int index)
{
    XBasicBoard *brd = xBasicConfig->getBoardData(ui->comboBoxBoard->itemText(index));
    setDialogBoardInfo(brd);
}

void Hardware::setBoardInfo(XBasicBoard *board)
{
    board->set(XBasicBoard::clkfreq,ui->lineEditClkFreq->text());

    if(ui->lineEditCacheDriver->text().length() > 0)
        board->set(XBasicBoard::cachedriver,ui->lineEditCacheDriver->text());
    if(ui->lineEditCacheParm1->text().length() > 0)
        board->set(XBasicBoard::cacheparam1,ui->lineEditCacheParm1->text());
    if(ui->lineEditCacheParm2->text().length() > 0)
        board->set(XBasicBoard::cacheparam2,ui->lineEditCacheParm2->text());

    board->set(XBasicBoard::rxpin,ui->lineEditRxPin->text());
    board->set(XBasicBoard::txpin,ui->lineEditTxPin->text());

    if(ui->lineEditTvPin->text().length() > 0)
        board->set(XBasicBoard::tvpin,ui->lineEditTvPin->text());

    board->set(XBasicBoard::clkmode, ui->comboBoxClkMode->currentText());
    board->set(XBasicBoard::baudrate, ui->comboBoxBaudrate->currentText());
    board->set(XBasicBoard::textseg, ui->comboBoxCodeMemory->currentText());
    board->set(XBasicBoard::dataseg, ui->comboBoxDataMemory->currentText());
    if(ui->comboBoxCacheSize->currentText().length() > 0)
        board->set(XBasicBoard::cachesize, ui->comboBoxCacheSize->currentText());
    if(ui->comboBoxExternalFlashSize->currentText().length() > 0)
        board->set(XBasicBoard::flashsize, ui->comboBoxExternalFlashSize->currentText());
    if(ui->comboBoxExternalRamSize->currentText().length() > 0)
        board->set(XBasicBoard::ramsize, ui->comboBoxExternalRamSize->currentText());
}

void Hardware::setDialogBoardInfo(XBasicBoard *board)
{
    ui->lineEditClkFreq->setText(board->get(XBasicBoard::clkfreq));
    ui->lineEditRxPin->setText(board->get(XBasicBoard::rxpin));
    ui->lineEditTxPin->setText(board->get(XBasicBoard::txpin));
    ui->lineEditTvPin->setText(board->get(XBasicBoard::tvpin));

    QString cds = board->get(XBasicBoard::cachedriver);
    QString cp1 = board->get(XBasicBoard::cacheparam1);
    QString cp2 = board->get(XBasicBoard::cacheparam2);

    if(cds.length() > 0)
        ui->lineEditCacheDriver->setText(board->get(XBasicBoard::cachedriver));
    if(cp1.length() > 0)
        ui->lineEditCacheParm1->setText(board->get(XBasicBoard::cacheparam1));
    if(cp2.length() > 0)
        ui->lineEditCacheParm2->setText(board->get(XBasicBoard::cacheparam2));

    QString clkmode = board->get(XBasicBoard::clkmode);
    QString baudrate = board->get(XBasicBoard::baudrate);
    QString textseg = board->get(XBasicBoard::textseg);
    QString dataseg = board->get(XBasicBoard::dataseg);
    QString cachesize = board->get(XBasicBoard::cachesize);
    QString flashsize = board->get(XBasicBoard::flashsize);
    QString ramsize = board->get(XBasicBoard::ramsize);

    setComboCurrent(ui->comboBoxClkMode,clkmode);
    setComboCurrent(ui->comboBoxBaudrate,baudrate);
    setComboCurrent(ui->comboBoxCodeMemory,textseg);
    setComboCurrent(ui->comboBoxDataMemory,dataseg);
    setComboCurrent(ui->comboBoxCacheSize,cachesize);
    setComboCurrent(ui->comboBoxExternalFlashSize,flashsize);
    setComboCurrent(ui->comboBoxExternalRamSize,ramsize);
}

void Hardware::setComboCurrent(QComboBox *cb, QString value)
{
    for(int n = cb->count()-1; n > -1; n--) {
        QString s = cb->itemText(n);
        if(s == value) {
            cb->setCurrentIndex(n);
            break;
        }
    }
}
