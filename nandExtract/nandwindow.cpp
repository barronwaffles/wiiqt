#include "nandwindow.h"
#include "ui_nandwindow.h"

NandWindow::NandWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::NandWindow),
    nandBin( this )
{
    ui->setupUi(this);
    //ui->treeWidget->header()->resizeSection( 0, 300 );//name
    QFontMetrics fm( fontMetrics() );
    ui->treeWidget->header()->resizeSection( 0, fm.width( QString( 22, 'W' ) ) );//name
    ui->treeWidget->header()->resizeSection( 1, fm.width( "WWWWW" ) );//entry #
    ui->treeWidget->header()->resizeSection( 2, fm.width( "WWWWW" ) );//size
    ui->treeWidget->header()->resizeSection( 3, fm.width( "WWWWWWWWW" ) );//uid
    ui->treeWidget->header()->resizeSection( 4, fm.width( "WWWWWWWWW" ) );//gid
    ui->treeWidget->header()->resizeSection( 5, fm.width( "WWWWWWWWW" ) );//x3
    ui->treeWidget->header()->resizeSection( 6, fm.width( "WWWWW" ) );//mode
    ui->treeWidget->header()->resizeSection( 7, fm.width( "WWWWW" ) );//attr


    connect( &nandBin, SIGNAL( SendError( QString ) ), this, SLOT( GetError( QString ) ) );
    connect( &nandBin, SIGNAL( SendText( QString ) ), this, SLOT( GetStatusUpdate( QString ) ) );
}

NandWindow::~NandWindow()
{
    delete ui;
}

void NandWindow::changeEvent(QEvent *e)
{
    QMainWindow::changeEvent(e);
    switch (e->type()) {
    case QEvent::LanguageChange:
        ui->retranslateUi(this);
        break;
    default:
        break;
    }
}

void NandWindow::GetStatusUpdate( QString s )
{
    ui->statusBar->showMessage( s );
}

void NandWindow::GetError( QString str )
{
    qWarning() << str;
}

void NandWindow::ExtractShit()
{
    ui->statusBar->showMessage( "Trying to extract..." );
    nandBin.ExtractToDir( exItem, exPath );//who cares if it returns false?  not me.  thats what the qDebug() info is for
    ui->statusBar->showMessage( "Done", 5000 );

}

//nand window right-clicked
void NandWindow::on_treeWidget_customContextMenuRequested( QPoint pos )
{
    QPoint globalPos = ui->treeWidget->viewport()->mapToGlobal( pos );
    QTreeWidgetItem* item = ui->treeWidget->itemAt( pos );
    if( !item )//right-clicked in the partition window, but not on an item
    {
	qDebug() << "no item selected";
	return;
    }

    QMenu myMenu( this );
    QAction extractA( tr( "Extract" ), &myMenu );
    myMenu.addAction( &extractA );

    QAction* s = myMenu.exec( globalPos );
    //respond to what was selected
    if( s )
    {
	// something was chosen, do stuff
	if( s == &extractA )//extract a file
	{
	    QString path = QFileDialog::getExistingDirectory( this, tr("Select a destination") );
	    if( path.isEmpty() )
		return;

	    exPath = path;
	    exItem = item;

	    //ghetto, but gives the dialog box time to dissappear before the gui freezes as it extracts the nand
	    QTimer::singleShot( 250, this, SLOT( ExtractShit() ) );
	}
    }
}

//file->open
void NandWindow::on_actionOpen_Nand_triggered()
{
    QString path = QFileDialog::getOpenFileName( this, tr( "Select a Nand to open" ) );
    if( path.isEmpty() )
	return;

    if( !nandBin.SetPath( path ) )
    {
	qDebug() << " error in nandBin.SetPath";
	ui->statusBar->showMessage( "Error setting path to " + path );
	return;
    }
    ui->statusBar->showMessage( "Loading " + path );
    QIcon groupIcon;
    QIcon keyIcon;
    groupIcon.addPixmap( style()->standardPixmap( QStyle::SP_DirClosedIcon ), QIcon::Normal, QIcon::Off );
    groupIcon.addPixmap( style()->standardPixmap( QStyle::SP_DirOpenIcon ), QIcon::Normal, QIcon::On );
    keyIcon.addPixmap( style()->standardPixmap( QStyle::SP_FileIcon ) );
    if( !nandBin.InitNand( groupIcon, keyIcon ) )
    {
	qDebug() << " error in nandBin.InitNand()";
	ui->statusBar->showMessage( "Error reading " + path );
	return;
    }

    ui->treeWidget->clear();

    //get an item holding a tree with all the items of the nand
    QTreeWidgetItem* tree = nandBin.GetTree();

    //take the actual contents of the nand from the made up root and add them to the gui widget
    ui->treeWidget->addTopLevelItems( tree->takeChildren() );

    //delete the made up root item
    delete tree;
    ui->statusBar->showMessage( "Loaded " + path, 5000 );

    //nandBin.GetData( "/title/00000001/00000002/data/setting.txt" );//testing 1,2,1,2
}
