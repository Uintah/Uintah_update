/*
   For more information, please see: http://software.sci.utah.edu

   The MIT License

   Copyright (c) 2004 Scientific Computing and Imaging Institute,
   University of Utah.

   License for the specific language governing rights and limitations under
   Permission is hereby granted, free of charge, to any person obtaining a
   copy of this software and associated documentation files (the "Software"),
   to deal in the Software without restriction, including without limitation
   the rights to use, copy, modify, merge, publish, distribute, sublicense,
   and/or sell copies of the Software, and to permit persons to whom the
   Software is furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
   DEALINGS IN THE SOFTWARE.
*/


/*
 *  BuilderWindow.cc:
 *
 *  Written by:
 *   Steven G. Parker
 *   Department of Computer Science
 *   University of Utah
 *   October 2001
 *
 */

#include <CCA/Components/Builder/BuilderWindow.h>
#include <Core/CCA/PIDL/PIDL.h>
#include <Core/CCA/spec/cca_sidl.h>
#include <CCA/Components/Builder/NetworkCanvasView.h>
#include <CCA/Components/Builder/ClusterDialog.h>
#include <CCA/Components/Builder/PathDialog.h>
#include <CCA/Components/Builder/QtUtils.h>
#include <SCIRun/TypeMap.h>
#include <Core/Thread/Thread.h>
#include <Core/Containers/StringUtil.h>

#include <fstream>
#include <iostream>

#include <qapplication.h>
#include <qaction.h>
#include <qcanvas.h>
#include <qlabel.h>
#include <qlayout.h>
#include <qmenubar.h>
#include <qmessagebox.h>
#include <qmotifplusstyle.h>
#include <qpopupmenu.h>
#include <qsplitter.h>
#include <qstatusbar.h>
#include <qtextedit.h>
#include <qvbox.h>
#include <qwhatsthis.h>
#include <qiconset.h> 
#include <qtoolbutton.h>
#include <qtooltip.h> 
#include <qfiledialog.h>
#include <qtextstream.h>
#include <qwhatsthis.h>


namespace SCIRun {

MenuTree::MenuTree(BuilderWindow* builder, const std::string &url)
  :  builder(builder)
{
    this->url = url;
}

MenuTree::~MenuTree()
{
    for (std::map<std::string, MenuTree*>::iterator iter = child.begin();
        iter != child.end(); iter++) {
        delete iter->second;
    }
}
  
void MenuTree::add(const std::vector<std::string>& name, int nameindex,
           const sci::cca::ComponentClassDescription::pointer& desc,
           const std::string& fullname)
{
    if (nameindex == (int) name.size()) {
        if ( !cd.isNull() ) {
            std::cerr << "Duplicate component: " << fullname << '\n';
        } else {
            cd = desc;
        }
    } else {
        const std::string& n = name[nameindex];
        std::map<std::string, MenuTree*>::iterator iter = child.find(n);
        if(iter == child.end()) {
            child[n] = new MenuTree(builder, url);
        }
        child[n]->add(name, nameindex+1, desc, fullname);
    }
}

void MenuTree::coalesce()
{
    for (std::map<std::string, MenuTree*>::iterator iter = child.begin();
            iter != child.end(); iter++) {
        MenuTree* c = iter->second;
        while (c->child.size() == 1) {
            std::map<std::string, MenuTree*>::iterator grandchild = c->child.begin();
            std::string newname = iter->first + "." + grandchild->first;

            MenuTree* gc = grandchild->second;
            c->child.clear(); // So that grandchild won't get deleted...
            delete c;

            child.erase(iter);
            child[newname] = gc;
            iter = child.begin();
            c = gc;
        }
        c->coalesce();
    }
}

void MenuTree::populateMenu(QPopupMenu* menu)
{
    menu->insertTearOffHandle();
    for (std::map<std::string, MenuTree*>::iterator iter = child.begin();
            iter != child.end(); iter++) {
        if (! iter->second->cd.isNull()) {
            menu->insertItem(iter->first.c_str(), iter->second, SLOT( instantiateComponent()));
        } else {
            QPopupMenu* submenu = new QPopupMenu(menu);
            submenu->setFont(builder->font());
            iter->second->populateMenu(submenu);
            menu->insertItem(iter->first.c_str(), submenu);
        }
    }
}

void MenuTree::clear()
{
    child.clear();
}

void MenuTree::instantiateComponent()
{
    builder->instantiateComponent(cd);
}


BuilderWindow::BuilderWindow(const sci::cca::Services::pointer& services,
                             QApplication *app)
  : QMainWindow(0, "SCIRun", WDestructiveClose | WType_TopLevel),
    services(services)
{
    addReference(); // Do something better than this! - used because of memory leak? (AK)

#if !defined (_WIN32) && !defined (__APPLE__)
    // add enhanced *nix style support
    QApplication::setStyle( new QMotifPlusStyle(TRUE) );
#endif

    menuBar()->setFrameStyle(QFrame::Raised | QFrame::MenuBarPanel);
    QColor bgcolor(0, 51, 102);
    bFont = new QFont(this->font().family(), 11);
    setFont(*bFont);
    componentMenu = new QPopupMenu(this, "Components");
    componentMenu->setFont(*bFont);
    resize(840, 820);

    vsplit = new QSplitter(this, "vsplit");
    vsplit->setOrientation(QSplitter::Vertical);
    hsplit = new QSplitter(vsplit, "hsplit");
    hsplit->setOrientation( QSplitter::Horizontal );

    miniCanvas = new QCanvas();
    miniCanvas->setAdvancePeriod(30);
    miniCanvas->setBackgroundColor(bgcolor);

    miniView = new QCanvasView(miniCanvas, hsplit, "mini_canvas_view");
    miniView->setFixedSize(214, 214);
    hsplit->setResizeMode(miniView, QSplitter::KeepSize);
    int miniW = miniView->contentsRect().width();
    int miniH = miniView->contentsRect().height();
    miniCanvas->resize(miniW, miniH);

    vBox = new QVBox(hsplit, "vbox");
    vBox->setMargin(2);
    vBox->setSpacing(2);

    msgLabel = new QLabel("Messages:", vBox);
    msgTextEdit = new QTextEdit(vBox, "messages");
    msgTextEdit->setTextFormat(Qt::PlainText);
    msgTextEdit->setWordWrap(QTextEdit::WidgetWidth);
    msgTextEdit->setVScrollBarMode(QTextEdit::AlwaysOn);
    msgTextEdit->setReadOnly(TRUE);
    msgTextEdit->setUndoRedoEnabled(FALSE);
    msgTextEdit->setFocus();

    msgLabel->setBuddy(msgTextEdit);

    QWhatsThis::add(msgTextEdit, "Read-only text edit widget.");
    QToolTip::add(msgTextEdit, "View SCIRun2 messages.");
    // version number?
    displayMsg("SCIRun2");
    sci::cca::ports::FrameworkProperties::pointer fwkProperties =
    pidl_cast<sci::cca::ports::FrameworkProperties::pointer>(
        services->getPort("cca.FrameworkProperties")
    );
    if (fwkProperties.isNull()) {
        QString msg("Framework URL: url not available");
        displayMsg(msg);
    } else {
        sci::cca::TypeMap::pointer tm = fwkProperties->getProperties();
        std::string url = tm->getString("url", "");
        displayMsg("Framework URL: " + url);
        services->releasePort("cca.FrameworkProperties");
    }
    displayMsg("----------------------");

    networkCanvas = new QCanvas(2000, 2000);
    networkCanvas->setAdvancePeriod(30);
    //networkCanvas->setUpdatePeriod(100);
    networkCanvas->setDoubleBuffering(true);
    networkCanvas->setBackgroundColor(bgcolor);

    networkCanvasView = new NetworkCanvasView(this, networkCanvas, vsplit, services);
    // need better help than this!
    QWhatsThis::add(networkCanvasView, "View and manipulate components.");

    setCentralWidget(vsplit);
    setupFileActions();
    setupLoaderActions();
    setupHelpActions();
    buildPackageMenus();

    statusBar()->message("SCIRun2 ready");

    sci::cca::ports::ComponentEventService::pointer ces =
        pidl_cast<sci::cca::ports::ComponentEventService::pointer>(
            services->getPort("cca.ComponentEventService")
        );
    if (ces.isNull()) {
        displayMsg("Error: Cannot find component event service.");
    } else {
        sci::cca::ports::ComponentEventListener::pointer listener(this);
        ces->addComponentEventListener(sci::cca::ports::AllComponentEvents,
            listener, true);
        services->releasePort("cca.ComponentEventService");
    }
    updateMiniView();
    filename = QString::null;
}

BuilderWindow::~BuilderWindow()
{
}

void BuilderWindow::setupFileActions()
{
    QPopupMenu* file = new QPopupMenu(this, "File");
    file->setFont(*bFont);
    int id = menuBar()->insertItem("&File", file, -1, 0);
    popupMenuID.insert(IntMap::value_type("File", id));
    file->insertTearOffHandle();

    QToolBar* fileTools = new QToolBar(this, "file operations");
    fileTools->setLabel("File Operations");

#include "load.xpm"
#include "save.xpm"
#include "insert.xpm"
#include "info.xpm"

    // Load
    loadAction = new QAction("&Load", CTRL+Key_L, this, "load");
    loadAction->setText("Load a network from a file.");
    loadAction->setWhatsThis("Load a network from a file.");
    connect(loadAction, SIGNAL( activated() ), this, SLOT( load() ));
    loadAction->addTo(file);
    new QToolButton( QIconSet( QPixmap(load) ), "Load", QString::null,
             this, SLOT( load() ), fileTools, "load" );

    // Insert
    insertAction = new QAction("&Insert", 0, this, "insert");
    insertAction->setText("Insert");
    connect(insertAction, SIGNAL( activated() ), this, SLOT( insert() ));
    insertAction->addTo(file);
    new QToolButton( QIconSet( QPixmap(insert_xpm) ), "Insert", QString::null,
             this, SLOT( insert() ), fileTools, "insert" );

    // Save
    saveAction = new QAction("&Save", CTRL+Key_S, this, "save");
    saveAction->setText("Save network to file");
    connect(saveAction, SIGNAL( activated() ), this, SLOT( save() ));
    saveAction->addTo(file);
    new QToolButton( QIconSet( QPixmap(save) ), "Save", QString::null,
             this, SLOT( save() ), fileTools, "save" );

    // Save As
    saveAsAction = new QAction("Save &As", 0, this, "save as");
    saveAsAction->setText("Save network to file as");
    connect(saveAsAction, SIGNAL( activated() ), this, SLOT( saveAs() ));
    saveAsAction->addTo(file);

    file->insertSeparator();

    // Clear
    clearAction = new QAction("&Clear Network", 0, this, "clear");
    clearAction->setText("Clear Network");
    connect(clearAction, SIGNAL( activated() ), this, SLOT( clear() ));
    clearAction->addTo(file);

    // AddInfo
    addInfoAction = new QAction("Add &Info", 0, this, "addInfo");
    addInfoAction->setText("Add Info");
    connect(addInfoAction, SIGNAL( activated() ), this, SLOT( addInfo() ));
    addInfoAction->addTo(file);
    new QToolButton( QIconSet( QPixmap(info) ), "Add Info", QString::null,
             this, SLOT( addInfo() ), fileTools, "addInfo" );

    // Add Sidl Xml Path
    addPathAction = new QAction("Add Sidl XML Path", 0, this, "edit path");
    addPathAction->setText("Add a path to XML descriptions of components.");
    addPathAction->setWhatsThis("Add a path to XML descriptions of components are found.");
    connect(addPathAction, SIGNAL( activated() ),
        this, SLOT( addSidlXmlPath() ));
    addPathAction->addTo(file);

    file->insertSeparator();

    // Quit
    quitAction = new QAction("Quit GUI", CTRL+Key_Q, this, "Quit");
    quitAction->setText("Quit GUI (leave SCIRun2 running)");
    connect(quitAction, SIGNAL( activated() ), this, SLOT( close() ));
    quitAction->addTo(file);

    // Exit
    exitAction = new QAction("Exit", CTRL+Key_X, this, "Exit");
    exitAction->setText("Exit (terminate all components)");
    exitAction->setWhatsThis("Terminate all components and exit.");
    connect(exitAction, SIGNAL( activated() ), this, SLOT( exit() ));
    exitAction->addTo(file);

    (void) QWhatsThis::whatsThisButton( fileTools );
}

void BuilderWindow::setupLoaderActions()
{
    QPopupMenu* loader = new QPopupMenu(this, "Loader");
    loader->setFont(*bFont);
    int id = menuBar()->insertItem("&Proxy Frameworks", loader, -1, 1);
    popupMenuID.insert(IntMap::value_type("Loader", id));
    loader->insertTearOffHandle();

    // AddLoader
    addLoaderAction = new QAction("&Add Loader", CTRL+Key_A, this, "Add Loader");
    addLoaderAction->setText("Start a parallel Component loader.");
    addLoaderAction->setWhatsThis("Add a parallel Component loader to the framework.");
    addLoaderAction->setToolTip("Add a parallel Component loader to the framework.");
    connect(addLoaderAction, SIGNAL( activated() ), this, SLOT( addLoader() ));
    addLoaderAction->addTo(loader);

    // RmLoader
    rmLoaderAction = new QAction("&Remove Loader", CTRL+Key_D, this, "Remove Loader");
    rmLoaderAction->setText("Remove a parallel Component loader.");
    rmLoaderAction->setWhatsThis("Remove a parallel Component loader from the framework.");
    rmLoaderAction->setToolTip("Remove a parallel Component loader from the framework.");
    connect(rmLoaderAction, SIGNAL( activated() ), this, SLOT( rmLoader() ));
    rmLoaderAction->addTo(loader);

    // Refresh
    refreshAction = new QAction("Refresh Menu", CTRL+Key_R, this, "Refresh Menu");
    refreshAction->setText("Refresh Menu");
    connect(refreshAction, SIGNAL( activated() ), this, SLOT( refresh() ));
    refreshAction->addTo(loader);

    //loader->insertSeparator();
    //loaderMenu = new QPopupMenu(loader, "Loaders");
    //loader->insertItem("Loaders", loaderMenu);

#if 0
/*
    QPopupMenu* mxn = new QPopupMenu( this );
    menuBar()->insertItem("&MxN", mxn );
    mxn->insertItem( "&MxN-enabled Component_1", this, SLOT( mxn_add() ), Key_F1 );
    mxn->insertItem( "&MxN-enabled Component_2", this, SLOT( mxn_add() ), Key_F2 );
    mxn->insertItem( "&MxN-enabled Component_3", this, SLOT( mxn_add() ), Key_F3 );
    QPopupMenu* performance = new QPopupMenu(this);
    menuBar()->insertItem( "&Performance", performance );
    performance->insertItem( "&Performance Manager", this, SLOT( performance_mngr() ), Key_F1 );
    performance->insertItem( "&Add Tau component", this, SLOT( performance_tau_add() ), Key_F2 );
*/
#endif
}

void BuilderWindow::setupHelpActions()
{
  int id = menuBar()->insertSeparator(3);
  popupMenuID.insert(IntMap::value_type("Separator", id));
  QPopupMenu *help = new QPopupMenu(this, "Help");
  help->setFont(*bFont);
  help->insertTearOffHandle();
  id = -1;
  id = menuBar()->insertItem("&Help", help, -1, 4);
  popupMenuID.insert(IntMap::value_type("Help", id));

  // maybe it would be better to create a QActionGroup for these
  help->insertItem( "&About", this, SLOT( about() ), Key_F1 );
  help->insertSeparator();
  help->insertItem( "What's &This", this, SLOT( whatsThis() ), SHIFT+Key_F1 );

#if 0
/*  
    help->insertTearOffHandle();
    menuBar()->insertItem( "&Help", help );
    help->insertItem( "&Demos", this, SLOT( demos() ), Key_F1 );
    help->insertItem( "What's &This", this, SLOT( whatsThis() ), SHIFT+Key_F2 );
    help->insertSeparator();
    help->insertItem( "&About", this, SLOT( about() ), Key_F3 );
*/
#endif
}

void BuilderWindow::closeEvent( QCloseEvent* ce )
{
  switch(QMessageBox::information(this, "SCIRun",
                  "Do you want to quit the SCIRun builder and leave all components running, or exit SCIRun and terminate all components?",
                  "Quit (leave SCIRun running)", "Exit (terminate all components)", "Cancel", 0, 2)){
  case 0:
    ce->accept();
    break;
  case 1:
    exit();
    ce->accept();
    break;
  case 2:
  default:
    ce->ignore();
    break;
  }
}

void BuilderWindow::buildRemotePackageMenus(
    const sci::cca::ports::ComponentRepository::pointer &reg,
    const std::string &frameworkURL)
{
    if (reg.isNull()) {
        displayMsg("Cannot get component registry, not building component menus.");
        return;
    }

    std::vector<sci::cca::ComponentClassDescription::pointer> list =
    reg->getAvailableComponentClasses();
    std::map<std::string, MenuTree*> menus;

    for (std::vector<sci::cca::ComponentClassDescription::pointer>::iterator iter =
            list.begin();
            iter != list.end(); iter++) {
        // model name could be obtained somehow locally.
        // and we can assume that the remote component model is always "CCA"
        std::string model = "CCA"; //(*iter)->getModelName();
        if (model != "CCA") { continue; }
            model = frameworkURL;
        if (menus.find(model) == menus.end()) {
            menus[model] = new MenuTree(this, frameworkURL);
        }
        std::string name = (*iter)->getComponentClassName();
        std::vector<std::string> splitname = split_string(name, '.');
        menus[model]->add(splitname, 0, *iter, name);
    }

    for (std::map<std::string, MenuTree*>::iterator iter = menus.begin();
            iter != menus.end(); iter++) {
        iter->second->coalesce();
    }

    for (std::map<std::string, MenuTree*>::iterator iter = menus.begin();
            iter != menus.end(); iter++) {
        QPopupMenu* menu = new QPopupMenu(this);
        menu->setFont(*bFont);
        iter->second->populateMenu(menu);
        menuBar()->insertItem(iter->first.c_str(), menu);
    }
}


/*
 * Populate component menu by obtaining component class descriptions from
 * the CCA component repository.
 */

void BuilderWindow::buildPackageMenus()
{
    statusBar()->message("Building component menus...");
    setCursor(Qt::WaitCursor);
    componentMenu->clear();
    menus.clear();

    sci::cca::ports::ComponentRepository::pointer reg =
        pidl_cast<sci::cca::ports::ComponentRepository::pointer>(
            services->getPort("cca.ComponentRepository")
        );
    if (reg.isNull()) {
        displayMsg("Error: cannot find component registry, not building component menus.");
        unsetCursor();
        statusBar()->clear();
        return;
    }
    std::vector<sci::cca::ComponentClassDescription::pointer> list =
        reg->getAvailableComponentClasses();

    for (std::vector<sci::cca::ComponentClassDescription::pointer>::iterator iter =
            list.begin();
            iter != list.end(); iter++) {
        // model name could be obtained somehow locally.
        // and we can assume that the remote component model is always "CCA"
        std::string model = (*iter)->getComponentModelName();
        std::string loaderName = (*iter)->getLoaderName();

        std::string name = (*iter)->getComponentClassName();

        // component class has a loader that is not in this address space?
        if (loaderName != "") {
            std::string::size_type i = name.find_first_of(".");
            name.insert(i, "@" + loaderName);
        }
        if (menus.find(model) == menus.end()) {
            menus[model] = new MenuTree(this, "");
        }
        std::vector<std::string> splitname = split_string(name, '.');
        menus[model]->add(splitname, 0, *iter, name);
    }

    for (std::map<std::string, MenuTree*>::iterator iter = menus.begin();
            iter != menus.end(); iter++) {
        iter->second->coalesce();
    }

    componentMenu->insertItem("Components");

    // component menus should be inserted between the loader menu and
    // the separator/help menu
    int insert_index;
    if (popupMenuID["Separator"]) {
        insert_index = menuBar()->indexOf(popupMenuID["Separator"]);
    } else {
        insert_index = popupMenuID["Loader"];
    }
    for (MenuMap::iterator iter = menus.begin();
            iter != menus.end(); iter++) {
        QPopupMenu* menu = new QPopupMenu(this);
        menu->setFont(*bFont);
        int menuID;
        iter->second->populateMenu(menu);
        IntMap::iterator found = popupMenuID.find(iter->first);
        if (found != popupMenuID.end()) {
            int index = menuBar()->indexOf(popupMenuID[iter->first]);
            menuBar()->removeItemAt(index);
            menuID = menuBar()->insertItem(iter->first.c_str(), menu,
                popupMenuID[iter->first], index);
        } else {
            menuID = menuBar()->insertItem(iter->first.c_str(), menu, -1, insert_index++);
        }
        popupMenuID[iter->first] = menuID;
        componentMenu->insertItem((" " + iter->first).c_str(), menu);
    }

    services->releasePort("cca.ComponentRepository");
    unsetCursor();
    statusBar()->clear();
}

void BuilderWindow::writeFile()
{
    QCanvasItemList tempQCL = miniCanvas->allItems();
    setCursor(Qt::WaitCursor);
    std::ofstream saveOutputFile(filename);

    std::vector<Module*> saveModules = networkCanvasView->getModules();
    std::vector<Connection*> saveConnections = networkCanvasView->getConnections();

    saveOutputFile << saveModules.size() << std::endl;
    saveOutputFile << saveConnections.size() << std::endl;

    if (saveOutputFile.is_open()) {
        for (unsigned int j = 0; j < saveModules.size(); j++) {
            saveOutputFile << saveModules[j]->moduleName() << std::endl;
            saveOutputFile << saveModules[j]->x() << std::endl;
            saveOutputFile << saveModules[j]->y() << std::endl;
        }
        for (unsigned int k = 0; k < saveConnections.size(); k++) {
            //Module* getProvidesModule();

            Module* um = saveConnections[k]->usesPort()->module();
            Module* pm = saveConnections[k]->providesPort()->module();
            unsigned int iu = 0;
            unsigned int ip = 0;
            for (unsigned int i = 0; i < saveModules.size();i++) {
                if (saveModules[i] == um) {
                    iu = i;
                }
                if (saveModules[i] == pm) {
                    ip = i;
                }
            }
            saveOutputFile << iu << " " <<
            saveConnections[k]->usesPort()->name() << " " << ip << " " <<
            saveConnections[k]->providesPort()->name() << std::endl;
        }
    }
    saveOutputFile.close();
    unsetCursor();
}

void BuilderWindow::save()
{
    if (filename.isEmpty()) {
    QString fn = QFileDialog::getSaveFileName(QString::null,
                          "Network File (*.net)", this);
        if (fn.isEmpty()) {
            statusBar()->message("Saving aborted", 2000);
        } else {
            if (fn.endsWith(".net")) {
                filename = fn;
            } else {
                QString fnExt = fn + ".net";
                filename = fnExt;
            }
            writeFile();
        }
    }
}

void BuilderWindow::saveAs()
{
    QString fn = QFileDialog::getSaveFileName(QString::null,
        "Network File (*.net)", this);
    if (fn.isEmpty()) {
        statusBar()->message("Saving aborted", 2000);
    } else {
        if (fn.endsWith(".net")) {
            filename = fn;
        } else {
            QString fnExt = fn + ".net";
            filename = fnExt;
        }
        writeFile();
    }
}

void BuilderWindow::load()
{
    setCursor(Qt::WaitCursor);

    std::vector<Module*> grab_latest_Modules;
    std::vector<Module*> ptr_table;
    QString fn = QFileDialog::getOpenFileName(QString::null,
                                              "Network File (*.net)",
                                              this);
    if (fn.isEmpty()) {
        unsetCursor();
        return;
    }

    filename = fn;
    std::ifstream is( fn ); 

    int load_Modules_size = 0;
    int load_Connections_size = 0;
    std::string tmp_moduleName;
    int tmp_moduleName_x;
    int tmp_moduleName_y;

    is >> load_Modules_size >> load_Connections_size;
    std::cout<<"load_Modules_size"<<load_Modules_size<<std::endl;
    std::cout<<"load_Connections_size"<<load_Connections_size<<std::endl;
    for (int i = 0; i < load_Modules_size; i++) {
        is >> tmp_moduleName >> tmp_moduleName_x >> tmp_moduleName_y;

        sci::cca::ports::BuilderService::pointer builder =
            pidl_cast<sci::cca::ports::BuilderService::pointer>(
                services->getPort("cca.BuilderService")
            );
        if (builder.isNull()) {
            displayMsg("Error: Cannot find builder service.");
            return;
        }
        sci::cca::ComponentID::pointer cid =
            builder->createInstance(tmp_moduleName, tmp_moduleName, sci::cca::TypeMap::pointer(0));
        services->releasePort("cca.BuilderService");

        if (tmp_moduleName != "SCIRun.Builder") {
            networkCanvasView->addModule(tmp_moduleName, tmp_moduleName_x, tmp_moduleName_y,cid, false); //fixed position
        }
        grab_latest_Modules = networkCanvasView->getModules();
        ptr_table.push_back( grab_latest_Modules[grab_latest_Modules.size()-1] );
    }

    for (int i = 0; i < load_Connections_size; i++) {
        int iu, ip;
        std::string up, pp;
        is >> iu >> up >> ip >> pp;
        networkCanvasView->addConnection(ptr_table[iu], up, ptr_table[ip], pp);
    }
    is.close();

    unsetCursor();
    statusBar()->message("Loading done.");
    return;
}

void BuilderWindow::insert()
{
    displayMsg("BuilderWindow::insert not finished.");
}

void BuilderWindow::clear()
{
    std::cerr << "BuilderWindow::clear(): deleting the following: " << std::endl;
    setCursor(Qt::WaitCursor);

    // assign modules to local variable
    std::vector<Module*> clearModules = networkCanvasView->getModules();

    for (unsigned int j = 0; j < clearModules.size(); j++) {
        std::cerr << "modules->getName = " <<
            clearModules[j]->moduleName() << std::endl;
        clearModules[j]->destroy();
    }
    unsetCursor();
}

void BuilderWindow::addInfo()
{
    displayMsg("BuilderWindow::addInfo not finished.");
}

void BuilderWindow::exit()
{
  std::cerr << "Exit should ask framework to shutdown instead!" << std::endl;
  //should stop and close socket in CCACommunicator first - support in CCACommunicator for this?
  Thread::exitAll(0);
}

void BuilderWindow::mxn_add()
{
  ( new QMessageBox())->about( this, "MxN: Add", "Under Construction\n\nWhen this menu item is activated, the chosen parallel component will be added to the \ncanvas.  This will occur in the same manner as when other CCA components \nare instantiated.\n\n" );
}

void BuilderWindow::performance_mngr()
{
  ( new QMessageBox())->about( this, "Performance Manager", "Under Construction\n\nThis dialog will display all components active on the canvas and assist the user in setting performance settings for each.\n\n" );
}

void BuilderWindow::performance_tau_add()
{
  ( new QMessageBox())->about( this, "Tau: Add", "Under Construction\n\nWhen this menu item is activated, a tau component will be added to the canvas.  \nThis will occur in the same manner as when other CCA components are \ninstantiated.\n\n" );
}

void BuilderWindow::demos()
{
  ( new QMessageBox())->about( this, "Demos", "CCA Demos\nComing Soon!" );
}

void BuilderWindow::about()
{
  std::cerr << "BuilderWindow::about not finished" << std::endl;
  (new QMessageBox())->about(this, "About", "CCA Builder (SCIRun Implementation)");
}

void BuilderWindow::instantiateComponent(
    const sci::cca::ComponentClassDescription::pointer& cd)
{
    statusBar()->message("Instantating component...");
    setCursor(Qt::WaitCursor);

    sci::cca::ports::BuilderService::pointer builder =
        pidl_cast<sci::cca::ports::BuilderService::pointer>(
            services->getPort("cca.BuilderService")
        );
    if (builder.isNull()) {
        std::cerr << "Fatal Error: Cannot find builder service" << std::endl;
        unsetCursor();
        return;
    }

    TypeMap *tm = new TypeMap;
    tm->putString("LOADER NAME", cd->getLoaderName());
    tm->putString("cca.className", cd->getComponentModelName()); // component type
    sci::cca::ComponentID::pointer cid =
        builder->createInstance(cd->getComponentClassName(),
                cd->getComponentClassName(),
                sci::cca::TypeMap::pointer(tm));

    if (cid.isNull()) {
        displayMsg("Error: could not instantiate component of type " + cd->getComponentClassName());
        statusBar()->message("Instantiate failed.", 2000);
    } else {
        statusBar()->clear();
    }
    unsetCursor();
}

Module* BuilderWindow::instantiateBridgeComponent(
    const std::string& className,
    const std::string& type,
    const std::string& loaderName)
{
    statusBar()->message("Instantating component " + className);
    setCursor(Qt::WaitCursor);

    sci::cca::ports::BuilderService::pointer builder =
    pidl_cast<sci::cca::ports::BuilderService::pointer>(
        services->getPort("cca.BuilderService")
    );

    if (builder.isNull()) {
        displayMsg("Error: Cannot find builder service.");
        unsetCursor();
        return NULL;
    }

    TypeMap *tm = new TypeMap;
    tm->putString("LOADER NAME", loaderName);
    sci::cca::ComponentID::pointer cid =
        builder->createInstance(className, type, sci::cca::TypeMap::pointer(tm));

    if (cid.isNull()) {
        std::cerr << "instantiateFailed..." << std::endl;
        statusBar()->message("Instantiate failed.");
        unsetCursor();
        return NULL;
    }

    services->releasePort("cca.BuilderService");
    statusBar()->clear();
    unsetCursor();

    if (className != "SCIRun.Builder") {
        int x = 20;
        int y = 20;
        // reposition module
        return (networkCanvasView->addModule(className, x, y, cid, true));
    }
    return NULL;
}

void BuilderWindow::componentActivity(const sci::cca::ports::ComponentEvent::pointer& e)
{
    if (e->getComponentID()->getInstanceName() == "QtBuilder") {
        return;
    }
    if (e->getComponentID()->getInstanceName().find("SCIRun.Builder")
            != std::string::npos) {
        std::cerr << "Got builder window: " <<
            e->getComponentID()->getInstanceName() << std::endl;
        return;
    }

    bool update = true;
    //std::cerr << "BuilderWindow::componentActivity: " << e->getComponentID()->getInstanceName() << " " << e->getEventType() << std::endl;
    if (e->getEventType() == sci::cca::ports::ComponentInstantiated) {
        std::vector<Module*> modules = networkCanvasView->getModules();
        for (std::vector<Module*>::iterator iter = modules.begin();
            iter != modules.end(); iter++) {
            if ((*iter)->componentID()->getInstanceName() ==
                    e->getComponentID()->getInstanceName()) {
                update = false;
                break;
            }
        }

        if (update) {
            sci::cca::ports::BuilderService::pointer builder =
                pidl_cast<sci::cca::ports::BuilderService::pointer>(
                    services->getPort("cca.BuilderService")
                );
            if (builder.isNull()) {
                displayMsg("Error: Cannot find builder service.");
                return;
            }
            // add Module to window
            int x = 20;
            int y = 20;
            // reposition module
            networkCanvasView->addModule(
                e->getComponentID()->getInstanceName(),
                x, y, e->getComponentID(), true);
            services->releasePort("cca.BuilderService");
        }
    } else if (e->getEventType() == sci::cca::ports::ComponentDestroyed) {
        std::vector<Module*> modules = networkCanvasView->getModules();
        for (std::vector<Module*>::iterator iter = modules.begin();
            iter != modules.end(); iter++) {
            if ((*iter)->componentID()->getInstanceName() ==
                    e->getComponentID()->getInstanceName()) {
                networkCanvasView->removeModule(*iter);
            }
        }
    }
}

void BuilderWindow::displayMsg(const char *msg)
{
    msgTextEdit->insert( tr(msg) + '\n');
}

void BuilderWindow::displayMsg(const QString &text)
{
    msgTextEdit->insert(text + '\n');
}

void BuilderWindow::updateMiniView()
{
    // assign the temporary list
    // needed for coordinates of each module
    QCanvasItemList tempQCL = miniCanvas->allItems();

    for (unsigned int i = 0; i < tempQCL.size(); i++) {
    delete tempQCL[i];
    }
  
    // assign modules to local variable
    std::vector<Module*> modules = networkCanvasView->getModules();
    std::vector<Connection*> connections = networkCanvasView->getConnections();

    double scaleH = double( networkCanvas->width() ) / miniCanvas->width();
    double scaleV = double( networkCanvas->height() ) / miniCanvas->height();

    QCanvasRectangle *viewableRect = new QCanvasRectangle(
        int( networkCanvasView->contentsX() / scaleH ),
        int( networkCanvasView->contentsY() / scaleV ),
        int( networkCanvasView->visibleWidth() / scaleH ),
        int( networkCanvasView->visibleHeight() / scaleV ),
        miniCanvas
    );
    viewableRect->show();

    for (unsigned int i = 0; i < modules.size(); i++) {
    QPoint pm = modules[i]->posInCanvas();
    QCanvasRectangle *rect = new QCanvasRectangle(
        int( pm.x() / scaleH ),
        int( pm.y() / scaleV ),
        int( modules[i]->width() / scaleH ),
        int( modules[i]->height() / scaleV ),
        miniCanvas
    );
    rect->setBrush( Qt::white );
    rect->show();
    }

    for (unsigned int j = 0; j < connections.size(); j++) {
        MiniConnection *mc =
            new MiniConnection(miniView, connections[j]->points(), scaleH, scaleV);
        mc->show();
    }

    miniCanvas->update();
}


void BuilderWindow::addLoader()
{

    sci::cca::ports::BuilderService::pointer builder =
    pidl_cast<sci::cca::ports::BuilderService::pointer>(
        services->getPort("cca.BuilderService")
    );
    if (builder.isNull()) {
        displayMsg("Error: Cannot find builder service.");
        return;
    }

    sci::cca::ports::FrameworkProperties::pointer fwkProperties =
    pidl_cast<sci::cca::ports::FrameworkProperties::pointer>(services->getPort("cca.FrameworkProperties"));

    ClusterDialog *dialog;
    if (fwkProperties.isNull()) {
        displayMsg("Error: Cannot find framework properties.");
        dialog = new ClusterDialog("qwerty", "qwerty.sci.utah.edu",
                    "", this, "Add Parallel Component Loader", TRUE);
    } else {
        sci::cca::TypeMap::pointer tm = fwkProperties->getProperties();
        dialog = new ClusterDialog("qwerty", "qwerty.sci.utah.edu",
            (tm->getString("default_login", "")).c_str(), this,
            "Add Loader", TRUE);
        services->releasePort("cca.FrameworkProperties");
    }

    if (dialog->exec() == QDialog::Accepted) {
        std::string loaderName = dialog->loader();
        std::string domainName = dialog->domain();
        std::string login = dialog->login();
        std::string loaderPath="mpirun -np 3 ploader";
        //string password="****"; //not used;

        builder->addLoader(loaderName, login, domainName, loaderPath); // spawns xterm
        services->releasePort("cca.BuilderService");
    } else { // QDialog::Rejected
        return;
    }
  
#if 0
  //buildPackageMenus(loaderName);

  /*
  slaveServer_impl* ss = new slaveServer_impl;
  std::cerr << "Waiting for slave connections..." << std::endl;
  std::cerr << ss->getURL().getString() << '\n';
  
  //Wait until we have some slave available
  ss->d_slave_sema.down();
  
  slaveClient::pointer sc = ss->rr->getPtrToAll();
  
  //Set up server's requirement of the distribution array
  Index** dr = new Index* [1];
  dr[0] = new Index(0,ss->rr->getSize(),1);
  MxNArrayRep* arrr = new MxNArrayRep(1,dr);
  sc->setCallerDistribution("dURL",arrr);
  */  

  /******** Simulate invocations:*/
  /*
    string arg1 = "bb";
  SSIDL::array1<std::string> urls;
  sc->instantiate("aa",arg1,urls);
  for(unsigned int i = 0; i<urls.size(); i++)
    std::cout << "URL = " << urls[i] << "" << std::endl;
  */
#endif
}

void BuilderWindow::rmLoader()
{
    sci::cca::ports::BuilderService::pointer builder =
    pidl_cast<sci::cca::ports::BuilderService::pointer>(
        services->getPort("cca.BuilderService")
    );
    if (builder.isNull()) {
        displayMsg("Error: Cannot find builder service.");
        return;
    }
    builder->removeLoader("buzz");
    services->releasePort("cca.BuilderService");
}

void BuilderWindow::refresh()
{
    buildPackageMenus();
}

// add or edit?
void BuilderWindow::addSidlXmlPath()
{
    PathDialog* pd = new PathDialog(this, "SIDL XML path dialog");
    if (pd->exec() == QDialog::Rejected) {
        return;
    }
    if (pd->selectedDirectory().isEmpty()) {
        displayMsg("Error: the directory path is blank.");
        return;
    }
    if (pd->selectedComponentModel().isEmpty()) {
        displayMsg("Error: the component model is blank.");
        return;
    }

    // append directory to sidl path!
   sci::cca::ports::FrameworkProperties::pointer fwkProperties =
    pidl_cast<sci::cca::ports::FrameworkProperties::pointer>(
        services->getPort("cca.FrameworkProperties")
    );

    if (fwkProperties.isNull()) {
        displayMsg("Error: cannot find framework properties, not adding new components.");
        return;
    }
    sci::cca::TypeMap::pointer tm = fwkProperties->getProperties();
    std::string dir = pd->selectedDirectory();
    SSIDL::array1<std::string> sArray = tm->getStringArray("sidl_xml_path", sArray);
    sArray.push_back(dir);
    tm->putStringArray("sidl_xml_path", sArray);
    fwkProperties->setProperties(tm);

    services->releasePort("cca.FrameworkProperties");

    sci::cca::ports::ComponentRepository::pointer reg =
    pidl_cast<sci::cca::ports::ComponentRepository::pointer>(
        services->getPort("cca.ComponentRepository")
    );
    if (reg.isNull()) {
        std::cerr << "Error: cannot find component repository" << std::endl;
        return;
    }
    statusBar()->message("Updating component classes.");
    setCursor(Qt::WaitCursor);

    reg->addComponentClass(pd->selectedComponentModel());

    statusBar()->clear();
    unsetCursor();
    services->releasePort("cca.ComponentRepository");

    buildPackageMenus();
}

} // end namespace SCIRun
