

#include "mainwindow.h"
#include "qextserialenumerator.h"

MainWindow::MainWindow(QWidget *parent) : QMainWindow(parent)
{
    /* setup application registry info */
    QCoreApplication::setOrganizationName(publisherKey);
    QCoreApplication::setOrganizationDomain(publisherComKey);
    QCoreApplication::setApplicationName(xBasicGuiKey);

    /* global settings */
    settings = new QSettings(publisherKey, xBasicGuiKey, this);

    /* setup properties dialog */
    propDialog = new Properties(this);
    connect(propDialog,SIGNAL(accepted()),this,SLOT(propertiesAccepted()));

    /* new xBasicConfig class */
    xBasicConfig = new XBasicConfig();

    projectModel = NULL;
    referenceModel = NULL;

    /* setup gui components */
    setupFileMenu();
    setupHelpMenu();
    setupToolBars();

    /* main container */
    setWindowTitle(tr("xBasic"));
    QSplitter *vsplit = new QSplitter(this);
    setCentralWidget(vsplit);

    /* minimum window height */
    this->setMinimumHeight(500);

    /* project tools */
    setupProjectTools(vsplit);

    /* start with an empty file if fresh install */
    newFile();

    /* status bar ... do we need this? how about a progressbar?
    QStatusBar *statusBar = new QStatusBar(this);
    this->setStatusBar(statusBar);
    */

    /* get app settings at startup and before any compiler call */
    getApplicationSettings();

    initBoardTypes();

    /* setup the port listener */
    portListener = new PortListener();

    /* get available ports at startup */
    enumeratePorts();

    /* these are read once per app startup */
    QVariant lastportv  = settings->value(lastPortNameKey);
    if(lastportv.canConvert(QVariant::String))
        portName = lastportv.toString();

    /* setup the first port displayed in the combo box */
    if(cbPort->count() > 0) {
        int ndx = 0;
        if(portName.length() != 0) {
            for(int n = cbPort->count()-1; n > -1; n--)
                if(cbPort->itemText(n) == portName)
                {
                    ndx = n;
                    break;
                }
        }
        setCurrentPort(ndx);
    }

    /* setup the terminal dialog box */
    term = new Terminal(this);
    connect(term,SIGNAL(accepted()),this,SLOT(terminalClosed()));
    connect(term,SIGNAL(rejected()),this,SLOT(terminalClosed()));

    /* tell port listener to use terminal editor for i/o */
    termEditor = term->getEditor();
    portListener->setTerminalWindow(termEditor);

    /* load the last file into the editor to make user happy */
    QVariant lastfilev = settings->value(lastFileNameKey);
    if(!lastfilev.isNull()) {
        if(lastfilev.canConvert(QVariant::String)) {
            QString fileName = lastfilev.toString();
            openFileName(fileName);
        }
    }

    hardwareDialog = new Hardware(this);
    connect(hardwareDialog,SIGNAL(accepted()),this,SLOT(initBoardTypes()));
}

void MainWindow::keyHandler(QKeyEvent* event)
{
    //qDebug() << "MainWindow::keyHandler";
    int key = event->key();
    switch(key)
    {
    case Qt::Key_Enter:
    case Qt::Key_Return:
        key = '\n';
        break;
    case Qt::Key_Backspace:
        key = '\b';
        break;
    }
    if(key & Qt::Key_Escape)
        return;
    QByteArray barry;
    barry.append((char)key);
    portListener->send(barry);
}

void MainWindow::terminalEditorTextChanged()
{
    QString text = termEditor->toPlainText();
}

/*
 * get the application settings from the registry for compile/startup
 */
void MainWindow::getApplicationSettings()
{
    QFile file;
    QVariant compv = settings->value(compilerKey);

    if(compv.canConvert(QVariant::String))
        xBasicCompiler = compv.toString();

    if(!file.exists(xBasicCompiler))
    {
        propDialog->show();
    }

    /* get the separator used at startup */
    QString appPath = QCoreApplication::applicationDirPath ();
    if(appPath.indexOf('\\') > -1)
        xBasicSeparator = "\\";
    else
        xBasicSeparator = "/";

    /* get the compiler path */
    if(xBasicCompiler.indexOf('\\') > -1) {
        xBasicCompilerPath = xBasicCompiler.mid(0,xBasicCompiler.lastIndexOf('\\')+1);
    }
    else if(xBasicCompiler.indexOf('/') > -1) {
        xBasicCompilerPath = xBasicCompiler.mid(0,xBasicCompiler.lastIndexOf('/')+1);
    }

    /* get the include path and config file set by user */
    QVariant incv = settings->value(includesKey);
    QVariant cfgv = settings->value(configFileKey);

    /* convert registry values to strings */
    if(incv.canConvert(QVariant::String))
        xBasicIncludes = incv.toString();

    if(cfgv.canConvert(QVariant::String))
        xBasicCfgFile = cfgv.toString();

    if(!file.exists(xBasicCfgFile))
    {
        propDialog->show();
    }
    else
    {
        /* load boards in case there were changes */
        xBasicConfig->loadBoards(xBasicCfgFile);
    }
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    portListener->close();

    QString filestr = editorTabs->tabToolTip(editorTabs->currentIndex());
    QString boardstr = cbBoard->itemText(cbBoard->currentIndex());
    QString portstr = cbPort->itemText(cbPort->currentIndex());

    settings->setValue(lastFileNameKey,filestr);
    settings->setValue(lastBoardNameKey,boardstr);
    settings->setValue(lastPortNameKey,portstr);
}

void MainWindow::newFile()
{
    setupEditor();
    editorTabs->addTab(editors->at(editors->count()-1),(const QString&)untitledstr);
}

void MainWindow::openFile(const QString &path)
{
    QString fileName = path;

    if (fileName.isNull())
        fileName = QFileDialog::getOpenFileName(this,
            tr("Open File"), "", "xBasic Files (*.bas)");
    openFileName(fileName);
}

void MainWindow::openFileName(QString fileName)
{
    QString data;
    if (!fileName.isEmpty()) {
        QFile file(fileName);
//        if (file.open(QFile::ReadOnly | QFile::Text))
        if (file.open(QFile::ReadOnly))
        {
            data = file.readAll();
            QString sname = this->shortFileName(fileName);
            if(editorTabs->count()>0) {
                for(int n = editorTabs->count()-1; n > -1; n--) {
                    if(editorTabs->tabText(n) == sname) {
                        setEditorTab(n, sname, fileName, data);
                        file.close();
                        return;
                    }
                }
            }
            if(editorTabs->tabText(0) == untitledstr) {
                setEditorTab(0, sname, fileName, data);
                file.close();
                return;
            }
            newFile();
            setEditorTab(editorTabs->count()-1, sname, fileName, data);
            file.close();
        }
    }
}


void MainWindow::saveFile(const QString &path)
{
    try {
        int n = this->editorTabs->currentIndex();
        QString fileName = editorTabs->tabToolTip(n);
        QString data = editors->at(n)->toPlainText();

        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QFile::WriteOnly)) {
                file.write(data.toAscii());
                file.close();
            }
        }
    } catch(...) {
    }
}

void MainWindow::saveAsFile(const QString &path)
{
    try {
        int n = this->editorTabs->currentIndex();
        //QString fileName = editors->at(n)->toolTip();
        QString data = editors->at(n)->toPlainText();
        QString fileName = path;
        //QString data = editor->toPlainText();

        if (fileName.isNull())
            fileName = QFileDialog::getSaveFileName(this,
                tr("Save As File"), "", "xBasic Files (*.bas)");

        this->editorTabs->setTabText(n,shortFileName(fileName));
        updateProjectTree(fileName,data);
        editorTabs->setTabToolTip(n,fileName);

        if (!fileName.isEmpty()) {
            QFile file(fileName);
            if (file.open(QFile::WriteOnly)) {
                file.write(data.toAscii());
                file.close();
            }
        }
    } catch(...) {
    }
}

void MainWindow::printFile(const QString &path)
{
    /*
    QString fileName = path;
    QString data = editor->toPlainText();

    if (!fileName.isEmpty()) {
    }
    */
}

void MainWindow::zipFile(const QString &path)
{
    /*
    QString fileName = path;
    QString data = editor->toPlainText();

    if (!fileName.isEmpty()) {
    }
    */
}

void MainWindow::hardware()
{
    hardwareDialog->loadBoards();
    hardwareDialog->show();
}

void MainWindow::properties()
{
    propDialog->show();
}
void MainWindow::propertiesAccepted()
{
    getApplicationSettings();
    initBoardTypes();
}

void MainWindow::setCurrentBoard(int index)
{
    boardName = cbBoard->itemText(index);
    cbBoard->setCurrentIndex(index);
}

void MainWindow::setCurrentPort(int index)
{
    portName = cbPort->itemText(index);
    cbPort->setCurrentIndex(index);
    if(portName.length()) {
        portListener->init(portName, BAUD115200);  // signals get hooked up internally
    }
}

int  MainWindow::checkCompilerInfo()
{
    QMessageBox mbox(QMessageBox::Critical,tr("Build Error"),"",QMessageBox::Ok);
    if(xBasicCompiler.length() == 0) {
        mbox.setInformativeText(tr("Please specify compiler application in properties."));
        mbox.exec();
        return -1;
    }
    if(xBasicIncludes.length() == 0) {
        mbox.setInformativeText(tr("Please specify include path in properties."));
        mbox.exec();
        return -1;
    }
    return 0;
}

QStringList MainWindow::getCompilerParameters(QString copts)
{
    QString srcpath = this->editorTabs->tabToolTip(this->editorTabs->currentIndex());
    srcpath = QDir::fromNativeSeparators(srcpath);
    srcpath = srcpath.mid(0,srcpath.lastIndexOf(xBasicSeparator)+1);

    portName = cbPort->itemText(cbPort->currentIndex());    // TODO should be itemToolTip
    boardName = cbBoard->itemText(cbBoard->currentIndex());

    QStringList args;
    args.append(tr("-b"));
    args.append(boardName);
    args.append(tr("-p"));
    args.append(portName);
    args.append(tr("-I"));
    args.append(xBasicIncludes);
    args.append(tr("-I"));
    args.append(srcpath);
    args.append(this->editorTabs->tabToolTip(this->editorTabs->currentIndex()));
    args.append(copts);

    qDebug() << args;
    return args;
}

int  MainWindow::runCompiler(QString copts)
{
    getApplicationSettings();
    if(checkCompilerInfo()) {
        return -1;
    }

    QStringList args = getCompilerParameters(copts);

    btnConnected->setChecked(false);
    connectButton();            // disconnect uart before use

    QMessageBox mbox;
    mbox.setStandardButtons(QMessageBox::Ok);

    QProcess proc(this);

    /* can use these signals, but can't collect error info in them.
    connect(&proc,SIGNAL(error(QProcess::ProcessError)),this,
            SLOT(compilerError(QProcess::ProcessError)));
    connect(&proc,SIGNAL(finished(int,QProcess::ExitStatus)),this,
            SLOT(compilerFinished(int,QProcess::ExitStatus)));
    */
    proc.setWorkingDirectory(xBasicCompilerPath);

    proc.start(xBasicCompiler,args);

    if(!proc.waitForStarted()) {
        mbox.setInformativeText(tr("Could not start compiler."));
        mbox.exec();
        return -1;
    }

    if(!proc.waitForFinished()) {
        mbox.setInformativeText(tr("Error waiting for compiler to finish."));
        mbox.exec();
        return -1;
    }

    QString result = proc.readAll();
    int exitCode = proc.exitCode();
    int exitStatus = proc.exitStatus();

    mbox.setInformativeText(result);

    if(exitStatus == QProcess::CrashExit)
    {
        mbox.setText(tr("xBasic Compiler Crashed"));
        mbox.exec();
        return -1;
    }
    if(exitCode != 0)
    {
        mbox.setText(tr("xBasic Compiler Error"));
        mbox.exec();
        return -1;
    }
    if(result.indexOf("error") > -1)
    { // just in case we get an error without exitCode
        mbox.setText(tr("xBasic Compiler Error"));
        mbox.exec();
        return -1;
    }

    return 0;
}

void MainWindow::compilerError(QProcess::ProcessError error)
{
    qDebug() << error;
}

void MainWindow::compilerFinished(int exitCode, QProcess::ExitStatus status)
{
    qDebug() << exitCode << status;
}

void MainWindow::programBuild()
{
    // must have something in the compiler options parameter
    runCompiler("-v");
}

void MainWindow::programBurnEE()
{
    runCompiler("-e");
}

void MainWindow::programRun()
{
    runCompiler("-r");
}

void MainWindow::programDebug()
{
    if(runCompiler("-r"))
        return; // don't start terminal if compile failed
    /*
     * setting the position of a new dialog doesn't work very nice
     * Term dialog will not close/reopen on debug so it doesn't matter.
     */
    term->show();   // show if hidden

    btnConnected->setChecked(true);
    connectButton();
}

void MainWindow::terminalClosed()
{
    btnConnected->setChecked(false);
    connectButton();
}

void MainWindow::setupHelpMenu()
{
    QMenu *helpMenu = new QMenu(tr("&Help"), this);
    menuBar()->addMenu(helpMenu);

    helpMenu->addAction(QIcon(":/images/helphint.png"), tr("&About"), this, SLOT(about()));
    helpMenu->addAction(QIcon(":/images/helpsymbol.png"), tr("Language Help"), this, SLOT(languageHelp()));
}

void MainWindow::about()
{
    QMessageBox::about(this, tr("About xBasic"),
                tr("<p><b>xBasic</b> allows running basic programs from " \
                   "Propeller HUB or user defined external memory.</p>"));
}

void MainWindow::languageHelp()
{
    QMessageBox::about(this, tr("Language Help"),
                tr("<p><b>xBasic</b> language help ... TBD ..." \
                   " </p>"));
}

void MainWindow::projectTreeClicked(QModelIndex index)
{
    QVariant vs = projectModel->data(index, Qt::DisplayRole);
    if(vs.canConvert(QVariant::String))
    {
        QString fileName = vs.toString();
        QString userFile = editorTabs->tabToolTip(editorTabs->currentIndex());
        QString userPath = userFile.mid(0,userFile.lastIndexOf(xBasicSeparator)+1);
        QFile file;
        if(file.exists(xBasicIncludes+fileName))
            openFileName(xBasicIncludes+fileName);
        else if(file.exists(userPath+fileName))
            openFileName(userPath+fileName);
    }
}

void MainWindow::referenceTreeClicked(QModelIndex index)
{
    QVariant vs = referenceModel->data(index, Qt::DisplayRole);
    if(vs.canConvert(QVariant::String))
    {
        QString method = vs.toString();
        QString myFile = referenceModel->file(index);
        QString text = 0;

        QFile file;
        if(file.exists(myFile))
        {
            QFile readfile(myFile);
            if (readfile.open(QFile::ReadOnly | QFile::Text))
            {
                text = readfile.readAll();
            }
        }

        if(text != 0)
        {
            QStringList lines = text.split("\n");
            method = method.trimmed();
            int position = 0;
            for(int n = 0; n < lines.length(); n++)
            {
                QString line = lines.at(n);
                position += line.length();
                if(line.indexOf(method) > -1)
                {
                    /* open file in tab if not there already */
                    openFileName(myFile);

                    QPlainTextEdit *editor = editors->at(editorTabs->currentIndex());
                    if(editor != NULL)
                    {
                        /* highlight method - yes, it will be ther */
                        editor->find(method);
                        QTextCursor cur = editor->textCursor();
                        cur.movePosition(QTextCursor::Start);
                        cur.movePosition(QTextCursor::Down,QTextCursor::KeepAnchor,n);

                        /* this part is a little troublesome */
                        QScrollBar *sb = editor->verticalScrollBar();
                        int sbval = sb->value();
                        int sbpage = sb->pageStep();
                        if(sbval > (sbpage*9)/10)
                        {
                            int step = (sb->pageStep()*4)/8;
                            sb->setValue(sb->value()+step);
                        }

                    }
                    return;
                }
            }
        }
    }
}

void MainWindow::closeTab(int index)
{
    editors->at(index)->setPlainText("");
    editors->remove(index);
    if(editorTabs->count() == 1)
        newFile();
    editorTabs->removeTab(index);
}

void MainWindow::changeTab(int index)
{
    QString fileName = editorTabs->tabToolTip(index);
    QString text = editors->at(index)->toPlainText();

    if(fileName.length() != 0 && text.length() != 0)
    {
        updateProjectTree(fileName,text);
        updateReferenceTree(fileName,text);
    }
}

void MainWindow::addToolButton(QToolBar *bar, QToolButton *btn, QString imgfile)
{
    const QSize buttonSize(24, 24);
    btn->setIcon(QIcon(QPixmap(imgfile.toAscii())));
    btn->setMinimumSize(buttonSize);
    btn->setMaximumSize(buttonSize);
    btn->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    bar->addWidget(btn);
}

void MainWindow::updateProjectTree(QString &fileName, QString &text)
{
    QString s = this->shortFileName(fileName);
    basicPath = fileName.mid(0,fileName.lastIndexOf('/')+1);

    if(projectModel != NULL) delete projectModel;
    projectModel = new TreeModel(s, this);
    projectModel->xBasicIncludes(text);

    projectTree->setWindowTitle(s);
    projectTree->setModel(projectModel);
    projectTree->show();

}

void MainWindow::updateReferenceTree(QString &fileName, QString &text)
{
    QString s = this->shortFileName(fileName);
    basicPath = fileName.mid(0,fileName.lastIndexOf('/')+1);

    if(referenceModel != NULL) delete referenceModel;
    referenceModel = new TreeModel(s, this);
    referenceModel->addFileReferences(fileName,this->xBasicIncludes, this->xBasicSeparator, text, true);

    referenceTree->setWindowTitle(s);
    referenceTree->setModel(referenceModel);
    referenceTree->show();

}

void MainWindow::setEditorTab(int num, QString &shortName, QString &fileName, QString &text)
{
    QPlainTextEdit *editor = editors->at(num);
    editor->setPlainText(text);
    editorTabs->setTabText(num,shortName);
    editorTabs->setTabToolTip(num,fileName);
    updateProjectTree(fileName,text);
    updateReferenceTree(fileName,text);
    editorTabs->setCurrentIndex(num);
}

void MainWindow::enumeratePorts()
{
    if(cbPort != NULL) cbPort->clear();
    QList<QextPortInfo> ports = QextSerialEnumerator::getPorts();
    QStringList stringlist;
    QString name;
    stringlist << "List of ports:";
    for (int i = 0; i < ports.size(); i++) {
        stringlist << "port name:" << ports.at(i).portName;
        stringlist << "friendly name:" << ports.at(i).friendName;
        stringlist << "physical name:" << ports.at(i).physName;
        stringlist << "enumerator name:" << ports.at(i).enumName;
        stringlist << "vendor ID:" << QString::number(ports.at(i).vendorID, 16);
        stringlist << "product ID:" << QString::number(ports.at(i).productID, 16);
        stringlist << "===================================";
#if defined(Q_WS_WIN32)
        name = ports.at(i).portName;
        cbPort->addItem(name);
#else
        name = "/"+ports.at(i).physName;
        cbPort->addItem(name);
#endif
    }
}

void MainWindow::connectButton()
{
    if(btnConnected->isChecked()) {
        btnConnected->setDisabled(true);
        portListener->open();
        btnConnected->setDisabled(false);
        term->show();
    }
    else {
        portListener->close();
    }
}

QString MainWindow::shortFileName(QString &fileName)
{
    return fileName.mid(fileName.lastIndexOf(xBasicSeparator)+1);
}

void MainWindow::initBoardTypes()
{
    cbBoard->clear();

    QFile file;
    if(file.exists(xBasicCfgFile))
    {
        /* load boards in case there were changes */
        xBasicConfig->loadBoards(xBasicCfgFile);
    }

    /* get board types */
    QStringList boards = xBasicConfig->getBoardNames();
    for(int n = 0; n < boards.count(); n++)
        cbBoard->addItem(boards.at(n));

    QVariant lastboardv = settings->value(lastBoardNameKey);
    /* read last board/port to make user happy */
    if(lastboardv.canConvert(QVariant::String))
        boardName = lastboardv.toString();

    /* setup the first board displayed in the combo box */
    if(cbBoard->count() > 0) {
        int ndx = 0;
        if(boardName.length() != 0) {
            for(int n = cbBoard->count()-1; n > -1; n--)
                if(cbBoard->itemText(n) == boardName)
                {
                    ndx = n;
                    break;
                }
        }
        setCurrentBoard(ndx);
    }
}


void MainWindow::setupEditor()
{
    //editor->clear();
    QFont font;
    font.setFamily("Courier");
    font.setFixedPitch(true);
    font.setPointSize(10);

    QPlainTextEdit *editor = new QPlainTextEdit;
    editor->setFont(font);

    highlighter = new Highlighter(editor->document());
    editors->append(editor);
}

void MainWindow::setupFileMenu()
{
    QMenu *fileMenu = new QMenu(tr("&File"), this);
    menuBar()->addMenu(fileMenu);

    fileMenu->addAction(QIcon(":/images/newfile.png"), tr("&New"), this, SLOT(newFile()),
                        QKeySequence::New);

    fileMenu->addAction(QIcon(":/images/openfile.png"), tr("&Open"), this, SLOT(openFile()),
                        QKeySequence::Open);

    fileMenu->addAction(QIcon(":/images/savefile.png"), tr("&Save"), this, SLOT(saveFile()),
                        QKeySequence::Save);

    fileMenu->addAction(QIcon(":/images/saveasfile.png"), tr("Save &As"), this, SLOT(saveAsFile()),
                        QKeySequence::SaveAs);

    // fileMenu->addAction(QIcon(":/images/print.png"), tr("Print"), this, SLOT(printFile()), QKeySequence::Print);
    // fileMenu->addAction(QIcon(":/images/zip.png"), tr("Archive"), this, SLOT(zipFile()), 0);

    fileMenu->addAction(tr("E&xit"), qApp, SLOT(quit()),
                        QKeySequence::Quit);

    QMenu *projMenu = new QMenu(tr("&Project"), this);
    menuBar()->addMenu(projMenu);

    projMenu->addAction(QIcon(":/images/hardware.png"), tr("Hardware"), this, SLOT(hardware()), Qt::Key_F6);
    projMenu->addAction(QIcon(":/images/properties.png"), tr("Properties"), this, SLOT(properties()), Qt::Key_F5);

    QMenu *debugMenu = new QMenu(tr("&Debug"), this);
    menuBar()->addMenu(debugMenu);

    debugMenu->addAction(QIcon(":/images/debug.png"), tr("Debug"), this, SLOT(programDebug()), Qt::Key_F8);
    debugMenu->addAction(QIcon(":/images/build.png"), tr("Build"), this, SLOT(programBuild()), Qt::Key_F9);
    debugMenu->addAction(QIcon(":/images/run.png"), tr("Run"), this, SLOT(programRun()), Qt::Key_F10);
    debugMenu->addAction(QIcon(":/images/burnee.png"), tr("Burn"), this, SLOT(programBurnEE()), Qt::Key_F11);
}

void MainWindow::setupProjectTools(QSplitter *vsplit)
{
    /* container for project, etc... */
    leftSplit = new QSplitter(this);
    leftSplit->setMinimumHeight(500);
    leftSplit->setOrientation(Qt::Vertical);
    vsplit->addWidget(leftSplit);

    /* project tree */
    projectTree = new QTreeView(this);
    projectTree->setMinimumWidth(200);
    projectTree->setMinimumHeight(100);
    projectTree->setMaximumHeight(200);
    projectTree->setMaximumWidth(250);
    connect(projectTree,SIGNAL(clicked(QModelIndex)),this,SLOT(projectTreeClicked(QModelIndex)));
    leftSplit->addWidget(projectTree);

    /* project reference tree */
    referenceTree = new QTreeView(this);
    referenceTree->setMinimumHeight(300);
    referenceTree->setMaximumWidth(250);
    QFont font = referenceTree->font();
    font.setItalic(true);   // pretty and matches def name font
    referenceTree->setFont(font);
    connect(referenceTree,SIGNAL(clicked(QModelIndex)),this,SLOT(referenceTreeClicked(QModelIndex)));
    leftSplit->addWidget(referenceTree);

    /* project editors */
    editors = new QVector<QPlainTextEdit*>();

    /* project editor tabs */
    editorTabs = new QTabWidget(this);
    editorTabs->setTabsClosable(true);
    editorTabs->setMinimumWidth(550);
    connect(editorTabs,SIGNAL(tabCloseRequested(int)),this,SLOT(closeTab(int)));
    connect(editorTabs,SIGNAL(currentChanged(int)),this,SLOT(changeTab(int)));
    vsplit->addWidget(editorTabs);
}

void MainWindow::setupToolBars()
{
    fileToolBar = addToolBar(tr("File"));
    QToolButton *btnFileNew = new QToolButton(this);
    QToolButton *btnFileOpen = new QToolButton(this);
    QToolButton *btnFileSave = new QToolButton(this);
    QToolButton *btnFileSaveAs = new QToolButton(this);
    //QToolButton *btnFilePrint = new QToolButton(this);
    //QToolButton *btnFileZip = new QToolButton(this);

    addToolButton(fileToolBar, btnFileNew, QString(":/images/newfile.png"));
    addToolButton(fileToolBar, btnFileOpen, QString(":/images/openfile.png"));
    addToolButton(fileToolBar, btnFileSave, QString(":/images/savefile.png"));
    addToolButton(fileToolBar, btnFileSaveAs, QString(":/images/saveasfile.png"));
    //addToolButton(fileToolBar, btnFilePrint, QString(":/images/print.png"));
    //addToolButton(fileToolBar, btnFileZip, QString(":/images/zip.png"));

    connect(btnFileNew,SIGNAL(clicked()),this,SLOT(newFile()));
    connect(btnFileOpen,SIGNAL(clicked()),this,SLOT(openFile()));
    connect(btnFileSave,SIGNAL(clicked()),this,SLOT(saveFile()));
    connect(btnFileSaveAs,SIGNAL(clicked()),this,SLOT(saveAsFile()));
    //connect(btnFilePrint,SIGNAL(clicked()),this,SLOT(printFile()));
    //connect(btnFileZip,SIGNAL(clicked()),this,SLOT(zipFile()));

    btnFileNew->setToolTip(tr("New"));
    btnFileOpen->setToolTip(tr("Open"));
    btnFileSave->setToolTip(tr("Save"));
    btnFileSaveAs->setToolTip(tr("Save As"));
    //btnFilePrint->setToolTip(tr("Print"));
    //btnFileZip->setToolTip(tr("Archive"));

    propToolBar = addToolBar(tr("Properties"));
    QToolButton *btnProjectBoard = new QToolButton(this);
    addToolButton(propToolBar, btnProjectBoard, QString(":/images/hardware.png"));
    connect(btnProjectBoard,SIGNAL(clicked()),this,SLOT(hardware()));
    btnProjectBoard->setToolTip(tr("Board Config"));
    QToolButton *btnProjectProperties = new QToolButton(this);
    addToolButton(propToolBar, btnProjectProperties, QString(":/images/properties.png"));
    connect(btnProjectProperties,SIGNAL(clicked()),this,SLOT(properties()));

    btnProjectProperties->setToolTip(tr("Properties"));

    debugToolBar = addToolBar(tr("Debug"));
    QToolButton *btnDebugDebugTerm = new QToolButton(this);
    QToolButton *btnDebugRun = new QToolButton(this);
    QToolButton *btnDebugBuild = new QToolButton(this);
    QToolButton *btnDebugBurnEEP = new QToolButton(this);

    addToolButton(debugToolBar, btnDebugBuild, QString(":/images/build.png"));
    addToolButton(debugToolBar, btnDebugBurnEEP, QString(":/images/burnee.png"));
    addToolButton(debugToolBar, btnDebugRun, QString(":/images/run.png"));
    addToolButton(debugToolBar, btnDebugDebugTerm, QString(":/images/debug.png"));

    connect(btnDebugBuild,SIGNAL(clicked()),this,SLOT(programBuild()));
    connect(btnDebugBurnEEP,SIGNAL(clicked()),this,SLOT(programBurnEE()));
    connect(btnDebugDebugTerm,SIGNAL(clicked()),this,SLOT(programDebug()));
    connect(btnDebugRun,SIGNAL(clicked()),this,SLOT(programRun()));

    btnDebugBuild->setToolTip(tr("Build"));
    btnDebugBurnEEP->setToolTip(tr("Burn EEPROM"));
    btnDebugDebugTerm->setToolTip(tr("Debug"));
    btnDebugRun->setToolTip(tr("Run"));

    ctrlToolBar = addToolBar(tr("Control"));
    ctrlToolBar->setLayoutDirection(Qt::RightToLeft);
    cbBoard = new QComboBox(this);
    cbPort = new QComboBox(this);
    cbBoard->setLayoutDirection(Qt::LeftToRight);
    cbPort->setLayoutDirection(Qt::LeftToRight);
    cbBoard->setToolTip(tr("Board Type Select"));
    cbPort->setToolTip(tr("Serial Port Select"));
    cbBoard->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    cbPort->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    connect(cbBoard,SIGNAL(currentIndexChanged(int)),this,SLOT(setCurrentBoard(int)));
    connect(cbPort,SIGNAL(currentIndexChanged(int)),this,SLOT(setCurrentPort(int)));

    btnConnected = new QToolButton(this);
    btnConnected->setToolTip(tr("Port Status"));
    btnConnected->setCheckable(true);
    connect(btnConnected,SIGNAL(clicked()),this,SLOT(connectButton()));

    QToolButton *btnPortScan = new QToolButton(this);
    btnPortScan->setToolTip(tr("Rescan Serial Ports"));
    connect(btnPortScan,SIGNAL(clicked()),this,SLOT(enumeratePorts()));

    /* this could be a refresh hardware toolbar button ...
     * the feature is automatic after hardware dialog OK now.
    QToolButton *btnConfigScan = new QToolButton(this);
    btnConfigScan->setToolTip(tr("Rescan Board Configurations"));
    connect(btnConfigScan,SIGNAL(clicked()),this,SLOT(initBoardTypes()));
    */

    addToolButton(ctrlToolBar, btnConnected, QString(":/images/connected2.png"));
    addToolButton(ctrlToolBar, btnPortScan, QString(":/images/refresh.png"));
    ctrlToolBar->addWidget(cbPort);
    //addToolButton(ctrlToolBar, btnConfigScan, QString(":/images/refresh.png"));
    ctrlToolBar->addWidget(cbBoard);
    ctrlToolBar->setSizePolicy(QSizePolicy::Expanding,QSizePolicy::Fixed);
}

