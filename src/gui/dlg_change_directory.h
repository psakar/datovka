

#ifndef _DLG_CHANGE_DIRECTORY_H_
#define _DLG_CHANGE_DIRECTORY_H_


#include <QDialog>

#include "src/common.h"
#include "ui_dlg_change_directory.h"


class DlgChangeDirectory : public QDialog, public Ui::ChangeDirectory {
    Q_OBJECT

public:
	DlgChangeDirectory(QString dirPath, QWidget *parent = 0);

private slots:
	void onDirectoryChange(void);
	void setNewDataDirectory(void);

signals:
	void sentNewPath(QString, QString, QString);

private:
	QString m_dirPath;
	void initDialog(void);
};


#endif /* _DLG_CHANGE_DIRECTORY_H_ */
