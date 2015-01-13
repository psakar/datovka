#include "dlg_import_zfo_result.h"
#include "src/common.h"

ImportZFOResultDialog::ImportZFOResultDialog(int filesCnt,
    QList<QPair<QString,QString>> errImportList,
    QList<QPair<QString,QString>> succImportList,
    QWidget *parent) :
    QDialog(parent),
    m_filesCnt(filesCnt),
    m_errImportList(errImportList),
    m_succImportList(succImportList)
{
	setupUi(this);

	QString zfoList = "";

	this->zfoInserted->setStyleSheet("QLabel { color: green }");
	this->zfoErrors->setStyleSheet("QLabel { color: red }");
	this->zfoTotal->setText(QString::number(m_filesCnt));
	this->zfoInserted->setText(QString::number(m_succImportList.size()));
	this->zfoErrors->setText(QString::number(m_errImportList.size()));

	if (m_succImportList.size() == 0) {
		this->succListHeader->setEnabled(false);
		this->succListHeader->hide();
		this->successList->setEnabled(false);
		this->successList->hide();
	} else {
		for (int i = 0; i < m_succImportList.size(); i++) {
			zfoList += QString::number(i+1) +
			    ". <b>" + m_succImportList.at(i).first + "</b><br/>"
			    + m_succImportList.at(i).second + "<br/><br/>";
		}
		this->successList->setText(zfoList);
	}

	zfoList = "";

	if (m_errImportList.size() == 0) {
		this->errListHeader->setEnabled(false);
		this->errListHeader->hide();
		this->errorList->setEnabled(false);
		this->errorList->hide();
	} else {
		for (int i = 0; i < m_errImportList.size(); i++) {
			zfoList += QString::number(i+1) +
			    ". <b>" + m_errImportList.at(i).first + "</b><br/>"
			    + m_errImportList.at(i).second + "<br/><br/>";
		}

		this->errorList->setText(zfoList);
	}
}
