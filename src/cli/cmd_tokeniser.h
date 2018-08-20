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

#pragma once

#include <QList>
#include <QPair>
#include <QString>

namespace CLI {

	/*!
	 * @brief Tokenises command line option command parameter.
	 *
	 * E.g. the string
	 * "dmAnnotation='Annotation',dmAttachment='test/test01.txt,test/test02.txt'"
	 * is going to be tokenised into a sequence of two pairs:
	 * "dmAnnotation" "Annotation"
	 * and
	 * "dmAttachment" "test/test01.txt,test/test02.txt".
	 */
	QList< QPair<QString, QString> > tokeniseCmdOption(const QString &opt,
	    bool *ok = Q_NULLPTR);

}
