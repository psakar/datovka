#include "dlg_import_zfo_result.h"
#include "src/common.h"

ImportZFOResultDialog::ImportZFOResultDialog(int filesCnt, int imported,
    QList<QPair<QString,QString>> errImportList, QWidget *parent) :
    QDialog(parent),
    m_filesCnt(filesCnt),
    m_imported(imported),
    m_errImportList(errImportList)
{
	setupUi(this);

	this->zfoInserted->setStyleSheet("QLabel { color: green }");
	this->zfoErrors->setStyleSheet("QLabel { color: red }");
	this->zfoTotal->setText(QString::number(m_filesCnt));
	this->zfoInserted->setText(QString::number(m_imported));
	this->zfoErrors->setText(QString::number(m_errImportList.size()));

	QString zfoList = "";

	for (int i = 0; i < m_errImportList.size(); i++) {

		zfoList += QString::number(i+1) +
		    ". <b>" + m_errImportList.at(i).first + "</b><br/>" +
		    m_errImportList.at(i).second + "<br/><br/>";
	}

	this->errorList->setText(zfoList);
}
