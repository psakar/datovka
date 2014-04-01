

#ifndef _ACCOUNTS_MODEL_H_
#define _ACCOUNTS_MODEL_H_


#include <QStandardItemModel>


/*!
 * @brief Account hierarchy.
 */
class AccountModel: public QStandardItemModel {

public:
	AccountModel(void);
	bool addAccount(const QString &accountName);
	bool addYearItemToAccount(const QModelIndex &parent, const QString &year);
private:

};


#endif /* _ACCOUNTS_MODEL_H_ */
