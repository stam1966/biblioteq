/*
** -- Qt Includes --
*/

#include <QCryptographicHash>
#include <QInputDialog>
#include <QShortcut>
#include <QSqlField>
#include <QSqlRecord>
#include <QUuid>

/*
** Includes grey-literature-specific methods.
*/

/*
** -- Local Includes --
*/

#include "biblioteq.h"
#include "biblioteq_filesize_table_item.h"
#include "biblioteq_grey_literature.h"
#include "biblioteq_pdfreader.h"

extern biblioteq *qmain;

biblioteq_grey_literature::biblioteq_grey_literature(QMainWindow *parentArg,
						     const QString &oidArg,
						     const int rowArg):
  QMainWindow(), biblioteq_item(rowArg)
{
  QMenu *menu = 0;

  if((menu = new(std::nothrow) QMenu(this)) == 0)
    biblioteq::quit("Memory allocation failure", __FILE__, __LINE__);

  m_duplicate = false;
  m_isQueryEnabled = false;
  m_oid = oidArg;
  m_parentWid = parentArg;
  m_row = rowArg;
  m_ui.setupUi(this);
  m_ui.resetButton->setMenu(menu);
  connect(m_ui.attach_files,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotAttachFiles(void)));
  connect(m_ui.cancelButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotCancel(void)));
  connect(m_ui.delete_files,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotDeleteFiles(void)));
  connect(m_ui.files,
	  SIGNAL(itemDoubleClicked(QTableWidgetItem *)),
	  this,
	  SLOT(slotFilesDoubleClicked(QTableWidgetItem *)));
  connect(m_ui.export_files,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotExportFiles(void)));
  connect(m_ui.okButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotGo(void)));
  connect(m_ui.printButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotPrint(void)));
  connect(m_ui.resetButton,
	  SIGNAL(clicked(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Title")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset ID")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Date")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Authors")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Clients")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Code-A")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Code-B")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Job Number")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Notes")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Location")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Status")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
  connect(menu->addAction(tr("Reset Type")),
	  SIGNAL(triggered(void)),
	  this,
	  SLOT(slotReset(void)));
#ifdef Q_OS_MAC
#if QT_VERSION < 0x050000
  setAttribute(Qt::WA_MacMetalStyle, BIBLIOTEQ_WA_MACMETALSTYLE);
#endif
#endif
  new (std::nothrow) QShortcut(QKeySequence(Qt::CTRL + Qt::Key_S),
			       this,
			       SLOT(slotGo(void)));
  QApplication::setOverrideCursor(Qt::WaitCursor);
  m_ui.files->setColumnHidden(m_ui.files->columnCount() - 1, true); // myoid

  QString errorstr("");

  m_ui.date->setDisplayFormat(qmain->publicationDateFormat("greyliterature"));
  m_ui.date_enabled->setVisible(false);
  m_ui.location->addItems
    (biblioteq_misc_functions::getLocations(qmain->getDB(),
					    "Grey Literature",
					    errorstr));
  qmain->addError
    (QString(tr("Database Error")),
     QString(tr("Unable to retrieve the grey literature locations.")),
     errorstr, __FILE__, __LINE__);
  m_ui.type->addItems
    (biblioteq_misc_functions::getGreyLiteratureTypes(qmain->getDB(),
						      errorstr));
  qmain->addError
    (QString(tr("Database Error")),
     QString(tr("Unable to retrieve the grey literature document types.")),
     errorstr, __FILE__, __LINE__);

  if(m_ui.location->findText(tr("UNKNOWN")) == -1)
    m_ui.location->addItem(tr("UNKNOWN"));

  if(m_ui.type->findText(tr("UNKNOWN")) == -1)
    m_ui.type->addItem(tr("UNKNOWN"));

  QApplication::restoreOverrideCursor();
  m_dt_orig_ss = m_ui.date->styleSheet();
  m_te_orig_pal = m_ui.author->viewport()->palette();
  updateFont(QApplication::font(), qobject_cast<QWidget *> (this));
  biblioteq_misc_functions::center(this, m_parentWid);
  biblioteq_misc_functions::hideAdminFields(this, qmain->getRoles());
}

biblioteq_grey_literature::~biblioteq_grey_literature()
{
}

bool biblioteq_grey_literature::validateWidgets(void)
{
  QString error("");

  m_ui.title->setText(m_ui.title->text().trimmed());

  if(m_ui.title->text().isEmpty())
    {
      error = tr("Please complete the Title field.");
      m_ui.title->setFocus();
      goto done_label;
    }

  m_ui.id->setText(m_ui.id->text().trimmed());

  if(m_ui.id->text().isEmpty())
    {
      error = tr("Please complete the ID field.");
      m_ui.id->setFocus();
      goto done_label;
    }

  m_ui.author->setPlainText(m_ui.author->toPlainText().trimmed());

  if(m_ui.author->toPlainText().isEmpty())
    {
      error = tr("Please complete the Authors field.");
      m_ui.author->setFocus();
      goto done_label;
    }

  m_ui.code_a->setText(m_ui.code_a->text().trimmed());

  if(m_ui.code_a->text().isEmpty())
    {
      error = tr("Please complete the Code-A field.");
      m_ui.code_a->setFocus();
      goto done_label;
    }

  m_ui.code_b->setText(m_ui.code_b->text().trimmed());

  if(m_ui.code_b->text().isEmpty())
    {
      error = tr("Please complete the Code-B field.");
      m_ui.code_b->setFocus();
      goto done_label;
    }

  m_ui.job_number->setText(m_ui.job_number->text().trimmed());

  if(m_ui.job_number->text().isEmpty())
    {
      error = tr("Please complete the Job Number field.");
      m_ui.job_number->setFocus();
      goto done_label;
    }

  m_ui.client->setPlainText(m_ui.client->toPlainText().trimmed());
  m_ui.notes->setPlainText(m_ui.notes->toPlainText().trimmed());
  m_ui.status->setText(m_ui.status->text().trimmed());
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!qmain->getDB().transaction())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError
	(QString(tr("Database Error")),
	 QString(tr("Unable to create a database transaction.")),
	 qmain->getDB().lastError().text(),
	 __FILE__,
	 __LINE__);
      QMessageBox::critical
	(this,
	 tr("BiblioteQ: Database Error"),
	 tr("Unable to create a database transaction."));
      return false;
    }

  QApplication::restoreOverrideCursor();

 done_label:

  if(!error.isEmpty())
    {
      QMessageBox::critical(this, tr("BiblioteQ: User Error"), error);
      return false;
    }

  return true;
}

void biblioteq_grey_literature::changeEvent(QEvent *event)
{
  if(event)
    switch(event->type())
      {
      case QEvent::LanguageChange:
	{
	  m_ui.retranslateUi(this);
	  break;
	}
      default:
	break;
      }

  QMainWindow::changeEvent(event);
}

void biblioteq_grey_literature::closeEvent(QCloseEvent *e)
{
  if(m_engWindowTitle.contains("Create") ||
     m_engWindowTitle.contains("Modify"))
    if(hasDataChanged(this))
      if(QMessageBox::
	 question(this,
		  tr("BiblioteQ: Question"),
		  tr("Your changes have not been saved. Continue closing?"),
		  QMessageBox::Yes | QMessageBox::No,
		  QMessageBox::No) == QMessageBox::No)
	{
	  if(e)
	    e->ignore();

	  return;
	}

  qmain->removeGreyLiterature(this);
}

void biblioteq_grey_literature::createFile(const QByteArray &bytes,
					   const QByteArray &digest,
					   const QString &fileName) const
{
  QSqlQuery query(qmain->getDB());

  if(qmain->getDB().driverName() != "QSQLITE")
    query.prepare("INSERT INTO grey_literature_files "
		  "(file, file_digest, file_name, item_oid) "
		  "VALUES (?, ?, ?, ?)");
  else
    query.prepare("INSERT INTO grey_literature_files "
		  "(file, file_digest, file_name, item_oid, myoid) "
		  "VALUES (?, ?, ?, ?, ?)");

  query.addBindValue(bytes);
  query.addBindValue(digest.toHex().constData());
  query.addBindValue(fileName);
  query.addBindValue(m_oid);

  if(qmain->getDB().driverName() == "QSQLITE")
    {
      QString errorstr("");
      qint64 value = biblioteq_misc_functions::getSqliteUniqueId
	(qmain->getDB(), errorstr);

      if(errorstr.isEmpty())
	query.addBindValue(value);
      else
	qmain->addError(QString(tr("Database Error")),
			QString(tr("Unable to generate a unique integer.")),
			errorstr);
    }

  if(!query.exec())
    qmain->addError
      (QString(tr("Database Error")),
       QString(tr("Unable to create a database transaction.")),
       query.lastError().text(),
       __FILE__,
       __LINE__);
}

void biblioteq_grey_literature::duplicate(const QString &p_oid, const int state)
{
  m_duplicate = true;
  modify(state); // Initial population.
  m_duplicate = false;
  m_engWindowTitle = "Create";
  m_oid = p_oid;
  m_ui.attach_files->setEnabled(false);
  m_ui.delete_files->setEnabled(false);
  m_ui.export_files->setEnabled(false);
  setWindowTitle(tr("BiblioteQ: Duplicate Grey Literature Entry"));
}

void biblioteq_grey_literature::highlightRequiredWidgets(void)
{
  biblioteq_misc_functions::highlightWidget
    (m_ui.author->viewport(), QColor(255, 248, 220));
  biblioteq_misc_functions::highlightWidget(m_ui.code_a, QColor(255, 248, 220));
  biblioteq_misc_functions::highlightWidget(m_ui.code_b, QColor(255, 248, 220));
  biblioteq_misc_functions::highlightWidget(m_ui.id, QColor(255, 248, 220));
  biblioteq_misc_functions::highlightWidget
    (m_ui.job_number, QColor(255, 248, 220));
  biblioteq_misc_functions::highlightWidget(m_ui.title, QColor(255, 248, 220));
}

void biblioteq_grey_literature::insert(void)
{
  m_engWindowTitle = "Create";
  m_te_orig_pal = m_ui.id->palette();
  m_ui.attach_files->setEnabled(false);
  m_ui.author->setPlainText("N/A");
  m_ui.client->clear();
  m_ui.code_a->setText("N/A");
  m_ui.code_b->setText("N/A");
  m_ui.date->setDate(QDate::fromString("01/01/2000", "MM/dd/yyyy"));
  m_ui.delete_files->setEnabled(false);
  m_ui.export_files->setEnabled(false);
  m_ui.id->setText(QUuid::createUuid().toString().remove("{").remove("}"));
  m_ui.job_number->setText("N/A");
  m_ui.location->setCurrentIndex(0);
  m_ui.notes->clear();
  m_ui.okButton->setText(tr("&Save"));
  m_ui.status->clear();
  m_ui.title->clear();
  m_ui.title->setFocus();
  m_ui.type->setCurrentIndex(0);
  highlightRequiredWidgets();
  setWindowTitle(tr("BiblioteQ: Create Grey Literature Entry"));
  storeData(this);
  showNormal();
  activateWindow();
  raise();
}

void biblioteq_grey_literature::insertDatabase(void)
{
  QSqlQuery query(qmain->getDB());

  if(qmain->getDB().driverName() != "QSQLITE")
    query.prepare
      ("INSERT INTO grey_literature "
       "(author, client, document_code_a, document_code_b, "
       "document_date, document_id, document_status, document_title, "
       "document_type, job_number, location, notes) "
       "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?) "
       "RETURNING myoid");
  else
    query.prepare
      ("INSERT INTO grey_literature "
       "(author, client, document_code_a, document_code_b, "
       "document_date, document_id, document_status, document_title, "
       "document_type, job_number, location, notes, "
       "myoid) "
       "VALUES (?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?, ?)");

  query.addBindValue(m_ui.author->toPlainText());

  if(m_ui.client->toPlainText().isEmpty())
    query.addBindValue(QVariant::String);
  else
    query.addBindValue(m_ui.client->toPlainText());

  query.addBindValue(m_ui.code_a->text());
  query.addBindValue(m_ui.code_b->text());
  query.addBindValue(m_ui.date->date().toString("MM/dd/yyyy"));
  query.addBindValue(m_ui.id->text());
  query.addBindValue(m_ui.status->text());
  query.addBindValue(m_ui.title->text());
  query.addBindValue(m_ui.type->currentText());
  query.addBindValue(m_ui.job_number->text());
  query.addBindValue(m_ui.location->currentText());

  if(m_ui.notes->toPlainText().isEmpty())
    query.addBindValue(QVariant::String);
  else
    query.addBindValue(m_ui.notes->toPlainText());

  if(qmain->getDB().driverName() == "QSQLITE")
    {
      QString errorstr("");
      qint64 value = biblioteq_misc_functions::getSqliteUniqueId
	(qmain->getDB(), errorstr);

      if(errorstr.isEmpty())
	{
	  m_oid = QString::number(value);
	  query.addBindValue(value);
	}
      else
	qmain->addError(QString(tr("Database Error")),
			QString(tr("Unable to generate a unique integer.")),
			errorstr);
    }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!query.exec())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError
	(QString(tr("Database Error")),
	 QString(tr("Unable to create the entry.")),
	 query.lastError().text(),
	 __FILE__,
	 __LINE__);
      goto db_rollback;
    }

  if(qmain->getDB().driverName() != "QSQLITE")
    {
      query.next();
      m_oid = query.value(0).toString();
    }

  if(!qmain->getDB().commit())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError
	(QString(tr("Database Error")),
	 QString(tr("Unable to commit the current database transaction.")),
	 qmain->getDB().lastError().text(),
	 __FILE__,
	 __LINE__);
      goto db_rollback;
    }

  m_ui.author->setMultipleLinks
    ("greyliterature_search", "author", m_ui.author->toPlainText());
  m_ui.client->setMultipleLinks
    ("greyliterature_search", "client", m_ui.client->toPlainText());
  m_ui.notes->setMultipleLinks
    ("greyliterature_search", "notes", m_ui.notes->toPlainText());
  QApplication::restoreOverrideCursor();
  qmain->replaceGreyLiterature(m_oid, this);
  updateWindow(biblioteq::EDITABLE);

  if(qmain->getUI().actionAutoPopulateOnCreation->isChecked())
    qmain->populateTable
      (biblioteq::POPULATE_ALL, "Grey Literature", QString(""));

  raise();
  storeData(this);
  return;

 db_rollback:

  QApplication::setOverrideCursor(Qt::WaitCursor);
  m_oid.clear();

  if(!qmain->getDB().rollback())
    qmain->addError(QString(tr("Database Error")),
		    QString(tr("Rollback failure.")),
		    qmain->getDB().lastError().text(),
		    __FILE__,
		    __LINE__);

  QApplication::restoreOverrideCursor();
  QMessageBox::critical(this,
			tr("BiblioteQ: Database Error"),
			tr("Unable to create the entry. Please verify that "
			   "the entry does not already exist."));
}

void biblioteq_grey_literature::modify(const int state)
{
  if(state == biblioteq::EDITABLE)
    {
      highlightRequiredWidgets();
      setReadOnlyFields(this, false);
      setWindowTitle(tr("BiblioteQ: Modify Grey Literature Entry"));
      m_engWindowTitle = "Modify";
      m_te_orig_pal = m_ui.id->palette();
      m_ui.attach_files->setEnabled(true);
      m_ui.delete_files->setEnabled(true);
      m_ui.export_files->setEnabled(true);
      m_ui.okButton->setText("&Save");
      m_ui.okButton->setVisible(true);
      m_ui.resetButton->setVisible(true);
    }
  else
    {
      setReadOnlyFields(this, true);
      setWindowTitle(tr("BiblioteQ: View Grey Literature Details"));
      m_engWindowTitle = "View";
      m_ui.attach_files->setVisible(false);
      m_ui.delete_files->setVisible(false);
      m_ui.export_files->setEnabled(true);
      m_ui.okButton->setVisible(false);
      m_ui.okButton->setVisible(false);
      m_ui.resetButton->setVisible(false);
    }

  QSqlQuery query(qmain->getDB());

  query.prepare("SELECT author, "
		"client, "
		"document_code_a, "
		"document_code_b, "
		"document_date, "
		"document_id, "
		"document_status, "
		"document_title, "
		"document_type, "
		"job_number, "
		"location, "
		"notes "
		"FROM grey_literature WHERE myoid = ?");
  query.addBindValue(m_oid);
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!query.exec() || !query.next())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError
	(QString(tr("Database Error")),
	 QString(tr("Unable to retrieve the selected grey literature's data.")),
	 query.lastError().text(),
	 __FILE__,
	 __LINE__);
      QMessageBox::critical
	(this,
	 tr("BiblioteQ: Database Error"),
	 tr("Unable to retrieve the selected grey literature's data."));
      m_ui.title->setFocus();
      return;
    }
  else
    {
      QApplication::restoreOverrideCursor();
      showNormal();
      activateWindow();
      raise();

      QSqlRecord record(query.record());

      for(int i = 0; i < record.count(); i++)
	{
	  QString fieldName(record.fieldName(i));
	  QVariant variant(record.field(i).value());

	  if(fieldName == "author")
	    m_ui.author->setMultipleLinks
	      ("greyliterature_search", "author", variant.toString());
	  else if(fieldName == "client")
	    m_ui.client->setMultipleLinks
	      ("greyliterature_search", "client", variant.toString());
	  else if(fieldName == "document_code_a")
	    m_ui.code_a->setText(variant.toString());
	  else if(fieldName == "document_code_b")
	    m_ui.code_b->setText(variant.toString());
	  else if(fieldName == "document_date")
	    m_ui.date->setDate
	      (QDate::fromString(variant.toString(), "MM/dd/yyyy"));
	  else if(fieldName == "document_id")
	    {
	      QString string("");

	      if(state == biblioteq::EDITABLE)
		{
		  if(!variant.toString().trimmed().isEmpty())
		    string = tr("BiblioteQ: Modify Grey Literature Entry (") +
		      variant.toString() +
		      tr(")");
		  else
		    string = tr("BiblioteQ: Modify Grey Literature Entry");
		}
	      else
		{
		  if(!variant.toString().trimmed().isEmpty())
		    string = tr("BiblioteQ: View Grey Literature Details (") +
		      variant.toString() +
		      tr(")");
		  else
		    string = tr("BiblioteQ: View Grey Literature Details");
		}

	      m_ui.id->setText(variant.toString());
	      setWindowTitle(string);
	    }
	  else if(fieldName == "document_status")
	    m_ui.status->setText(variant.toString());
	  else if(fieldName == "document_title")
	    m_ui.title->setText(variant.toString());
	  else if(fieldName == "document_type")
	    {
	      if(m_ui.type->findText(variant.toString()) > -1)
		m_ui.type->setCurrentIndex
		  (m_ui.type->findText(variant.toString()));
	      else
		m_ui.type->setCurrentIndex(0);
	    }
	  else if(fieldName == "job_number")
	    m_ui.job_number->setText(variant.toString());
	  else if(fieldName == "location")
	    {
	      if(m_ui.location->findText(variant.toString()) > -1)
		m_ui.location->setCurrentIndex
		  (m_ui.location->findText(variant.toString()));
	      else
		m_ui.location->setCurrentIndex(0);
	    }
	  else if(fieldName == "notes")
	    m_ui.notes->setMultipleLinks
	      ("greyliterature_search", "notes", variant.toString());
	}

      foreach(QLineEdit *textfield, findChildren<QLineEdit *> ())
	textfield->setCursorPosition(0);

      storeData(this);

      if(!m_duplicate)
	populateFiles();
    }

  m_ui.title->setFocus();
}

void biblioteq_grey_literature::populateFiles(void)
{
  m_ui.files->setRowCount(0);
  m_ui.files->setSortingEnabled(false);

  QSqlQuery query(qmain->getDB());
  int count = 0;

  query.prepare
    ("SELECT COUNT(*) FROM grey_literature_files WHERE item_oid = ?");
  query.addBindValue(m_oid);

  if(query.exec())
    if(query.next())
      {
	count = query.value(0).toInt();
	m_ui.files->setRowCount(count);
      }

  query.prepare("SELECT file_name, "
		"file_digest, "
		"LENGTH(file) AS f_s, "
		"description, "
		"myoid FROM grey_literature_files "
                "WHERE item_oid = ? ORDER BY file_name");
  query.addBindValue(m_oid);
  QApplication::setOverrideCursor(Qt::WaitCursor);

  QLocale locale;
  int row = 0;
  int totalRows = 0;

  if(query.exec())
    while(query.next() && totalRows < m_ui.files->rowCount())
      {
	totalRows += 1;

	QSqlRecord record(query.record());

	for(int i = 0; i < record.count(); i++)
	  {
	    QTableWidgetItem *item = 0;

	    if(record.fieldName(i) == "f_s")
	      item = new(std::nothrow) biblioteq_filesize_table_item
		(locale.toString(query.value(i).toLongLong()));
	    else
	      item = new(std::nothrow)
		QTableWidgetItem(query.value(i).toString());

	    if(!item)
	      continue;

	    item->setData
	      (Qt::UserRole, query.value(record.count() - 1).toLongLong());
	    item->setFlags(Qt::ItemIsEnabled | Qt::ItemIsSelectable);

	    if(m_engWindowTitle == "Modify")
	      if(record.fieldName(i) == "description")
		item->setToolTip(tr("Double-click to edit."));

	    m_ui.files->setItem(row, i, item);
	  }

	row += 1;
      }

  m_ui.files->horizontalHeader()->setSortIndicator(0, Qt::AscendingOrder);
  m_ui.files->setRowCount(totalRows);
  m_ui.files->setSortingEnabled(true);

  if((qmain->getTypeFilterString() == "All" ||
      qmain->getTypeFilterString() == "All Available" ||
      qmain->getTypeFilterString() == "All Overdue" ||
      qmain->getTypeFilterString() == "All Requested" ||
      qmain->getTypeFilterString() == "All Reserved" ||
      qmain->getTypeFilterString() == "Grey Literature") &&
     m_oid == biblioteq_misc_functions::getColumnString
     (qmain->getUI().table,
      m_row,
      qmain->getUI().table->columnNumber("MYOID")) &&
     biblioteq_misc_functions::getColumnString
     (qmain->getUI().table,
      m_row,
      qmain->getUI().table->columnNumber("Type")) == "Grey Literature")
    {
      qmain->getUI().table->setSortingEnabled(false);

      QStringList names(qmain->getUI().table->columnNames());

      for(int i = 0; i < names.size(); i++)
	if(names.at(i) == "File Count")
	  {
	    qmain->getUI().table->item(m_row, i)->
	      setText(QString::number(count));
	    break;
	  }

      qmain->getUI().table->setSortingEnabled(true);
    }

  QApplication::restoreOverrideCursor();
}

void biblioteq_grey_literature::search(const QString &field,
				       const QString &value)
{
  m_ui.attach_files->setVisible(false);
  m_ui.date->setDate(QDate::fromString("2001", "yyyy"));
  m_ui.date->setDisplayFormat("yyyy");
  m_ui.date_enabled->setVisible(true);
  m_ui.delete_files->setVisible(false);
  m_ui.export_files->setVisible(false);
  m_ui.files->setVisible(false);
  m_ui.files_label->setVisible(false);
  m_ui.location->insertItem(0, tr("Any"));
  m_ui.location->setCurrentIndex(0);
  m_ui.okButton->setText(tr("&Search"));
  m_ui.type->insertItem(0, tr("Any"));
  m_ui.type->setCurrentIndex(0);

  if(field.isEmpty() && value.isEmpty())
    {
      m_engWindowTitle = "Search";
      m_ui.title->setFocus();
      setWindowTitle(tr("BiblioteQ: Database Grey Literature Search"));
      biblioteq_misc_functions::center(this, m_parentWid);
      showNormal();
      activateWindow();
      raise();
    }
  else
    {
      if(field == "author")
	m_ui.author->setPlainText(value);
      else if(field == "client")
	m_ui.client->setPlainText(value);
      else if(field == "notes")
	m_ui.notes->setPlainText(value);

      slotGo();
    }
}

void biblioteq_grey_literature::slotAttachFiles(void)
{
  QFileDialog fileDialog
    (this, tr("BiblioteQ: Grey Literature File Attachment(s)"));

  fileDialog.setAcceptMode(QFileDialog::AcceptOpen);
  fileDialog.setDirectory(QDir::homePath());
  fileDialog.setFileMode(QFileDialog::ExistingFiles);
  fileDialog.setOption(QFileDialog::DontUseNativeDialog);

  if(fileDialog.exec() == QDialog::Accepted)
    {
      repaint();
#ifndef Q_OS_MAC
      QApplication::processEvents();
#endif

      QProgressDialog progress(this);
      QStringList files(fileDialog.selectedFiles());
      int i = -1;

#ifdef Q_OS_MAC
#if QT_VERSION < 0x050000
      progress.setAttribute(Qt::WA_MacMetalStyle, BIBLIOTEQ_WA_MACMETALSTYLE);
#endif
#endif
      progress.setLabelText(tr("Uploading files..."));
      progress.setMaximum(files.size());
      progress.setMinimum(0);
      progress.setModal(true);
      progress.setWindowTitle(tr("BiblioteQ: Progress Dialog"));
      progress.show();
      progress.repaint();
#ifndef Q_OS_MAC
      QApplication::processEvents();
#endif

      while(i++, !files.isEmpty() && !progress.wasCanceled())
	{
	  QCryptographicHash digest(QCryptographicHash::Sha1);
	  QFile file;
	  QString fileName(files.takeFirst());

	  file.setFileName(fileName);

	  if(file.open(QIODevice::ReadOnly))
	    {
	      QByteArray bytes(4096, 0);
	      QByteArray total;
	      qint64 rc = 0;

	      while((rc = file.read(bytes.data(), bytes.size())) > 0)
		{
		  digest.addData(bytes.mid(0, static_cast<int> (rc)));
		  total.append(bytes.mid(0, static_cast<int> (rc)));
		}

	      if(!total.isEmpty())
		{
		  total = qCompress(total, 9);
		  createFile
		    (total, digest.result(), QFileInfo(fileName).fileName());
		}
	    }

	  file.close();

	  if(i + 1 <= progress.maximum())
	    progress.setValue(i + 1);

	  progress.repaint();
#ifndef Q_OS_MAC
	  QApplication::processEvents();
#endif
	}

      QApplication::restoreOverrideCursor();
      populateFiles();
    }
}

void biblioteq_grey_literature::slotCancel(void)
{
  close();
}

void biblioteq_grey_literature::slotDeleteFiles(void)
{
  QModelIndexList list
    (m_ui.files->selectionModel()->
     selectedRows(m_ui.files->columnCount() - 1)); // myoid

  if(list.isEmpty())
    {
      QMessageBox::critical
	(this, tr("BiblioteQ: User Error"),
	 tr("Please select at least one file to delete."));
      return;
    }

  if(QMessageBox::question(this, tr("BiblioteQ: Question"),
			   tr("Are you sure that you wish to delete the "
			      "selected file(s)?"),
			   QMessageBox::Yes | QMessageBox::No,
			   QMessageBox::No) == QMessageBox::No)
    {
      list.clear();
      return;
    }

  QApplication::setOverrideCursor(Qt::WaitCursor);

  while(!list.isEmpty())
    {
      QSqlQuery query(qmain->getDB());

      query.prepare("DELETE FROM grey_literature_files WHERE "
		    "item_oid = ? AND myoid = ?");
      query.addBindValue(m_oid);
      query.addBindValue(list.takeFirst().data());
      query.exec();
    }

  QApplication::restoreOverrideCursor();
  populateFiles();
}

void biblioteq_grey_literature::slotExportFiles(void)
{
  QModelIndexList list(m_ui.files->selectionModel()->
		       selectedRows(m_ui.files->columnCount() - 1)); // myoid

  if(list.isEmpty())
    return;

  QFileDialog dialog(this);

#ifdef Q_OS_MAC
#if QT_VERSION < 0x050000
  dialog.setAttribute(Qt::WA_MacMetalStyle, BIBLIOTEQ_WA_MACMETALSTYLE);
#endif
#endif
  dialog.setDirectory(QDir::homePath());
  dialog.setFileMode(QFileDialog::Directory);
  dialog.setOption(QFileDialog::DontUseNativeDialog);
  dialog.setWindowTitle(tr("BiblioteQ: Grey Literature File Export"));
  dialog.exec();

  if(dialog.result() != QDialog::Accepted)
    return;

  repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif

  QProgressDialog progress(this);

#ifdef Q_OS_MAC
#if QT_VERSION < 0x050000
  progress.setAttribute(Qt::WA_MacMetalStyle, BIBLIOTEQ_WA_MACMETALSTYLE);
#endif
#endif
  progress.setLabelText(tr("Exporting file(s)..."));
  progress.setMaximum(list.size());
  progress.setMinimum(0);
  progress.setModal(true);
  progress.setWindowTitle(tr("BiblioteQ: Progress Dialog"));
  progress.show();
  progress.repaint();
#ifndef Q_OS_MAC
  QApplication::processEvents();
#endif

  int i = -1;

  while(i++, !list.isEmpty() && !progress.wasCanceled())
    {
      QSqlQuery query(qmain->getDB());

      query.prepare("SELECT file, file_name FROM grey_literature_files "
		    "WHERE item_oid = ? AND myoid = ?");
      query.addBindValue(m_oid);
      query.addBindValue(list.takeFirst().data());

      if(query.exec() && query.next())
	{
	  QFile file(dialog.selectedFiles().value(0) +
		     QDir::separator() +
		     query.value(1).toString());

	  if(file.open(QIODevice::WriteOnly))
	    file.write(qUncompress(query.value(0).toByteArray()));

	  file.flush();
	  file.close();
	}

      if(i + 1 <= progress.maximum())
	progress.setValue(i + 1);

      progress.repaint();
#ifndef Q_OS_MAC
      QApplication::processEvents();
#endif
    }
}


void biblioteq_grey_literature::slotFilesDoubleClicked(QTableWidgetItem *item)
{
  if(!item)
    return;

  if(item->column() != 3 || m_engWindowTitle != "Modify")
    {
      QTableWidgetItem *item1 = m_ui.files->item(item->row(), 0); // File

      if(!item1)
	return;

#ifdef BIBLIOTEQ_LINKED_WITH_POPPLER
      if(item1->text().toLower().trimmed().endsWith(".pdf"))
	{
	  QApplication::setOverrideCursor(Qt::WaitCursor);

	  QByteArray data;
	  QSqlQuery query(qmain->getDB());

	  query.prepare("SELECT file, file_name FROM grey_literature_files "
			"WHERE item_oid = ? AND myoid = ?");
	  query.addBindValue(m_oid);
	  query.addBindValue(item1->data(Qt::UserRole).toLongLong());

	  if(query.exec() && query.next())
	    data = qUncompress(query.value(0).toByteArray());

	  if(!data.isEmpty())
	    {
	      biblioteq_pdfreader *reader =
		new(std::nothrow) biblioteq_pdfreader(this);

	      if(reader)
		{
		  reader->load(data, item1->text());
		  biblioteq_misc_functions::center(reader, this);
		  reader->show();
		}
	    }

	  QApplication::restoreOverrideCursor();
	}
#endif

      return;
    }

  if(m_engWindowTitle != "Modify")
    return;

  QTableWidgetItem *item1 = m_ui.files->item(item->row(), 3); // Description

  if(!item1)
    return;

  QString description(item1->text());
  QTableWidgetItem *item2 =
    m_ui.files->item(item->row(), m_ui.files->columnCount() - 1); // myoid

  if(!item2)
    return;

  QString text("");
  bool ok = true;

  text = QInputDialog::getText(this,
			       tr("BiblioteQ: File Description"),
			       tr("Description"),
			       QLineEdit::Normal,
			       description,
			       &ok).trimmed();

  if(!ok)
    return;

  QSqlQuery query(qmain->getDB());
  QString myoid(item2->text());

  query.prepare("UPDATE grey_literature_files SET description = ? "
		"WHERE item_oid = ? AND myoid = ?");
  query.addBindValue(text);
  query.addBindValue(m_oid);
  query.addBindValue(myoid);

  if(query.exec())
    item1->setText(text);
}

void biblioteq_grey_literature::slotGo(void)
{
  if(m_engWindowTitle.contains("Create"))
    {
      if(validateWidgets())
	insertDatabase();
    }
  else if(m_engWindowTitle.contains("Modify"))
    {
      if(validateWidgets())
	updateDatabase();
    }
  else if(m_engWindowTitle.contains("Search"))
    {
      QString searchstr("");

      searchstr = "SELECT DISTINCT grey_literature.author, "
	"grey_literature.client, "
	"grey_literature.document_code_a, "
	"grey_literature.document_code_b, "
	"grey_literature.document_date, "
	"grey_literature.document_id, "
	"grey_literature.document_status, "
	"grey_literature.document_title, "
	"grey_literature.document_type, "
	"grey_literature.job_number, "
	"grey_literature.location, "
	"(SELECT COUNT(myoid) FROM grey_literature_files "
	"WHERE grey_literature_files.item_oid = grey_literature.myoid) "
	"AS file_count, "
	"grey_literature.type, "
	"grey_literature.myoid, "
	"grey_literature.front_cover "
	"FROM grey_literature WHERE ";

      QString E("");

      if(qmain->getDB().driverName() != "QSQLITE")
	E = "E";

      searchstr.append("LOWER(document_title) LIKE LOWER(" +
		       E +
		       "'%" +
		       biblioteq_myqstring::
		       escape(m_ui.title->text().trimmed()) +
		       "%') AND ");
      searchstr.append("LOWER(document_id) LIKE LOWER(" +
		       E +
		       "'%" +
		       biblioteq_myqstring::
		       escape(m_ui.id->text().trimmed()) +
		       "%') AND ");

      if(m_ui.date_enabled->isChecked())
	searchstr.append("SUBSTR(document_date, 7) = '" +
			 m_ui.date->date().toString("yyyy") +
			 "' AND ");

      searchstr.append
	("LOWER(author) LIKE LOWER(" +
	 E +
	 "'%" +
	 biblioteq_myqstring::escape(m_ui.author->toPlainText().trimmed()) +
	 "%') AND ");
      searchstr.append
	("LOWER(COALESCE(client, '')) LIKE LOWER(" +
	 E +
	 "'%" +
	 biblioteq_myqstring::escape(m_ui.client->toPlainText().trimmed()) +
	 "%') AND ");
      searchstr.append
	("LOWER(document_code_a) LIKE LOWER(" +
	 E +
	 "'%" +
	 biblioteq_myqstring::escape(m_ui.code_a->text().trimmed()) +
	 "%') AND ");
      searchstr.append
	("LOWER(document_code_b) LIKE LOWER(" +
	 E +
	 "'%" +
	 biblioteq_myqstring::escape(m_ui.code_b->text().trimmed()) +
	 "%') AND ");
      searchstr.append
	("LOWER(job_number) LIKE LOWER(" +
	 E +
	 "'%" +
	 biblioteq_myqstring::escape(m_ui.job_number->text().trimmed()) +
	 "%') AND ");
      searchstr.append
	("LOWER(COALESCE(notes, '')) LIKE LOWER(" +
	 E +
	 "'%" +
	 biblioteq_myqstring::escape(m_ui.notes->toPlainText().trimmed()) +
	 "%') AND ");

      if(m_ui.location->currentIndex() != 0)
	searchstr.append("location = '" +
			 m_ui.location->currentText().trimmed() +
			 "' AND ");

      searchstr.append
	("LOWER(COALESCE(document_status, '')) LIKE LOWER(" +
	 E +
	 "'%" +
	 biblioteq_myqstring::escape(m_ui.status->text().trimmed()) +
	 "%') ");

      if(m_ui.type->currentIndex() != 0)
	searchstr.append
	  ("AND document_type = '" +
	   m_ui.type->currentText().trimmed() +
	   "' ");

      /*
      ** Search the database.
      */

      (void) qmain->populateTable
	(biblioteq::POPULATE_SEARCH, "Grey Literature", searchstr);
    }
}

void biblioteq_grey_literature::slotPrint(void)
{
  m_html = "<html>";

  QStringList titles;
  QStringList values;

  titles << tr("Title:")
	 << tr("ID:")
	 << tr("Date:")
	 << tr("Authors:")
	 << tr("Clients:")
	 << tr("Code-A:")
	 << tr("Code-B:")
	 << tr("Job Number:")
	 << tr("Notes:")
	 << tr("Location:")
	 << tr("Status:")
	 << tr("Type:");
  values << m_ui.title->text().trimmed()
	 << m_ui.id->text().trimmed()
	 << m_ui.date->date().toString(Qt::ISODate)
	 << m_ui.author->toPlainText().trimmed()
	 << m_ui.client->toPlainText().trimmed()
	 << m_ui.code_a->text().trimmed()
	 << m_ui.code_b->text().trimmed()
	 << m_ui.job_number->text().trimmed()
	 << m_ui.notes->toPlainText().trimmed()
	 << m_ui.location->currentText().trimmed()
	 << m_ui.status->text().trimmed()
	 << m_ui.type->currentText().trimmed();

  for(int i = 0; i < titles.size(); i++)
    {
      m_html += "<b>" + titles.at(i) + "</b>" + values.at(i);

      if(i != titles.size() - 1)
	m_html += "<br>";
    }

  m_html += "</html>";
  print(this);
}

void biblioteq_grey_literature::slotPublicationDateEnabled(bool state)
{
  Q_UNUSED(state);
}

void biblioteq_grey_literature::slotQuery(void)
{
}

void biblioteq_grey_literature::slotReset(void)
{
  QAction *action = qobject_cast<QAction *> (sender());

  if(action != 0)
    {
      QList<QAction *> actions = m_ui.resetButton->menu()->actions();

      if(actions.size() < 12)
	{
	  // Error.
	}
      else if(action == actions[0])
	{
	  m_ui.title->clear();
	  m_ui.title->setPalette(m_te_orig_pal);
	  m_ui.title->setFocus();
	}
      else if(action == actions[1])
	{
	  m_ui.id->clear();
	  m_ui.id->setPalette(m_te_orig_pal);
	  m_ui.id->setFocus();
	}
      else if(action == actions[2])
	{
	  if(m_engWindowTitle.contains("Search"))
	    m_ui.date->setDate(QDate::fromString("2001", "yyyy"));
	  else
	    m_ui.date->setDate(QDate::fromString("01/01/2000", "MM/dd/yyyy"));

	  m_ui.date->setFocus();
	  m_ui.date->setStyleSheet(m_dt_orig_ss);
	  m_ui.date_enabled->setChecked(false);
	}
      else if(action == actions[3])
	{
	  if(!m_engWindowTitle.contains("Search"))
	    m_ui.author->setPlainText("N/A");
	  else
	    m_ui.author->clear();

	  m_ui.author->setFocus();
	  m_ui.author->viewport()->setPalette(m_te_orig_pal);
	}
      else if(action == actions[4])
	{
	  m_ui.client->clear();
	  m_ui.client->setFocus();
	}
      else if(action == actions[5])
	{
	  if(!m_engWindowTitle.contains("Search"))
	    m_ui.code_a->setText("N/A");
	  else
	    m_ui.code_a->clear();

	  m_ui.code_a->setFocus();
	  m_ui.code_a->setPalette(m_te_orig_pal);
	}
      else if(action == actions[6])
	{
	  if(!m_engWindowTitle.contains("Search"))
	    m_ui.code_b->setText("N/A");
	  else
	    m_ui.code_b->clear();

	  m_ui.code_b->setFocus();
	  m_ui.code_b->setPalette(m_te_orig_pal);
	}
      else if(action == actions[7])
	{
	  if(!m_engWindowTitle.contains("Search"))
	    m_ui.job_number->setText("N/A");
	  else
	    m_ui.job_number->clear();

	  m_ui.job_number->setFocus();
	  m_ui.job_number->setPalette(m_te_orig_pal);
	}
      else if(action == actions[8])
	{
	  m_ui.notes->clear();
	  m_ui.notes->setFocus();
	}
      else if(action == actions[9])
	{
	  m_ui.location->setCurrentIndex(0);
	  m_ui.location->setFocus();
	}
      else if(action == actions[10])
	{
	  m_ui.status->clear();
	  m_ui.status->setFocus();
	}
      else if(action == actions[11])
	{
	  m_ui.type->setCurrentIndex(0);
	  m_ui.type->setFocus();
	}
    }
  else
    {
      /*
      ** Reset all.
      */

      if(!m_engWindowTitle.contains("Search"))
	m_ui.author->setPlainText("N/A");
      else
	m_ui.author->clear();

      m_ui.client->clear();

      if(!m_engWindowTitle.contains("Search"))
	m_ui.code_a->setText("N/A");
      else
	m_ui.code_a->clear();

      if(!m_engWindowTitle.contains("Search"))
	m_ui.code_b->setText("N/A");
      else
	m_ui.code_b->clear();

      if(m_engWindowTitle.contains("Search"))
	m_ui.date->setDate(QDate::fromString("2001", "yyyy"));
      else
	m_ui.date->setDate(QDate::fromString("01/01/2000", "MM/dd/yyyy"));

      m_ui.date_enabled->setChecked(false);
      m_ui.id->clear();

      if(!m_engWindowTitle.contains("Search"))
	m_ui.job_number->setText("N/A");
      else
	m_ui.job_number->clear();

      m_ui.location->setCurrentIndex(0);
      m_ui.notes->clear();
      m_ui.status->clear();
      m_ui.title->clear();
      m_ui.title->setFocus();
      m_ui.type->setCurrentIndex(0);
    }
}

void biblioteq_grey_literature::updateDatabase(void)
{
  QSqlQuery query(qmain->getDB());
  QString string("");

  query.prepare("UPDATE grey_literature SET "
		"author = ?, "
		"client = ?, "
		"document_code_a = ?, "
		"document_code_b = ?, "
		"document_date = ?, "
		"document_id = ?, "
		"document_status = ?, "
		"document_title = ?, "
		"document_type = ?, "
		"job_number = ?, "
		"location = ?, "
		"notes = ? "
		"WHERE myoid = ?");

  query.addBindValue(m_ui.author->toPlainText());

  if(m_ui.client->toPlainText().isEmpty())
    query.addBindValue(QVariant::String);
  else
    query.addBindValue(m_ui.client->toPlainText());

  query.addBindValue(m_ui.code_a->text());
  query.addBindValue(m_ui.code_b->text());
  query.addBindValue(m_ui.date->date().toString("MM/dd/yyyy"));
  query.addBindValue(m_ui.id->text());
  query.addBindValue(m_ui.status->text());
  query.addBindValue(m_ui.title->text());
  query.addBindValue(m_ui.type->currentText());
  query.addBindValue(m_ui.job_number->text());
  query.addBindValue(m_ui.location->currentText());

  if(m_ui.notes->toPlainText().isEmpty())
    query.addBindValue(QVariant::String);
  else
    query.addBindValue(m_ui.notes->toPlainText());

  query.addBindValue(m_oid);
  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!query.exec())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError
	(QString(tr("Database Error")),
	 QString(tr("Unable to update the entry.")),
	 query.lastError().text(),
	 __FILE__,
	 __LINE__);
      goto db_rollback;
    }

  if(!qmain->getDB().commit())
    {
      QApplication::restoreOverrideCursor();
      qmain->addError
	(QString(tr("Database Error")),
	 QString(tr("Unable to commit the current database transaction.")),
	 qmain->getDB().lastError().text(),
	 __FILE__,
	 __LINE__);
      goto db_rollback;
    }

  m_ui.author->setMultipleLinks
    ("greyliterature_search", "author", m_ui.author->toPlainText());
  m_ui.client->setMultipleLinks
    ("greyliterature_search", "client", m_ui.client->toPlainText());
  m_ui.notes->setMultipleLinks
    ("greyliterature_search", "notes", m_ui.notes->toPlainText());
  QApplication::restoreOverrideCursor();

  if(!m_ui.id->text().isEmpty())
    string = tr("BiblioteQ: Modify Grey Literature Entry (") +
      m_ui.id->text() + tr(")");
  else
    string = tr("BiblioteQ: Modify Grey Literature Entry");

  setWindowTitle(string);

  if((qmain->getTypeFilterString() == "All" ||
      qmain->getTypeFilterString() == "All Available" ||
      qmain->getTypeFilterString() == "All Overdue" ||
      qmain->getTypeFilterString() == "All Requested" ||
      qmain->getTypeFilterString() == "All Reserved" ||
      qmain->getTypeFilterString() == "Grey Literature") &&
     m_oid == biblioteq_misc_functions::
     getColumnString(qmain->getUI().table,
		     m_row,
		     qmain->getUI().table->columnNumber("MYOID")) &&
     biblioteq_misc_functions::getColumnString
     (qmain->getUI().table,
      m_row,
      qmain->getUI().table->columnNumber("Type")) == "Grey Literature")
    {
      qmain->getUI().table->setSortingEnabled(false);

      QStringList names(qmain->getUI().table->columnNames());

      for(int i = 0; i < names.size(); i++)
	{
	  QString string("");
	  bool set = false;

	  if(names.at(i) == "Accession Number" || names.at(i) == "Job Number")
	    {
	      set = true;
	      string = m_ui.job_number->text();
	    }
	  else if(names.at(i) == "Author" ||
		  names.at(i) == "Authors" ||
		  names.at(i) == "Publisher")
	    {
	      set = true;
	      string = m_ui.author->toPlainText();
	    }
	  else if(names.at(i) == "Clients")
	    {
	      set = true;
	      string = m_ui.client->toPlainText();
	    }
	  else if(names.at(i) == "Date" ||
		  names.at(i) == "Document Date" ||
		  names.at(i) == "Publication Date")
	    {
	      set = true;
	      string = m_ui.date->date().toString(Qt::ISODate);
	    }
	  else if(names.at(i) == "Document Code A")
	    {
	      set = true;
	      string = m_ui.code_a->text();
	    }
	  else if(names.at(i) == "Document Code B")
	    {
	      set = true;
	      string = m_ui.code_b->text();
	    }
	  else if(names.at(i) == "Document ID" ||
		  names.at(i) == "ID" ||
		  names.at(i) == "ID Number")
	    {
	      set = true;
	      string = m_ui.id->text();
	    }
	  else if(names.at(i) == "Document Status")
	    {
	      set = true;
	      string = m_ui.status->text();
	    }
	  else if(names.at(i) == "Document Type")
	    {
	      set = true;
	      string = m_ui.type->currentText();
	    }
	  else if(names.at(i) == "Location")
	    {
	      set = true;
	      string = m_ui.location->currentText();
	    }
	  else if(names.at(i) == "Title")
	    {
	      set = true;
	      string = m_ui.title->text();
	    }

	  if(set)
	    qmain->getUI().table->item(m_row, i)->setText(string);
	}

      qmain->getUI().table->setSortingEnabled(true);

      foreach(QLineEdit *textfield, findChildren<QLineEdit *> ())
	textfield->setCursorPosition(0);

      qmain->slotResizeColumns();
    }

  qmain->slotDisplaySummary();
  storeData(this);
  return;

 db_rollback:

  QApplication::setOverrideCursor(Qt::WaitCursor);

  if(!qmain->getDB().rollback())
    qmain->addError(QString(tr("Database Error")),
		    QString(tr("Rollback failure.")),
		    qmain->getDB().lastError().text(),
		    __FILE__,
		    __LINE__);

  QApplication::restoreOverrideCursor();
  QMessageBox::critical(this,
			tr("BiblioteQ: Database Error"),
			tr("Unable to update the entry."));
}

void biblioteq_grey_literature::updateWindow(const int state)
{
  QString string("");

  if(state == biblioteq::EDITABLE)
    {
      m_engWindowTitle = "Modify";
      m_ui.attach_files->setEnabled(true);
      m_ui.delete_files->setEnabled(true);
      m_ui.export_files->setEnabled(true);
      m_ui.okButton->setVisible(true);
      m_ui.resetButton->setVisible(true);
      string = QString(tr("BiblioteQ: Modify Grey Literature Entry (")) +
	m_ui.id->text() + tr(")");
    }
  else
    {
      m_engWindowTitle = "View";
      m_ui.attach_files->setVisible(false);
      m_ui.delete_files->setVisible(false);
      m_ui.export_files->setEnabled(true);
      m_ui.okButton->setVisible(false);
      m_ui.resetButton->setVisible(false);
      string = QString(tr("BiblioteQ: View Grey Literature Details (")) +
	m_ui.id->text() + tr(")");
    }

  setReadOnlyFields(this, state != biblioteq::EDITABLE);
  setWindowTitle(string);
}
