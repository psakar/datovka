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

#include <QObject>
#include <QScopedPointer>
#include <QString>

namespace Gov {

	/*!
	 * @brief Provides enumeration types for form field.
	 */
	class FormFieldType : public QObject {
		Q_OBJECT

	private:
		/*!
		 * @brief Private constructor.
		 */
		explicit FormFieldType(QObject *parent = Q_NULLPTR);

	public:
		/*!
		 * @brief Describes entry properties.
		 */
		enum Property {
			PROP_NONE = 0x00, /*!< Convenience value. */
			PROP_MANDATORY = 0x01, /*!< Value is mandatory and must be provided. */
			PROP_USER_INPUT = 0x02, /*!< Value is provided by the user. */
			PROP_BOX_INPUT = 0x04, /*!< Value is derived from data box owner information. */
			PROP_TYPE_DATE = 0x08 /*!< Is a date value. */
		};
		Q_DECLARE_FLAGS(Properties, Property)
#if (QT_VERSION >= QT_VERSION_CHECK(5, 5, 0))
		Q_FLAG(Properties)
#else /* < Qt-5.5 */
		Q_FLAGS(Properties)
#endif /* >= Qt-5.5 */
	};

	class FormFieldPrivate;
	/*!
	 * @brief Describes a form field.
	 */
	class FormField {
		Q_DECLARE_PRIVATE(FormField)

	public:
		FormField(void);
		FormField(const FormField &other);
#ifdef Q_COMPILER_RVALUE_REFS
		FormField(FormField &&other) Q_DECL_NOEXCEPT;
#endif /* Q_COMPILER_RVALUE_REFS */
		~FormField(void);

		FormField &operator=(const FormField &other) Q_DECL_NOTHROW;
#ifdef Q_COMPILER_RVALUE_REFS
		FormField &operator=(FormField &&other) Q_DECL_NOTHROW;
#endif /* Q_COMPILER_RVALUE_REFS */

		bool operator==(const FormField &other) const;
		bool operator!=(const FormField &other) const;

		friend void swap(FormField &first, FormField &second) Q_DECL_NOTHROW;

		bool isNull(void) const;

		const QString &key(void) const;
		void setKey(const QString &k);
#ifdef Q_COMPILER_RVALUE_REFS
		void setKey(QString &&k);
#endif /* Q_COMPILER_RVALUE_REFS */
		const QString &val(void) const;
		void setVal(const QString &v);
#ifdef Q_COMPILER_RVALUE_REFS
		void setVal(QString &&v);
#endif /* Q_COMPILER_RVALUE_REFS */
		const QString &descr(void) const;
		void setDescr(const QString &de);
#ifdef Q_COMPILER_RVALUE_REFS
		void setDescr(QString &&de);
#endif /* Q_COMPILER_RVALUE_REFS */
		const QString &placeholder(void) const;
		void setPlaceholder(const QString &p);
#ifdef Q_COMPILER_RVALUE_REFS
		void setPlaceholder(QString &&p);
#endif /* Q_COMPILER_RVALUE_REFS */
		FormFieldType::Properties properties(void) const;
		void setProperties(FormFieldType::Properties p);

	private:
		QScopedPointer<FormFieldPrivate> d_ptr; // std::unique_ptr ?
	};

	void swap(FormField &first, FormField &second) Q_DECL_NOTHROW;

	Q_DECLARE_OPERATORS_FOR_FLAGS(FormFieldType::Properties)

}
