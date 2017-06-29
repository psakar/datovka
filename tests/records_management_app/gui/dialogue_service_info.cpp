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
//#include <QScopedPointer>
#include <QSvgRenderer>

#include "tests/records_management_app/gui/dialogue_service_info.h"
#include "ui_dialogue_service_info.h"

DialogueServiceInfo::DialogueServiceInfo(const ServiceInfoResp &sir,
    QWidget *parent)
    : QDialog(parent),
    m_ui(new Ui::DialogueServiceInfo)
{
	m_ui->setupUi(this);

	setWindowTitle("Service Info Response");

	m_ui->graphicsView->resize(100, m_ui->graphicsView->height());

	m_ui->nameLine->setText(sir.name());
	m_ui->nameLine->setReadOnly(true);
	m_ui->tokenNameLine->setText(sir.tokenName());
	m_ui->tokenNameLine->setReadOnly(true);

	setupGraphicsView();
	loadSvg(sir.logoSvg());
}

DialogueServiceInfo::~DialogueServiceInfo(void)
{
	delete m_ui;
}

void DialogueServiceInfo::setupGraphicsView(void)
{
	QGraphicsView *gv = m_ui->graphicsView;

	gv->setScene(new (std::nothrow) QGraphicsScene(this));
	gv->setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	gv->setDragMode(QGraphicsView::ScrollHandDrag);
	gv->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

	// Prepare background check-board pattern
	QPixmap tilePixmap(64, 64);
	tilePixmap.fill(Qt::white);
	QPainter tilePainter(&tilePixmap);
	QColor color(220, 220, 220);
	tilePainter.fillRect(0, 0, 32, 32, color);
	tilePainter.fillRect(32, 32, 32, 32, color);
	tilePainter.end();

	gv->setBackgroundBrush(tilePixmap);
}

void DialogueServiceInfo::loadSvg(const QByteArray &svgData)
{
	if (svgData.isEmpty()) {
		return;
	}

	QGraphicsView *gv = m_ui->graphicsView;
	QGraphicsScene *s = m_ui->graphicsView->scene();

	QGraphicsSvgItem *svgItem = new (std::nothrow) QGraphicsSvgItem();
	{
		QSvgRenderer *svgRenderer = new (std::nothrow) QSvgRenderer(svgData, this);
		svgItem->setSharedRenderer(svgRenderer);
	}

	s->clear();
	gv->resetTransform();

	svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
	svgItem->setCacheMode(QGraphicsItem::NoCache);
	svgItem->setZValue(0);

	QGraphicsRectItem *backgroundItem = new (std::nothrow) QGraphicsRectItem(svgItem->boundingRect());
	backgroundItem->setBrush(Qt::white);
	backgroundItem->setPen(Qt::NoPen);
	backgroundItem->setVisible(false);
	backgroundItem->setZValue(-1);

	s->addItem(backgroundItem);
	s->addItem(svgItem);
}
