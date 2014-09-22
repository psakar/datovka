

#ifndef _DATOVKA_H_
#define _DATOVKA_H_


#include <QLineEdit>
#include <QMainWindow>
#include <QSortFilterProxyModel>
#include <QStandardItemModel>
#include <QProgressBar>
#include <QThread>
#include <QPushButton>

#include "src/common.h"
#include "src/io/account_db.h"
#include "src/io/message_db.h"
#include "src/models/accounts_model.h"
#include "thread/worker.h"

namespace Ui {
	class MainWindow;
}


class MainWindow : public QMainWindow {
	Q_OBJECT

public:
	explicit MainWindow(QWidget *parent = 0);
	~MainWindow(void);

	void setDefaultProgressStatus(void);

	QProgressBar *m_statusProgressBar; /*!< Progress-bar object. */

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

	/*!
	 * @brief Open attachment in default application.
	 */
	void openSelectedAttachment(void);

	/*!
	 * @brief Downloads the attachments for the selected message.
	 */
	void downloadSelectedMessageAttachments(void);

	/*!
	 * @brief Mark all messages as read in selected account item.
	 */
	void accountItemMarkAllRead(void);

	/*!
	 * @brief Deletes selected message from message list.
	 */
	void messageItemDeleteMessage(void);

	/*!
	 * @brief Downloads new messages from server for all accounts.
	 */
	void synchroniseAllAccounts(void);

	/*!
	 * @brief Downloads new messages from server for selected account.
	 */
	void synchroniseSelectedAccount(void);

	/*!
	 * @brief Creates and sends new message.
	 */
	void createAndSendMessage(void);

	void on_actionAdd_account_triggered(void);

	void on_actionDelete_account_triggered(void);

	void on_actionChange_password_triggered(void);

	void on_actionAccount_properties_triggered(void);

	void on_actionMove_account_up_triggered(void);

	void on_actionMove_account_down_triggered(void);

	void on_actionChange_data_directory_triggered(void);

	void on_actionReply_to_the_sender_triggered(void);

	void on_actionFind_databox_triggered(void);

	void on_actionReply_triggered(void);

	void on_actionSearchClear_triggered(void);

	void filterMessages(const QString &text);

	void onTableColumnResized(int index, int oldSize, int newSize);

	void onTableColumnSort(int column);

	void on_actionDownload_message_signed_triggered(void);

	void on_actionAbout_Datovka_triggered(void);

	void on_actionImport_database_directory_triggered(void);

	void on_actionOpen_attachment_triggered(void);

	void on_actionSave_attachment_triggered(void);

	void on_actionAuthenticate_message_file_triggered(void);

	void on_actionAuthenticate_message_triggered(void);

	void on_actionVerify_a_message_triggered(void);

	void on_actionView_message_from_ZPO_file_triggered(void);

	void on_actionHepl_triggered(void);

	void on_actionExport_as_ZFO_triggered(void);

	void on_actionExport_delivery_info_as_ZFO_triggered(void);

	void on_actionExport_delivery_info_as_PDF_triggered(void);

	void on_actionExport_message_envelope_as_PDF_triggered(void);

	void on_actionOpen_message_externally_triggered(void);

	void on_actionOpen_delivery_info_externally_triggered(void);

	void on_actionSignature_detail_triggered(void);

	/*!
	 * @brief Delete worker and thread objects, enable sync buttons.
	 */
	void deleteThreadSyncAll(void);

	/*!
	 * @brief Delete worker and thread objects, enable sync buttons.
	 */
	void deleteThreadSyncOne(void);

	/*!
	 * @brief Set ProgressBar value and Status bar text.
	 */
	void setProgressBarFromWorker(QString label, int value);

	/*!
	 * @brief Refresh AccountList.
	 */
	void refreshAccountListFromWorker(const QModelIndex acntTopIdx);

	/*!
	 * @brief Set and run any actions after mainwindow has been created.
	 */
	void setWindowsAfterInit(void);

	/*!
	 * @brief Open dialog with signature detail.
	 */
	void on_signatureDetails_clicked(void);

	/*!
	 * @brief Recevied and store new account database path.
	 */
	void ReceivedNewDataPath(QString newPath);

private:

	QThread *threadSyncAll, *threadSyncOne;
	Worker *workerSyncAll, *workerSyncOne;
	QTimer *timer;
	int timeout;

	/*!
	 * @brief Connects top menu-bar buttons to appropriate actions.
	 */
	void connectTopMenuBarSlots(void);

	/*!
	 * @brief Connect top tool-bar buttons to appropriate actions.
	 */
	void connectTopToolBarSlots(void);

	/*!
	 * @brief Default settings of main window.
	 */
	void defaultUiMainWindowSettings(void) const;

	/*!
	 * @brief Set message action/button to visible.
	 */
	void setMessageActionVisibility(bool action) const;

	/*!
	 * @brief Active/Inactive account menu and buttons in the mainwindow.
	 */
	void activeAccountMenuAndButtons(bool action) const;

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
	 * @brief Update numbers of unread messages in account model.
	 *
	 * param[in] index Index identifying account.
	 * @return True on success.
	 *
	 * @note Does not add/remove any nodes, just updates the counts.
	 */
	bool updateExistingAccountModelUnread(QModelIndex index);

	/*!
	 * @brief Partially regenerates account model according to the database
	 *     content.
	 *
	 * @param[in] index Index identifying account.
	 * @return True on success.
	 *
	 * @note This function adds/removes nodes and does not set the
	 *     currentIndex back to its original position.
	 */
	bool regenerateAccountModelYears(QModelIndex index);

	/*!
	 * @brief Regenerates account model according to the database content.
	 *
	 * @return True on success.
	 *
	 * @note This function add/removes nodes and does not set the
	 *     currentIndex back to its original position.
	 */
	bool regenerateAllAccountModelYears(void);

	/*!
	 * @brief Generate account info HTML message.
	 */
	QString createAccountInfo(const QStandardItem &topItem) const;

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
	 * TODO - If accountItem is 0 then currently selected account is
	 *     processed.
	 */
	QString accountUserName(const QStandardItem *accountItem = 0) const;

	/*!
	 * @brief Get message db to selected account item.
	 *
	 * @note If accountItem is 0 then currently selected account is
	 *     processed.
	 */
	MessageDb * accountMessageDb(const QStandardItem *accountItem);

	/*!
	 * @brief Download sent/received message list for specific account.
	 */
	qdatovka_error downloadMessageList(const QModelIndex &acntTopIdx,
	    const QString messageType);

	/*!
	 * @brief Download attachments, envelope and raw for specific message.
	 */
	qdatovka_error downloadMessage(const QModelIndex &acntTopIdx,
	    const QModelIndex &msgIdx, bool signedMsg, bool incoming);

	/*!
	 * @brief Set message as downloaded from ISDS.
	 */
	bool markMessageAsDownloaded(const QModelIndex &acntTopIdx,
	    const QModelIndex &msgIdx);

	/*!
	 * @brief Download received message delivery info and events
	 */
	bool getReceivedsDeliveryInfo(const QModelIndex &acntTopIdx,
	    const QModelIndex &msgIdx, bool signedMsg);

	/*!
	 * @brief Download sent message delivery info and events
	 */
	bool getSentDeliveryInfo(const QModelIndex &acntTopIdx,
	    int msgIdx, bool signedMsg);

	/*!
	 * @brief Get list of sent message state changes
	 */
	bool getListSentMessageStateChanges(const QModelIndex &acntTopIdx);

	/*!
	 * @brief Get password expiration info for account index
	 */
	bool getPasswordInfo(const QModelIndex &acntTopIdx);

	/*!
	 * @brief Get additional info about author (sender)
	 */
	bool getMessageAuthor(const QModelIndex &acntTopIdx,
	    const QModelIndex &msgIdx);

	/*!
	 * @brief Delete message from long term storage in ISDS.
	 */
	qdatovka_error eraseMessage(const QModelIndex &acntTopIdx, QString dmId);

	/*!
	 * @brief Verify message. Compare hash with hash stored in ISDS.
	 */
	qdatovka_error verifyMessage(const QModelIndex &acntTopIdx,
	    const QModelIndex &msgIdx);

	/*!
	 * @brief Get data about logged in user and his box.
	 */
	bool getOwnerInfoFromLogin(const QModelIndex &acntTopIdx);

	/*!
	 * @brief Get data about logged in user.
	 */
	bool getUserInfoFromLogin(const QModelIndex &acntTopIdx);

	/*!
	 * @brief Authenticate message form db.
	 */
	qdatovka_error authenticateMessageFromDb(const QModelIndex &acntTopIdx,
	    const QModelIndex &msgIdx);

	/*!
	 * @brief Authenticate message form file.
	 */
	qdatovka_error authenticateMessageFromZFO(void);

	/*!
	 * @brief Show message info for user if connection to ISDS fails.
	 */
	void showConnectionErrorMessageBox(int status, QString accountName);

	/*!
	 * @brief Check if connection to ISDS fails.
	 */
	bool checkConnectionError(int status, QString accountName);

	/*!
	 * @brief Show password input boxs
	 */
	QString showPasswordInputBox(const QModelIndex acntTopIdx);

	bool connectToIsds(const QModelIndex acntTopIdx);



	QString m_confDirName; /*!< Configuration directory location. */
	QString m_confFileName; /*!< Configuration file location. */

	AccountModel m_accountModel; /*!<
	                              * Account tree view model. Generated from
	                              * configuration file.
	                              */
	AccountDb m_accountDb; /*!< Account information database. */
	DbMsgsTblModel *m_messageModel; /*!< Currently displayed model. */
	dbContainer m_messageDbs; /*!< Map of message databases. */
	QLineEdit *m_searchLine; /*!< Search-line object. */
	QPushButton *m_pushButton;
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
