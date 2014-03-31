

#ifndef _MESSAGES_REMOTE_MODEL_
#define _MESSAGES_REMOTE_MODEL_


#include <QStandardItemModel>


/*
 * @brief On-line accessible messages.
 */
class ReceivedMessagesRemoteModel : public QStandardItemModel {

public:
	ReceivedMessagesRemoteModel(QObject *parent = 0);

private:
	bool addMessage(int row, const QString &id, const QString &title,
	    const QString &sender, const QString &delivered,
	    const QString &accepted);
	void fillContent(void);

};


/*
 * @brief On-line accessible messages.
 */
class SentMessagesRemoteModel : public QStandardItemModel {

public:
	SentMessagesRemoteModel(QObject *parent = 0);

private:
	bool addMessage(int row, const QString &id, const QString &title,
	    const QString &recipient, const QString &status,
	    const QString &delivered, const QString &accepted);
	void fillContent(void);

};


#endif /* _MESSAGES_REMOTE_MODEL_ */
