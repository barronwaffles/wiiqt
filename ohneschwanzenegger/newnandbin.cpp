#include "newnandbin.h"
#include "ui_newnandbin.h"
#include "../WiiQt/tools.h"

NewNandBin::NewNandBin( QWidget *parent, QList<quint16> badBlocks ) : QDialog(parent), ui(new Ui::NewNandBin), nand( this )
{
    dir = QDir::currentPath();
    ui->setupUi(this);
    foreach( quint16 block, badBlocks )
    {
		QString txt = QString( "%1" ).arg( block );
		if( ui->listWidget_badBlocks->findItems( txt, Qt::MatchExactly ).isEmpty() )
			ui->listWidget_badBlocks->addItem( txt );
    }
}

NewNandBin::~NewNandBin()
{
    delete ui;
}

void NewNandBin::on_pushButton_keys_clicked()
{
    QString f = QFileDialog::getOpenFileName( this, tr( "Select Keys.bin" ), dir );
    if( f.isEmpty() )
		return;
    ui->lineEdit_keys->setText( f );
    dir = QFileInfo( f ).canonicalPath();
}

void NewNandBin::on_pushButton_boot_clicked()
{
    QString f = QFileDialog::getOpenFileName( this, tr( "Select Boot 1 & 2" ), dir );
    if( f.isEmpty() )
		return;
    ui->lineEdit_boot->setText( f );
    dir = QFileInfo( f ).canonicalPath();
}

void NewNandBin::on_pushButton_dest_clicked()
{
    QString f = QFileDialog::getSaveFileName( this, tr( "Output file" ), dir );
    if( f.isEmpty() )
		return;
    ui->lineEdit_dest->setText( f );
    dir = QFileInfo( f ).canonicalPath();
}

QList<quint16> NewNandBin::BadBlocks()
{
    QList<quint16> ret;
    quint16 cnt = ui->listWidget_badBlocks->count();
    for( quint16 i = 0; i < cnt; i++ )
    {
		bool ok = false;
		quint16 num = ui->listWidget_badBlocks->item( i )->text().toInt( &ok );
		if( ok )
			ret << num;
    }
    return ret;
}

void NewNandBin::on_pushButton_bb_add_clicked()
{
    quint16 val = ui->spinBox->value();
    if( !BadBlocks().contains( val ) )
    {
		ui->listWidget_badBlocks->addItem( QString( "%1" ).arg( val ) );
    }
}

void NewNandBin::on_pushButton_bb_rm_clicked()
{
    QList<QListWidgetItem *> items = ui->listWidget_badBlocks->selectedItems();
    foreach( QListWidgetItem *item, items )
    {
		ui->listWidget_badBlocks->removeItemWidget( item );
		delete item;
    }
}

bool NewNandBin::CreateDefaultEntries()
{
	if( !nand.CreateEntry( "/sys", 0, 0, NAND_DIR, NAND_RW, NAND_RW, 0 )
		|| !nand.CreateEntry( "/ticket", 0, 0, NAND_DIR, NAND_RW, NAND_RW, 0 )
		|| !nand.CreateEntry( "/title", 0, 0, NAND_DIR, NAND_RW, NAND_RW, NAND_READ )
		|| !nand.CreateEntry( "/shared1", 0, 0, NAND_DIR, NAND_RW, NAND_RW, 0 )
		|| !nand.CreateEntry( "/shared2", 0, 0, NAND_DIR, NAND_RW, NAND_RW, NAND_RW )
		|| !nand.CreateEntry( "/import", 0, 0, NAND_DIR, NAND_RW, NAND_RW, 0 )
		|| !nand.CreateEntry( "/meta", 0x1000, 1, NAND_DIR, NAND_RW, NAND_RW, NAND_RW )
		|| !nand.CreateEntry( "/tmp", 0, 0, NAND_DIR, NAND_RW, NAND_RW, NAND_RW )
		|| !nand.WriteMetaData() )
	{
		qWarning() << "NewNandBin::on_buttonBox_accepted -> error creating directories in the new nand";
		QMessageBox::warning( this, tr( "Error" ), \
							  tr( "Can't create base folders in the new nand." ) );
		return false;
	}
	//add cert.sys
	quint16 handle = nand.CreateEntry( "/sys/cert.sys", 0, 0, NAND_FILE, NAND_RW, NAND_RW, NAND_READ );
	if( !handle || !nand.SetData( handle, QByteArray( (const char*)&certs_dat, CERTS_DAT_SIZE ) ) )
	{
		qWarning() << "NewNandBin::on_buttonBox_accepted -> error creating cert in the new nand";
		QMessageBox::warning( this, tr( "Error" ), \
							  tr( "Can't create cert.sys folders in the new nand." ) );
		return false;
	}
	//commit changes to metadata
	if( !nand.WriteMetaData() )
	{
		qWarning() << "NewNandBin::on_buttonBox_accepted -> error writing metadata";
		QMessageBox::warning( this, tr( "Error" ), \
							  tr( "Can't write metadata in the new nand." ) );
		return false;
	}
	return true;
}

//ok clicked
void NewNandBin::on_buttonBox_accepted()
{
    if( ui->lineEdit_keys->text().isEmpty() || ui->lineEdit_boot->text().isEmpty() || ui->lineEdit_dest->text().isEmpty() )
    {
		QMessageBox::warning( this, tr( "Error" ), tr( "Required feilds are empty" ) );
		return;
    }
	if( keys.isEmpty() )
		keys = ReadFile( ui->lineEdit_keys->text() );
	if( boots.isEmpty() )
		boots = ReadFile( ui->lineEdit_boot->text() );
    if( keys.size() != 0x400 || boots.size() != 0x108000 )
    {
		QMessageBox::warning( this, tr( "Error" ), tr( "The keys or boot1/2 is not correct" ) );
		keys.clear();
		boots.clear();
		return;
    }
    if( !nand.CreateNew( ui->lineEdit_dest->text(), keys, boots, BadBlocks() ) )
    {
		qDebug() << "error creating nand.bin";
		keys.clear();
		boots.clear();
		return;
    }
	//qDebug() << "created nand, trying to add default entries";
	if( !CreateDefaultEntries() )
	{
		keys.clear();
		boots.clear();
		return;
	}

    ret = ui->lineEdit_dest->text();
}

QString NewNandBin::GetNewNandPath( QWidget *parent, QList<quint16> badBlocks )
{
    NewNandBin d( parent, badBlocks );
    if( !d.exec() )
		return QString();
    return d.ret;
}

//read bad blocks from a txt file
void NewNandBin::on_pushButton_badBlockFile_clicked()
{
    QString f = QFileDialog::getOpenFileName( this, tr( "Select File with Bad Block List" ), dir );
    if( f.isEmpty() )
		return;
    dir = QFileInfo( f ).canonicalPath();
    QString str = QString( ReadFile( f ) );
    if( str.isEmpty() )
    {
		qWarning() << "NewNandBin::on_pushButton_badBlockFile_clicked -> error reading file";
		return;
    }
    ui->listWidget_badBlocks->clear();

    str.replace( "\r\n", "\n" );
    QStringList lines = str.split( "\n", QString::SkipEmptyParts );
    foreach( QString line, lines )
    {
		if( line.size() > 5 )
			continue;
		bool ok = false;

		if( ui->listWidget_badBlocks->findItems( line, Qt::MatchExactly ).size() )//this one is already in the list
			continue;

		quint16 bb = line.toInt( &ok );
		if( !ok || bb < 8 || bb > 4079 )
			continue;

		ui->listWidget_badBlocks->addItem( line );
    }
}

//read info from existing nand.bin
void NewNandBin::on_pushButton_oldNand_clicked()
{
	QString f = QFileDialog::getOpenFileName( this, tr( "Select Old nand.bin" ), dir );
	if( f.isEmpty() )
		return;


	QFileInfo fi( f );
	QFile file( fi.absoluteFilePath() );
	if( !file.exists() || !file.open( QIODevice::ReadOnly ) )
	{
		QMessageBox::warning( this, tr( "Error" ), \
							  tr( "Can't open %1!" ).arg( fi.absoluteFilePath() ) );
		return;
	}
	ui->listWidget_badBlocks->clear();

	switch( fi.size() )
	{
	case 0x21000000:// w/ ecc, keys in different file
		{
			boots = file.read( 0x108000 );
			//file.seek();
			keys = ReadFile( fi.absoluteDir().absoluteFilePath( "keys.bin" ) );

		}
		break;
	case 0x21000400:// w/ ecc, keys at end of nand dump
		{
			boots = file.read( 0x108000 );
			file.seek( 0x21000000 );
			keys = file.read( 0x400 );
			//keys = ReadFile( fi.absoluteDir().absoluteFilePath( "keys.bin" ) );

		}
		break;
	default://unsupported for this
		QMessageBox::warning( this, tr( "Error" ), tr( "I need a nand dump with ecc to create a new nand from.<br>Accepted sizes are 0x21000000 and 0x21000400." ) );
		file.close();
		return;
		break;
	}
	file.close();

	//create nandBin object to get the list of bad blocks
	NandBin old( this );
	if( !old.SetPath( fi.absoluteFilePath() ) || !old.InitNand() )
	{
		QMessageBox::warning( this, tr( "Error" ), \
							  tr( "Error reading %1." ).arg( fi.absoluteFilePath() ) );
		keys.clear();
		boots.clear();
		return;
	}
	QList<quint16> clusters = old.GetFats();
	QList<quint16> badBlacks;
	if( !clusters.size() == 0x8000 )
	{
		QMessageBox::warning( this, tr( "Error" ), \
							  tr( "Expected 0x8000 clusters from the nand, but got %1 instead!" ).arg( clusters.size(), 0, 16 ), QMessageBox::Ok );
		keys.clear();
		boots.clear();
		return;
	}
	for( quint16 i = 0; i < 0x8000; i += 8 )//first cluster of each block.
	{
		//qDebug() << hex << i << clusters.at( i );
		if( clusters.at( i ) == 0xFFFD )
		{
			quint16 block = ( i / 8 );
			badBlacks << block;
			QString txt = QString( "%1" ).arg( block );
			qDebug() << "bad cluster" << hex << i << block << txt;
			//if( ui->listWidget_badBlocks->findItems( txt, Qt::MatchExactly ).isEmpty() )//just in case, but this should always be true
				ui->listWidget_badBlocks->addItem( txt );
		}
	}
	ui->lineEdit_boot->setText( tr( "<From old nand>" ) );
	ui->lineEdit_keys->setText( tr( "<From old nand>" ) );

}
