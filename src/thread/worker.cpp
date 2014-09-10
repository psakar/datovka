#include <QTimer>
#include <QEventLoop>
#include <QThread>
#include <QDebug>

#include "worker.h"
#include "src/common.h"
#include "src/io/db_tables.h"
#include "src/io/dbs.h"
#include "src/io/isds_sessions.h"
#include "src/io/pkcs7.h"




Worker::Worker(QObject *parent) : QObject(parent) {
	_working =false;
	_abort = false;
}

void Worker::requestWorkInThread() {

	mutex.lock();
	_working = true;
	_abort = false;
	qDebug() << "Request worker start in Thread " <<
	    thread()->currentThreadId();
	mutex.unlock();

	emit workRequestedInThread();
}

void Worker::abortWork() {

	mutex.lock();
	if (_working) {
		_abort = true;
		qDebug() << "Request worker aborting in Thread " <<
		    thread()->currentThreadId();
	}

	mutex.unlock();
}

void Worker::doWork()
{
	qDebug() << "Starting worker process in Thread "
	   << thread()->currentThreadId();

	for (int i = 0; i < 20; i ++) {

		// Checks if the process should be aborted
		mutex.lock();
		bool abort = _abort;
		mutex.unlock();

		if (abort) {
			qDebug()<<"Aborting worker process in Thread "<<thread()->currentThreadId();
			break;
		}

		// This will stupidly wait 1 sec doing nothing...
		QEventLoop loop;
		QTimer::singleShot(500, &loop, SLOT(quit()));
		loop.exec();

		qDebug() << "--" << i << "-"
			<< thread()->currentThreadId() ;

		// Once we're done waiting, value is updated
		//emit valueChanged(QString::number(i));
	}

	// Set _working to false, meaning the process can't be aborted anymore.
	mutex.lock();
	_working = false;
	mutex.unlock();

	qDebug() << "Worker process finished in Thread " <<
	    thread()->currentThreadId();

	//Once 60 sec passed, the finished signal is sent
	emit finishedWork();
}
