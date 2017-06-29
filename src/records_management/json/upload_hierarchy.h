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

#ifndef _UPLOAD_HIERARCHY_H_
#define _UPLOAD_HIERARCHY_H_

#include <QByteArray>
#include <QList>
#include <QString>
#include <QStringList>

class QJsonObject; /* Forward declaration. */

/*!
 * @brief Encapsulates the upload_hierarchy response.
 */
class UploadHierarchyResp {
public:
	/*!
	 * @brief Node entry element.
	 */
	class NodeEntry {
	private:
		/*!
		 * @brief Constructor.
		 */
		NodeEntry(void);

	public:
		/*!
		 * @brief Return superordinate node,
		 *
		 * @return Superordinate node.
		 */
		const NodeEntry *super(void) const;

		/*!
		 * @brief Returns node name.
		 *
		 * @return Node name.
		 */
		const QString &name(void) const;

		/*!
		 * @brief Returns node id.
		 *
		 * @return Node id.
		 */
		const QString &id(void) const;

		/*!
		 * @brief Returns metadata.
		 *
		 * @return Stored metadata.
		 */
		const QStringList &metadata(void) const;

		/*!
		 * @brief Returns list of subordinated nodes.
		 *
		 * @return List of subordinated nodes.
		 */
		const QList<NodeEntry *> &sub(void) const;

	private:
		/*!
		 * @brief Performs a recursive (deep) copy.
		 *
		 * @param[in] root Non-null tree root.
		 * @return Copied tree on success, null pointer on error.
		 */
		static
		NodeEntry *copyRecursive(const NodeEntry *root);

		/*!
		 * @brief Recursively delete the tree of nodes.
		 *
		 * @param[in] root Root of the tree to delete.
		 */
		static
		void deleteRecursive(NodeEntry *root);

		/*!
		 * @brief Recursively constructs a hierarchy.
		 *
		 * @param[in]  jsonObj Object to be used as root of the hierarchy.
		 * @param[out] ok Set to true if no error encountered.
		 * @param[in]  acceptNullName True if null values should be accepted.
		 * @return Parsed hierarchy if \a ok set to true.
		 */
		static
		NodeEntry *fromJsonRecursive(const QJsonObject *jsonObj,
		    bool &ok, bool acceptNullName);

		/*!
		 * @brief Recursively constructs a JSON object hierarchy.
		 *
		 * @param[in,out] jsonObj JSON object to append sub-nodes to.
		 * @param[in]     uhrList List of sub-nodes to convert to JSON nodes
		 *                        from and to add as sub-nodes.
		 * @return True on success.
		 */
		static
		bool toJsonRecursive(QJsonObject *jsonObj,
		    const QList<NodeEntry *> &uhrList);

		NodeEntry *m_super; /*!< Superordinate node. */
		QString m_name; /*!< Entry name. Root entry name may be null. */
		QString m_id; /*!< Entry identifier. May be null. */
		QStringList m_metadata; /*!< Metadata. List my be empty. */
		QList<NodeEntry *> m_sub; /*!< Subordinated nodes. */

	friend class UploadHierarchyResp;
	};

public:
	/*!
	 * @brief Constructor. Creates an invalid structure.
	 */
	UploadHierarchyResp(void);

	/*!
	 * @brief Copy constructor.
	 *
	 * @param[in] uhr Upload hierarchy response.
	 */
	UploadHierarchyResp(const UploadHierarchyResp &uhr);

	/*!
	 * @brief Destructor.
	 */
	~UploadHierarchyResp(void);

	/*!
	 * @brief Returns root node.
	 *
	 * @return Root node.
	 */
	const NodeEntry *root(void) const;

	/*!
	 * @brief Check whether content is valid.
	 *
	 * @return True if content is valid.
	 */
	bool isValid(void) const;

	/*!
	 * @brief Creates a upload hierarchy structure from supplied JSON
	 *     document.
	 *
	 * @param[in]  json JSON document.
	 * @param[out] ok Set to true on success.
	 * @return Invalid structure on error a valid structure else.
	 */
	static
	UploadHierarchyResp fromJson(const QByteArray &json,
	    bool *ok = Q_NULLPTR);

	/*!
	 * @brief Converts upload hierarchy structure into a JSON document.
	 *
	 * @note Unspecified values are stores as null into the JSON document.
	 *
	 * @return JSON document containing stored data.
	 */
	QByteArray toJson(void) const;

	/*!
	 * @brief Copy assignment.
	 *
	 * @param[in] other Source.
	 * @return Destination reference.
	 */
	UploadHierarchyResp &operator=(
	    const UploadHierarchyResp &other) Q_DECL_NOTHROW;

#ifdef Q_COMPILER_RVALUE_REFS
	/* TODO -- Move assignment. */
#endif /* Q_COMPILER_RVALUE_REFS */

private:
	NodeEntry *m_root; /*!< Tree root. */
};

#endif /* _UPLOAD_HIERARCHY_H_ */
