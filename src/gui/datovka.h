

#ifndef _DATOVKA_H_
#define _DATOVKA_H_


#include <QLineEdit>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QProgressBar>

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"


namespace Ui {
	class MainWindow;
}


class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow(void);

	void setDefaultProgressStatus(void);
	AccountStructInfo getAccountInfos(QModelIndex index);

	QProgressBar *m_statusProgressBar; /*!< Progressbar object. */

private slots:
	void on_actionPreferences_triggered();
	void on_actionProxy_settings_triggered();

	/*!
	 * @brief Redraws widgets according to selected account item.
	 */
	void accountItemSelectionChanged(const QModelIndex &current,
	    const QModelIndex &previous = QModelIndex());

	/*!
	 * @brief Generates menu to selected account item.
	 *     (And redraws widgets.)
	 */
	void accountItemRightClicked(const QPoint &point);

	/*!
	 * @brief Sets content of widgets according to selected message.
	 */
	void messageItemSelectionChanged(const QModelIndex &current,
	    const QModelIndex &previous = QModelIndex());

	/*!
	 * @brief Generates menu to selected message item.
	 *     (And redraws widgets.)
	 */
	void messageItemRightClicked(const QPoint &point);

	/*!
	 * @brief Redraws widgets according to selected attachment item.
	 */
	void attachmentItemSelectionChanged(const QModelIndex &current,
	    const QModelIndex &previous = QModelIndex());

	/*!
	 * @brief Generates menu to selected message item.
	 *     (And redraws widgets.)
	 */
	void attachmentItemRightClicked(const QPoint &point);

	/*!
	 * @brief Handle attachment double click.
	 */
	void attachmentItemDoubleClicked(const QModelIndex &index);

	/*!
	 * @brief Saves selected attachment to file.
	 */
	void saveSelectedAttachmentToFile(void);

	void on_actionCreate_message_triggered();

	void on_actionSent_message_triggered();

	void on_actionAdd_account_triggered();

	void on_actionDelete_account_triggered();

	void on_actionChange_password_triggered();

	void on_actionAccount_properties_triggered();

	void on_actionMove_account_up_triggered();

	void on_actionMove_account_down_triggered();

	void on_actionChange_data_directory_triggered();

	void on_actionMark_all_as_read_triggered();

	void on_actionGet_messages_triggered();

	void on_actionReply_to_the_sender_triggered();

	void on_actionFind_databox_triggered();

	void on_actionReply_triggered();

	void on_actionSearchClear_triggered(void);

	void filterMessages(const QString &text);

	void onTableColumnResized(int index, int oldSize, int newSize);

	void onTableColumnSort(int column);

	void on_actionDownload_messages_triggered();

private:

	/*!
	 * @brief Get configuration directory name.
	 */
	static
	QString confDir(void);

	/*!
	 * @brief Default settings of main window.
	 */
	void defaultUiMainWindowSettings(void) const;

	/*!
	 * @brief Create configuration file if not present.
	 */
	void ensureConfPresence(void) const;

	/*!
	 * @brief Load and apply setting from configuration file.
	 */
	void loadSettings(void);

	/*!
	 * @brief Store current setting to configuration file.
	 */
	void saveSettings(void) const;

	/*!
	 * @brief Sets geometry from settings.
	 */
	void loadWindowGeometry(const QSettings &settings);

	/*!
	 * @brief Set default account from settings.
	 */
	void setDefaultAccount(const QSettings &settings);

	/*!
	 * @brief Load sent/received messages column widths from settings.
	 */
	void loadSentReceivedMessagesColumnWidth(const QSettings &settings);

	/*!
	 * @brief Load collapse info of account items from settings.
	 */
	void loadAccountCollapseInfo(QSettings &settings);

	/*!
	 * @brief Save collapse info of account items into settings.
	 */
	void saveAccountCollapseInfo(QSettings &settings) const;

	/*!
	 * @brief Save sent/received messages column widths into settings.
	 */
	void saveSentReceivedColumnWidth(QSettings &settings) const;

	/*!
	 * @brief Set received message column widths.
	 */
	void setReciveidColumnWidths(void);

	/*!
	 * @brief Set sent message column widths.
	 */
	void setSentColumnWidths(void);

	/*!
	 * @brief Store geometry to settings.
	 */
	void saveWindowGeometry(QSettings &settings) const;

	/*!
	 * @brief Store current account user name to settings.
	 */
	void saveAccountIndex(QSettings &settings) const;

	/*!
	 * @brief Regenerates account model according to the database content.
	 */
	bool regenerateAccountModelYears(void);

	/*!
	 * @brief Generate account info HTML message.
	 */
	QString createAccountInfo(const QStandardItem &item) const;

	/*!
	 * @brief Generate overall account information.
	 */
	QString createAccountInfoAllField(const QString &accountName,
	    const QList< QPair<QString, int> > &receivedCounts,
	    const QList< QPair<QString, int> > &sent) const;

	/*!
	 * @brief Generate banner.
	 */
	QString createDatovkaBanner(const QString &version) const;

	/*!
	 * @brief Returns user name related to given account item.
	 */
	QString accountUserName(const QStandardItem *accountItem = 0) const;

	/*!
	 * @brief Get message db to selected account item.
	 */
	MessageDb * accountMessageDb(const QStandardItem *accountItem = 0);

	QString m_confDirName; /*!< Configuration directory location. */
	QString m_confFileName; /*!< Configuration file location. */
	AccountModel m_accountModel; /*!<
	                              * Account tree view model. Generated from
	                              * configuration file.
	                              */
	AccountDb m_accountDb; /*!< Account information database. */
	dbContainer m_messageDbs; /*!< Map of message databases. */
	QLineEdit *m_searchLine; /*!< Search-line object. */
	QSortFilterProxyModel m_messageListProxyModel; /*!<
	                                                * Used for message
	                                                * sorting.
	                                                */

	int m_received_1;
	int m_received_2;
	int m_sent_1;
	int m_sent_2;
	int m_sort_column;
	QString m_sort_order;

	Ui::MainWindow *ui;
};


#endif /* _DATOVKA_H_ */
