#ifndef GMAILSENDER_H
#define GMAILSENDER_H

#include <QObject>
#include <QDebug>
#include "CppModules/3rdparty/SmtpClient-for-Qt/src/SmtpMime"
#include <mutex>
#include <QThread>

// Singleton
class GMailSender : public QObject
{
    Q_OBJECT

    GMailSender();
    virtual ~GMailSender();
    GMailSender(const GMailSender&) = delete;
    GMailSender& operator=(const GMailSender&) = delete;
    static std::mutex gmailsender_instance_mutex;

protected:
    static GMailSender* m_Instance;
    friend class Cleanup;
    class Cleanup{
        public:
            ~Cleanup();
    };


public:
    // Instance should be init(... before use!
    static GMailSender& instance();
    // this is called from c++, main.cpp
    void init(const QString &user, const QString &pass);
    // this is called from QML side
    Q_INVOKABLE void sendEMail(const QString& sender,
                          const QString& recipient,
                          const QString& subject,
                          const QStringList& attachFilesPaths);

signals:
    // These signals are needed to communicate with the worker object in the other thread
    void initSignal(const QString &user, const QString &pass);
    void sendEMailSignal(const QString& sender,
                              const QString& recipient,
                              const QString& subject,
                              const QStringList& attachFilesPaths);
    // this signal is external, it is received at QML side
    void finishedSendingMailSignal(bool successfull);

private:
    QThread m_workerThread;
};


class SMTPWorker : public QObject
{
    Q_OBJECT

public slots:

    void init(const QString &user, const QString &pass);

    void sendEMail(const QString& sender,
                  const QString& recipient,
                  const QString& subject,
                  const QStringList& attachFilesPaths);

signals:
    void finishedSendingMailSignal(bool successfull);

private:
    QString m_p;
    QString m_u;
    bool m_sendingInProgress;
};

#endif // GMAILSENDER_H
