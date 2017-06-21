/*
 * Copyright (C) 2014-2017 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 *
 * In addition, as a special exception, the copyright holders give
 * permission to link the code of portions of this program with the
 * OpenSSL library under certain conditions as described in each
 * individual source file, and distribute linked combinations including
 * the two.
 */

#include <QGraphicsSvgItem>
#include <QMessageBox>
#include <QSvgRenderer>

#include "src/document_service/gui/dlg_document_service.h"
#include "src/document_service/json/service_info.h"
#include "src/io/document_service_db.h"
#include "ui_dlg_document_service.h"

#define IGNORE_SSL_ERRORS true

/*!
 * @brief Return disabled palette.
 */
static
const QPalette &disableEditPalette(void)
{
        static QPalette palette;
        static bool prepared = false;
        if (!prepared) {
                palette.setColor(QPalette::Base, Qt::lightGray);
                palette.setColor(QPalette::Text, Qt::darkGray);
                prepared = true;
        }
        return palette;
}

DlgDocumentService::DlgDocumentService(const QString &urlStr,
    const QString &tokenStr, QWidget *parent)
    : QDialog(parent),
    m_ui(new (std::nothrow) Ui::DlgDocumentService),
    m_dsc(IGNORE_SSL_ERRORS, this),
    m_logoSvg()
{
	m_ui->setupUi(this);
	setWindowTitle(tr("Document Service Settings"));
	setUpGraphicsView();

	m_ui->urlLine->setText(urlStr);
	m_ui->tokenLine->setText(tokenStr);

	m_ui->infoButton->setEnabled(false);
	m_ui->eraseButton->setEnabled(!m_ui->urlLine->text().isEmpty() ||
	    !m_ui->tokenLine->text().isEmpty());

	m_ui->nameLine->setReadOnly(true);
	m_ui->nameLine->setPalette(disableEditPalette());
	m_ui->tokenNameLine->setReadOnly(true);
	m_ui->tokenNameLine->setPalette(disableEditPalette());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);

	connect(m_ui->urlLine, SIGNAL(textChanged(QString)),
	    this, SLOT(activateServiceButtons()));
	connect(m_ui->tokenLine, SIGNAL(textChanged(QString)),
	    this, SLOT(activateServiceButtons()));

	connect(m_ui->infoButton, SIGNAL(clicked(bool)),
	    this, SLOT(callServiceInfo()));
	connect(m_ui->eraseButton, SIGNAL(clicked(bool)),
	    this, SLOT(eraseContent()));

	connect(&m_dsc, SIGNAL(connectionError(QString)),
	    this, SLOT(notifyCommunicationError(QString)));

	loadStoredServiceInfo();
}

DlgDocumentService::~DlgDocumentService(void)
{
	delete m_ui;
}

bool DlgDocumentService::updateSettings(
    DocumentServiceSettings &docSrvcSettings, QWidget *parent)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return false;
	}

	DlgDocumentService dlg(docSrvcSettings.url, docSrvcSettings.token,
	    parent);
	if (QDialog::Accepted != dlg.exec()) {
		return false;
	}

	if (!dlg.m_ui->urlLine->text().trimmed().isEmpty()) {
		Q_ASSERT(!dlg.m_ui->tokenLine->text().trimmed().isEmpty());

		/* Update entry. */
		DocumentServiceDb::ServiceInfoEntry entry;
		entry.url = dlg.m_ui->urlLine->text().trimmed();
		entry.name = dlg.m_ui->nameLine->text();
		entry.tokenName = dlg.m_ui->tokenNameLine->text();
		entry.logoSvg = dlg.m_logoSvg;
		globDocumentServiceDbPtr->updateServiceInfo(entry);

		if (docSrvcSettings.url != dlg.m_ui->urlLine->text()) {
			/* Erase all message-related data as URL has changed. */
			globDocumentServiceDbPtr->deleteAllStoredMsg();
		}
	} else {
		Q_ASSERT(docSrvcSettings.token.isEmpty());

		globDocumentServiceDbPtr->deleteAllEntries();
	}

	/* Save changes to settings. */
	docSrvcSettings.url = dlg.m_ui->urlLine->text().trimmed();
	docSrvcSettings.token = dlg.m_ui->tokenLine->text().trimmed();

	return true;
}

void DlgDocumentService::activateServiceButtons(void)
{
	m_ui->infoButton->setEnabled(!m_ui->urlLine->text().isEmpty() &&
	    !m_ui->tokenLine->text().isEmpty());
	m_ui->eraseButton->setEnabled(!m_ui->urlLine->text().isEmpty() ||
	    !m_ui->tokenLine->text().isEmpty());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(false);
}

void DlgDocumentService::callServiceInfo(void)
{
	QByteArray response;

	m_dsc.setConnection(m_ui->urlLine->text().trimmed(),
	    m_ui->tokenLine->text().trimmed());

	if (m_dsc.communicate(DocumentServiceConnection::SRVC_SERVICE_INFO,
	        QByteArray(), response)) {
		if (!response.isEmpty()) {
			bool ok = false;
			ServiceInfoResp siRes(
			    ServiceInfoResp::fromJson(response, &ok));
			if (!ok || !siRes.isValid()) {
				QMessageBox::critical(this,
				    tr("Communication Error"),
				    tr("Received invalid response."));
				return;
			}

			setResponseContent(siRes.logoSvg(), siRes.name(),
			    siRes.tokenName());
		} else {
			QMessageBox::critical(this, tr("Communication Error"),
			    tr("Received empty response."));
			return;
		}
	} else {
		return;
	}

	m_ui->infoButton->setEnabled(false);
	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgDocumentService::eraseContent(void)
{
	m_ui->urlLine->setText(QString());
	m_ui->tokenLine->setText(QString());

	setUpGraphicsView();
	m_logoSvg = QByteArray();
	m_ui->nameLine->setText(QString());
	m_ui->tokenNameLine->setText(QString());

	m_ui->buttonBox->button(QDialogButtonBox::Ok)->setEnabled(true);
}

void DlgDocumentService::notifyCommunicationError(const QString &errMsg)
{
	QMessageBox::critical(this, tr("Communication Error"), errMsg);
}

void DlgDocumentService::setUpGraphicsView(void)
{
	QGraphicsView *gv = m_ui->graphicsView;

	gv->resize(150, 150);

	gv->setScene(new (std::nothrow) QGraphicsScene(this));
	gv->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	gv->setDragMode(QGraphicsView::ScrollHandDrag);
	gv->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

	/* Prepare background check-board pattern. */
	QPixmap tilePixmap(64, 64);
	tilePixmap.fill(Qt::white);
	QPainter tilePainter(&tilePixmap);
	QColor color(220, 220, 220);
	tilePainter.fillRect(0, 0, 32, 32, color);
	tilePainter.fillRect(32, 32, 32, 32, color);
	tilePainter.end();

	gv->setBackgroundBrush(tilePixmap);
}

/*!
 * @brief Resize graphics item to fit graphics view.
 *
 * @param[in]     gv Graphics view.
 * @param[in,out] gi Graphics item to be resized.
 */
static
void resizeGraphicsItem(const QGraphicsView *gv, QGraphicsItem *gi)
{
	if ((Q_NULLPTR == gv) || (Q_NULLPTR == gi)) {
		Q_ASSERT(0);
		return;
	}

	qreal wscale = gv->geometry().size().width() /
	    gi->boundingRect().size().width();
	qreal hscale = gv->geometry().size().height() /
	    gi->boundingRect().size().height();

	gi->setScale((wscale < hscale) ? wscale : hscale);
}

void DlgDocumentService::displaySvg(const QByteArray &svgData)
{
	if (svgData.isEmpty()) {
		return;
	}

	QGraphicsView *gv = m_ui->graphicsView;
	QGraphicsScene *s = m_ui->graphicsView->scene();

	QGraphicsSvgItem *svgItem = new (std::nothrow) QGraphicsSvgItem();
	svgItem->setObjectName(QStringLiteral("svgItem"));;
	{
		QSvgRenderer *svgRenderer =
		    new (std::nothrow) QSvgRenderer(svgData, this);
		svgItem->setSharedRenderer(svgRenderer);
	}

	s->clear();
	gv->resetTransform();

	svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
	svgItem->setCacheMode(QGraphicsItem::NoCache);
	svgItem->setZValue(0);
	resizeGraphicsItem(gv, svgItem);

#if 0
	QGraphicsRectItem *backgroundItem =
	    new (std::nothrow) QGraphicsRectItem(svgItem->boundingRect());
	backgroundItem->setBrush(Qt::white);
	backgroundItem->setPen(Qt::NoPen);
	backgroundItem->setVisible(false);
	backgroundItem->setZValue(-1);

	s->addItem(backgroundItem);
#endif
	s->addItem(svgItem);
}

void DlgDocumentService::loadStoredServiceInfo(void)
{
	if (Q_NULLPTR == globDocumentServiceDbPtr) {
		return;
	}

	DocumentServiceDb::ServiceInfoEntry entry(
	    globDocumentServiceDbPtr->serviceInfo());
	if (!entry.isValid()) {
		return;
	}

	setResponseContent(entry.logoSvg, entry.name, entry.tokenName);
}

void DlgDocumentService::setResponseContent(const QByteArray &logoSvg,
    const QString &name, const QString &tokenName)
{
	displaySvg(logoSvg);
	m_logoSvg = logoSvg;
	m_ui->nameLine->setText(name);
	m_ui->tokenNameLine->setText(tokenName);
}
