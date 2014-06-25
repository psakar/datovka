

#ifndef _DLG_VIEW_ZFO_H_
#define _DLG_VIEW_ZFO_H_


#include <QDialog>

#include "src/common.h"
#include "src/io/isds_sessions.h"
#include "ui_dlg_view_zfo.h"


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
	 * @return String containing description in HTML format.
	 */
	QString descriptionHtml(void);

	const isds_message *m_message;
};

#endif /* _DLG_VIEW_ZFO_H_ */
