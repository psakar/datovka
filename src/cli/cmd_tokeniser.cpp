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

#include <QChar>
#include <QStringRef>

#include "src/cli/cmd_tokeniser.h"

/* QStringRef::operator[] is avaliable from Qt-5.7. */

/*!
 * @brief Returns s list of substrings which have been divided by separator.
 *
 * @note The string
 * "dmAnnotation='Annotation',dmAttachment='test/test01.txt,test/test02.txt'"
 * is going to be divided into
 * "dmAnnotation='Annotation'" and "dmAttachment='test/test01.txt,test/test02.txt'".
 *
 * @param[in]  sep Separator character.
 * @param[in]  strRef String reference to be parsed.
 * @param[in]  ignoreEmpty Set to true if empty content should be ignored.
 * @param[out] ok Set to false on failure.
 */
static
QList<QStringRef> separatedItemsQuotes(const QChar &sep,
    const QStringRef &strRef, bool ignoreEmpty, bool *ok = Q_NULLPTR)
{
	if (Q_UNLIKELY(strRef.isEmpty())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return QList<QStringRef>();
	}

	const QChar quote('\'');
	QList<QStringRef> refList;

	int start = 0;
	int stop = 0;
	bool inSingeQuotes = false;
	while (stop < strRef.size()) {
		const QChar ch(strRef.at(stop));

		if (ch == quote) {
			/* Start or end of single quotes. */
			inSingeQuotes = !inSingeQuotes;
		} else if (ch == sep) {
			if (!inSingeQuotes) {
				/* Found comma outside single quotes. */
				QStringRef ref(strRef.mid(start, stop - start).trimmed());
				if (!ignoreEmpty || !ref.isEmpty()) {
					refList.append(ref);
				}
				start = stop;
				++start;
			}
		}

		++stop;
	}

	if (inSingeQuotes) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return QList<QStringRef>();
	} else if ((start < strRef.size()) || (strRef.at(strRef.size() - 1) == sep)) {
		QStringRef ref(strRef.mid(start).trimmed());
		if (!ignoreEmpty || !ref.isEmpty()) {
			refList.append(ref);
		}
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return refList;
}

/*!
 * @brief Returns s list of substring which have been created by separating the
 *     string using the quotation marks.
 *
 * @note The string
 * "'test/test01.txt,test/test02.txt'foo'bar'"
 * is going to be divided into
 * "test/test01.txt,test/test02.txt" "foo" and "bar".
 *
 * @param[in]  strRef String reference to be parsed.
 * @param[in]  ignoreEmpty Set to true if empty content should be ignored.
 * @param[out] ok Set to false on failure.
 */
static
QList<QStringRef> blocksQuotes(const QStringRef &strRef, bool ignoreEmpty,
    bool *ok = Q_NULLPTR)
{
	if (Q_UNLIKELY(strRef.isEmpty())) {
		if (ok != Q_NULLPTR) {
			*ok = true;
		}
		return QList<QStringRef>();
	}

	const QChar quote('\'');
	QList<QStringRef> refList;

	int start = 0;
	int stop = 0;
	bool inSingeQuotes = false;
	while (stop < strRef.size()) {
		const QChar ch(strRef.at(stop));

		if (ch == quote) {
			QStringRef ref(strRef.mid(start, stop - start).trimmed());
			if (inSingeQuotes || (start != stop)) {
				/* Always ignore empty space between quotes. */
				if (!ignoreEmpty || !ref.isEmpty()) {
					refList.append(ref);
				}
			}
			start = stop;
			++start;

			inSingeQuotes = !inSingeQuotes;
		}

		++stop;
	}

	if (inSingeQuotes) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return QList<QStringRef>();
	} else if (start < strRef.size()) {
		QStringRef ref(strRef.mid(start).trimmed());
		if (!ignoreEmpty || !ref.isEmpty()) {
			refList.append(ref);
		}
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return refList;
}

typedef QPair<QString, QString> TokenPair;
typedef QList<TokenPair> TokenPairList;

TokenPairList CLI::tokeniseCmdOption(const QString &opt, bool *ok)
{
	if (Q_UNLIKELY(opt.isEmpty())) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return TokenPairList();
	}

	bool iOk = false;
	const QList<QStringRef> separatedOpts(
	    separatedItemsQuotes(QChar(','), opt.midRef(0), true, &iOk));
	if (!iOk) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return TokenPairList();
	}

	TokenPairList tokenPairList;

	foreach (const QStringRef &optRef, separatedOpts) {
		const QList<QStringRef> optPair(separatedItemsQuotes(QChar('='),
		    optRef, false, &iOk));
		if ((optPair.size() != 2) || !iOk) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return TokenPairList();
		}

		const QList<QStringRef> optName(blocksQuotes(optPair.at(0), false));
		const QList<QStringRef> optVal(blocksQuotes(optPair.at(1), false));

		if ((optName.size() != 1) || (optVal.size() != 1)) {
			if (ok != Q_NULLPTR) {
				*ok = false;
			}
			return TokenPairList();
		}

		tokenPairList.append(
		    TokenPair(optName.at(0).toString(), optVal.at(0).toString()));
	}

	if (ok != Q_NULLPTR) {
		*ok = true;
	}
	return tokenPairList;
}
