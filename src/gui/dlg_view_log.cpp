/*
 * Copyright (C) 2014-2018 CZ.NIC
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
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

#include "src/datovka_shared/log/global.h"
#include "src/datovka_shared/log/log.h"
#include "src/datovka_shared/log/memory_log.h"
#include "src/gui/dlg_view_log.h"
#include "ui_dlg_view_log.h"

DlgViewLog::DlgViewLog(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags),
    m_ui(new (std::nothrow) Ui::DlgViewLog),
    m_memLog(Q_NULLPTR)
{
	m_ui->setupUi(this);

	m_memLog = GlobInstcs::logPtr->memoryLog();
	if (m_memLog != Q_NULLPTR) {
		connect(m_memLog, SIGNAL(logged(quint64)),
		    this, SLOT(handleNewMsg(quint64)));
	}
}

DlgViewLog::~DlgViewLog(void)
{
	if (m_memLog != Q_NULLPTR) {
		m_memLog->disconnect(SIGNAL(logged(quint64)),
		    this, SLOT(handleNewMsg(quint64)));
	}

	delete m_ui;
}

void DlgViewLog::handleNewMsg(quint64 key)
{
	appendNewMsg(m_memLog->message(key));
}

void DlgViewLog::appendNewMsg(const QString &msg)
{
	m_ui->plainTextEdit->moveCursor (QTextCursor::End);
	m_ui->plainTextEdit->insertPlainText(msg);
	m_ui->plainTextEdit->moveCursor (QTextCursor::End);
}
