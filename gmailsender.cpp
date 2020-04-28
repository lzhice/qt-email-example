#include "gmailsender.h"


GMailSender* GMailSender::m_Instance = nullptr;
std::mutex GMailSender::gmailsender_instance_mutex;

GMailSender& GMailSender::instance() {

    static Cleanup cleanup;
    std::lock_guard<std::mutex> guard(gmailsender_instance_mutex);
    if (m_Instance == nullptr) {
        m_Instance = new GMailSender();
    }
    return  *m_Instance;
}

GMailSender::Cleanup::~Cleanup() {
    std::lock_guard<std::mutex> guard(GMailSender::gmailsender_instance_mutex);
    delete GMailSender::m_Instance;
    GMailSender::m_Instance = nullptr;

}

GMailSender::GMailSender()
{
    SMTPWorker *smtpWorker = new SMTPWorker;
    smtpWorker->moveToThread(&m_workerThread);
    connect(&m_workerThread, &QThread::finished, smtpWorker, &QObject::deleteLater);
    // connect some communication
    connect(this, &GMailSender::initSignal, smtpWorker, &SMTPWorker::init);
    connect(this, &GMailSender::sendEMailSignal, smtpWorker, &SMTPWorker::sendEMail);
    connect(smtpWorker, &SMTPWorker::finishedSendingMailSignal, this, &GMailSender::finishedSendingMailSignal);
    m_workerThread.start();
}

GMailSender::~GMailSender()
{
    m_workerThread.quit();
    m_workerThread.wait();
}

void GMailSender::init(const QString &user, const QString &pass)
{
    // received by the worker
    emit initSignal(user, pass);
}

void GMailSender::sendEMail(const QString &sender, const QString &recipient, const QString &subject, const QStringList &attachFilesPaths)
{
    emit sendEMailSignal(sender, recipient, subject, attachFilesPaths);
}


void SMTPWorker::sendEMail(const QString &sender,
                            const QString &recipient,
                            const QString &subject,
                            const QStringList& attachFilesPaths)
{
    if (m_sendingInProgress) {
        return;
    }
    m_sendingInProgress = true;

    bool connected = false;
    bool loggedIn = false;
    bool mailSent = false;

    //qDebug() << "Sending email";

    //m_smtp.setHost("smtp.gmail.com");
    //m_smtp.setPort(465);
    //m_smtp.setConnectionType(SmtpClient::SslConnection);

    //m_smtp.setUser("mailaddress@gmail.com");
    //m_smtp.setPassword("12345");

    SmtpClient smtp("smtp.gmail.com", 465, SmtpClient::SslConnection);
    smtp.setUser(m_u);
    smtp.setPassword(m_p);

    // connect
    connected = smtp.connectToHost();

    //login
    if (connected) {
        loggedIn = smtp.login();
    } else {
        qDebug() << "Error: SMTP connectToHost";
    }

    // send mail
    if (loggedIn) {
        // create the message
        MimeMessage message;

        message.setSender(new EmailAddress(sender));
        message.addRecipient(new EmailAddress(recipient));
        message.setSubject(subject);

        //MimeText text;
        //text.setText("Hi!\n This is an email with some attachments.");
        //message.addPart(&text);

        // Now we create the attachment object

        QList<MimeAttachment*> l_attachmentsList;
        for (const auto& file : attachFilesPaths) {
            MimeAttachment* attachment = new MimeAttachment(new QFile(file));
            if (file.right(3) == "pdf") {
                attachment->setContentType("application/pdf");
            } else {
                // default
                attachment->setContentType("application/octet-stream");
            }
            l_attachmentsList.append(attachment);
        }
        for (const auto& att : l_attachmentsList) {
            message.addPart(att);
        }

        // Add an another attachment
        //message.addPart(new MimeAttachment(new QFile("document.pdf")));

        // Now we can send the mail
        mailSent = smtp.sendMail(message);

        // free resources
        qDeleteAll(l_attachmentsList);

    } else {
        qDebug() << "Error: SMTP login";
    }

    if (!mailSent) {
        qDebug() << "Error: sendMail";
    }

    if (connected) {
        smtp.quit();
    }

    emit finishedSendingMailSignal(mailSent);

    m_sendingInProgress = false;

    //std::this_thread::sleep_for(std::chrono::milliseconds(500));

    //qDebug() << "SMTP mime sent";
}


void SMTPWorker::init(const QString &user, const QString &pass)
{
    m_u = user;
    m_p = pass;
    m_sendingInProgress = false;
}


//void GMailSender::sendEMail(const QString &sender, const QString &recipient,
//                            const QString &subject, const QStringList& attachFilesPaths)
//{
//    QFuture<void> future = QtConcurrent::run(this, &GMailSender::m_sendEMail, sender, recipient, subject, attachFilesPaths);
//}
