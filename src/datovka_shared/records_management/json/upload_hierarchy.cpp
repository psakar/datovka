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

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>

#include "src/datovka_shared/records_management/json/helper.h"
#include "src/datovka_shared/records_management/json/upload_hierarchy.h"

static
const QString keyName("name");
static
const QString keyId("id");
static
const QString keyMetadata("metadata");
static
const QString keySub("sub");

UploadHierarchyResp::NodeEntry::NodeEntry(void)
    : m_super(Q_NULLPTR),
    m_name(),
    m_id(),
    m_metadata(),
    m_sub()
{
}

const UploadHierarchyResp::NodeEntry *UploadHierarchyResp::NodeEntry::super(void) const
{
	return m_super;
}

const QString &UploadHierarchyResp::NodeEntry::name(void) const
{
	return m_name;
}

const QString &UploadHierarchyResp::NodeEntry::id(void) const
{
	return m_id;
}

const QStringList &UploadHierarchyResp::NodeEntry::metadata(void) const
{
	return m_metadata;
}

const QList<UploadHierarchyResp::NodeEntry *> &UploadHierarchyResp::NodeEntry::sub(void) const
{
	return m_sub;
}

UploadHierarchyResp::NodeEntry *UploadHierarchyResp::NodeEntry::copyRecursive(
    const NodeEntry *root)
{
	if (root == Q_NULLPTR) {
		Q_ASSERT(0);
		return Q_NULLPTR;
	}

	NodeEntry *newRoot = new (std::nothrow) NodeEntry();
	if (newRoot == Q_NULLPTR) {
		return Q_NULLPTR;
	}

	newRoot->m_name = root->m_name;
	newRoot->m_id = root->m_id;
	newRoot->m_metadata = root->m_metadata;

	foreach (const NodeEntry *sub, root->m_sub) {
		if (sub == Q_NULLPTR) {
			Q_ASSERT(0);
			deleteRecursive(newRoot);
			return Q_NULLPTR;
		}
		NodeEntry *newSub = copyRecursive(sub);
		if (newSub == Q_NULLPTR) {
			deleteRecursive(newRoot);
			return Q_NULLPTR;
		}

		newSub->m_super = newRoot;
		newRoot->m_sub.append(newSub);
	}

	return newRoot;
}

void UploadHierarchyResp::NodeEntry::deleteRecursive(NodeEntry *root)
{
	if (root == Q_NULLPTR) {
		return;
	}

	foreach (NodeEntry *entry, root->m_sub) {
		deleteRecursive(entry);
	}

	delete root;
}

UploadHierarchyResp::NodeEntry *UploadHierarchyResp::NodeEntry::fromJsonRecursive(
    const QJsonObject *jsonObj, bool &ok, bool acceptNullName)
{
	if (jsonObj == Q_NULLPTR) {
		Q_ASSERT(0);
		ok = false;
		return Q_NULLPTR;
	}

	NodeEntry *newEntry = new (std::nothrow) NodeEntry();
	if (newEntry == Q_NULLPTR) {
		ok = false;
		return Q_NULLPTR;
	}

	if (!JsonHelper::readString(*jsonObj, keyName, newEntry->m_name,
	        acceptNullName)) {
		ok = false;
		deleteRecursive(newEntry);
		return Q_NULLPTR;
	}
	if (!JsonHelper::readString(*jsonObj, keyId, newEntry->m_id, true)) {
		ok = false;
		deleteRecursive(newEntry);
		return Q_NULLPTR;
	}
	if (!JsonHelper::readStringList(*jsonObj, keyMetadata,
	        newEntry->m_metadata, false)) {
		ok = false;
		deleteRecursive(newEntry);
		return Q_NULLPTR;
	}

	{
		QJsonArray jsonArr;
		if (!JsonHelper::readArray(*jsonObj, keySub, jsonArr, false)) {
			ok = false;
			deleteRecursive(newEntry);
			return Q_NULLPTR;
		}

		foreach (const QJsonValue &jsonVal, jsonArr) {
			if (!jsonVal.isObject()) {
				qCritical("%s",
				    "Sub-node array holds a non-object value.");
				ok = false;
				deleteRecursive(newEntry);
				return Q_NULLPTR;
			}

			QJsonObject jsonSubObj(jsonVal.toObject());
			NodeEntry *subNewEntry =
			    fromJsonRecursive(&jsonSubObj, ok, false);
			if (!ok) {
				/* ok = false; */
				deleteRecursive(newEntry);
				return Q_NULLPTR;
			}
			Q_ASSERT(subNewEntry != Q_NULLPTR);

			subNewEntry->m_super = newEntry;
			newEntry->m_sub.append(subNewEntry);
		}
	}

	ok = true;
	return newEntry;
}

/*!
 * @brief Append a JSON object into sub-nodes.
 *
 * @param[in,out] jsonNode Not to append a sub-node to.
 * @param[in]     jsnoSubNode Sub-node to be appended.
 * @return True on success, false else.
 */
static
bool jsonHierarchyAppendSub(QJsonObject &jsonNode,
    const QJsonObject &jsonSubNode)
{
	QJsonObject::iterator it(jsonNode.find(keySub));
	if (it == jsonNode.end()) {
		return false;
	}

	QJsonValueRef arrRef(it.value());
	if (!arrRef.isArray()) {
		return false;
	}

	/* Sub-node must have a name. */
	{
		QJsonObject::const_iterator sit(jsonSubNode.constFind(keyName));
		if ((sit == jsonSubNode.constEnd()) ||
		    !sit.value().isString() ||
		    sit.value().toString().isEmpty()) {
			return false;
		}
	}

#if 0
	/* QJsonArray::operator+() is not available in Qt 5.2. */
	arrRef = arrRef.toArray() + jsonSubNode;
#else
	QJsonArray arr(arrRef.toArray());
	arr.append(jsonSubNode);
	arrRef = arr;
#endif
	return true;
}

/*!
 * @brief Creates a JSON object representing an upload hierarchy node with
 *     empty sub-node list.
 *
 * @param[in] name Node name.
 * @param[in] id Node identifier.
 * @param[in] metadata List of metadata.
 * @return JSON object.
 */
static
QJsonObject jsonHierarchyNode(const QString &name, const QString &id,
    const QStringList &metadata)
{
	QJsonObject jsonObj;
	/* Intentionally using isEmpty() instead of isNull(). */
	jsonObj.insert(keyName, !name.isEmpty() ? name : QJsonValue());
	jsonObj.insert(keyId, !id.isEmpty() ? id : QJsonValue());
	jsonObj.insert(keyMetadata, QJsonArray::fromStringList(metadata));
	jsonObj.insert(keySub, QJsonArray());

	return jsonObj;
}

bool UploadHierarchyResp::NodeEntry::toJsonRecursive(QJsonObject *jsonObj,
    const QList<NodeEntry *> &uhrList)
{
	if (jsonObj == Q_NULLPTR) {
		Q_ASSERT(0);
		return false;
	}

	foreach (const NodeEntry *entry, uhrList) {
		if (entry == Q_NULLPTR) {
			Q_ASSERT(0);
			return false;
		}
		QJsonObject jsonSubObj(jsonHierarchyNode(entry->m_name,
		    entry->m_id, entry->m_metadata));
		if (!toJsonRecursive(&jsonSubObj, entry->m_sub)) {
			return false;
		}
		if (!jsonHierarchyAppendSub(*jsonObj, jsonSubObj)) {
			return false;
		}
	}

	return true;
}

UploadHierarchyResp::UploadHierarchyResp(void)
    : m_root(Q_NULLPTR)
{
}

UploadHierarchyResp::UploadHierarchyResp(const UploadHierarchyResp &uhr)
    : m_root(Q_NULLPTR)
{
	if (uhr.m_root != Q_NULLPTR) {
		m_root = NodeEntry::copyRecursive(uhr.m_root);
	}
}

UploadHierarchyResp::~UploadHierarchyResp(void)
{
	NodeEntry::deleteRecursive(m_root);
}

const UploadHierarchyResp::NodeEntry *UploadHierarchyResp::root(void) const
{
	return m_root;
}

bool UploadHierarchyResp::isValid(void) const
{
	return m_root != Q_NULLPTR;
}

UploadHierarchyResp UploadHierarchyResp::fromJson(const QByteArray &json,
    bool *ok)
{
	QJsonObject jsonObj;
	if (!JsonHelper::readRootObject(json, jsonObj)) {
		if (ok != Q_NULLPTR) {
			*ok = false;
		}
		return UploadHierarchyResp();
	}

	bool intOk = false;
	UploadHierarchyResp uhr;

	uhr.m_root = NodeEntry::fromJsonRecursive(&jsonObj, intOk, true);
	if (ok != Q_NULLPTR) {
		*ok = intOk;
	}
	Q_ASSERT(uhr.m_root != Q_NULLPTR);
	return intOk ? uhr : UploadHierarchyResp();
}

QByteArray UploadHierarchyResp::toJson(void) const
{
	if (m_root == Q_NULLPTR) {
		Q_ASSERT(0);
		return QJsonDocument(QJsonObject()).toJson(
		    QJsonDocument::Indented);
	}

	QJsonObject jsonObj(jsonHierarchyNode(m_root->m_name,
	    m_root->m_id, m_root->m_metadata));

	if (!NodeEntry::toJsonRecursive(&jsonObj, m_root->m_sub)) {
		return QByteArray();
	}

	return QJsonDocument(jsonObj).toJson(QJsonDocument::Indented);
}

UploadHierarchyResp &UploadHierarchyResp::operator=(
    const UploadHierarchyResp &other) Q_DECL_NOTHROW
{
	NodeEntry *tmpRoot = Q_NULLPTR;

	if (other.m_root != Q_NULLPTR) {
		tmpRoot = NodeEntry::copyRecursive(other.m_root);
		if (tmpRoot == Q_NULLPTR) {
			/* Copying failed. */
			Q_ASSERT(0);
		}
	}

	NodeEntry::deleteRecursive(m_root);
	m_root = tmpRoot;

	return *this;
}
