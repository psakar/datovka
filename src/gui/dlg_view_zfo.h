

#ifndef _DLG_VIEW_ZFO_H_
#define _DLG_VIEW_ZFO_H_


#include <QAbstractTableModel>
#include <QDialog>

#include "src/common.h"
#include "src/io/isds_sessions.h"
#include "ui_dlg_view_zfo.h"

/*!
 * @brief Attachment table model.
 */
class AttachmentModel : public QAbstractTableModel {
	Q_OBJECT /* Not supported for nested classes. */

	friend class DlgViewZfo;
public:
	/*!
	 * @brief Constructor.
	 */
	AttachmentModel(QObject *parent = 0);

	/*!
	 * @brief Destructor.
	 */
	virtual
	~AttachmentModel(void);

	/*!
	 * @brief Returns row count.
	 */
	virtual
	int rowCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Returns column count.
	 */
	virtual
	int columnCount(const QModelIndex &parent = QModelIndex()) const;

	/*!
	 * @brief Returns data.
	 */
	virtual
	QVariant data(const QModelIndex &index,
	    int role = Qt::DisplayRole) const;

	/*!
	 * @brief Returns header data.
	 */
	virtual
	QVariant headerData(int section, Qt::Orientation orientation,
	    int role) const;

private:
	/*!
	 * @brief Set attachment model according to message content.
	 *
	 * @param[in] message Pointer to ISDS message.
	 * @return True on success.
	 */
	bool setModelData(const isds_message *message);

	static
	const QVector<QString> m_headerLabels; /*!< Header labels. */

	QVector<const struct isds_document *> m_docs; /*!< Pointers. */
};


/*!
 * @brief Dialog for ZFO content viewing.
 */
class DlgViewZfo : public QDialog, public Ui::ViewZfo {
	Q_OBJECT

public:
	/*!
	 * @brief Constructor.
	 */
	DlgViewZfo(const isds_message *isdsMsg, QWidget *parent = 0);

	/*
	 * TODO
	 * -- Signature checking.
	 * -- Callback for signature details.
	 * -- Attachment viewing.
	 * -- Callback for opening attachments.
	 */

private:
	/*!
	 * @brief Generate description from supplied message.
	 *
	 * @param[in] attachmentCount Number of attached files.
	 * @return String containing description in HTML format.
	 */
	QString descriptionHtml(int attachmentCount);

	const isds_message *m_message; /*!< ISDS message pointer copy. */
	AttachmentModel m_attachmentModel; /*!< Attachment model. */
};

#endif /* _DLG_VIEW_ZFO_H_ */
