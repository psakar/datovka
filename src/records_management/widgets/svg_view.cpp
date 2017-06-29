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
#include <QSvgRenderer>

#include "src/records_management/widgets/svg_view.h"

/*
 * See following for inspiration:
 * https://stackoverflow.com/questions/14107144/how-do-i-make-an-image-resize-to-scale-in-qt
 * https://stackoverflow.com/questions/8551690/how-to-render-a-scaled-svg-to-a-qimage
 * https://stackoverflow.com/questions/37114430/qt-qgraphicssvgitem-scaleing-and-resizeing
 * https://stackoverflow.com/questions/16013027/setting-an-image-to-a-label-in-qt
 */

SvgView::SvgView(QWidget *parent)
    : QGraphicsView(parent),
    m_svgData()
{
}

void SvgView::setSvgData(const QByteArray &svgData)
{
	m_svgData = svgData;

	setUp();

	if (!m_svgData.isEmpty()) {
		displaySvg(m_svgData);
	}
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

#define SVG_ITEM_NAME QStringLiteral("svgItem")

void SvgView::resizeEvent(QResizeEvent *event)
{
	QGraphicsView::resizeEvent(event);

	QGraphicsScene *s = this->scene();

	/* Find SVG item and resize it. */
	QGraphicsSvgItem *svgItem = Q_NULLPTR;
	foreach (QGraphicsItem *item, s->items()) {
		/* Cannot use qobject_cast() here. */
		svgItem = dynamic_cast<QGraphicsSvgItem *>(item);
		if (svgItem == Q_NULLPTR) {
			continue;
		}
		if (svgItem->objectName() == SVG_ITEM_NAME) {
			break;
		}
		svgItem = Q_NULLPTR;
	}

	if (svgItem == Q_NULLPTR) {
		return;
	}
	resizeGraphicsItem(this, svgItem);
}

void SvgView::setUp(void)
{
	setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
	setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);

	setScene(new (std::nothrow) QGraphicsScene(this));
	setTransformationAnchor(QGraphicsView::AnchorUnderMouse);
	setDragMode(QGraphicsView::ScrollHandDrag);
	setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

	/* Prepare background check-board pattern. */
	QPixmap tilePixmap(64, 64);
	tilePixmap.fill(Qt::white);
	QPainter tilePainter(&tilePixmap);
	QColor color(220, 220, 220);
	tilePainter.fillRect(0, 0, 32, 32, color);
	tilePainter.fillRect(32, 32, 32, 32, color);
	tilePainter.end();

	setBackgroundBrush(tilePixmap);
}

void SvgView::displaySvg(const QByteArray &svgData)
{
	if (svgData.isEmpty()) {
		return;
	}

	QGraphicsScene *s = this->scene();

	QGraphicsSvgItem *svgItem = new (std::nothrow) QGraphicsSvgItem();
	svgItem->setObjectName(SVG_ITEM_NAME);;
	{
		QSvgRenderer *svgRenderer =
		    new (std::nothrow) QSvgRenderer(svgData, this);
		svgItem->setSharedRenderer(svgRenderer);
	}

	s->clear();
	this->resetTransform();

	svgItem->setFlags(QGraphicsItem::ItemClipsToShape);
	svgItem->setCacheMode(QGraphicsItem::NoCache);
	svgItem->setZValue(0);
	resizeGraphicsItem(this, svgItem);

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
