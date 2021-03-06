#include "TdlibJsonWrapper.hpp"
#include "include/AppApiInfo.hpp"

#include <QThread>
#include "ListenObject.hpp"
#include "ParseObject.hpp"
#include <td/telegram/td_log.h>

namespace tdlibQt {

TdlibJsonWrapper::TdlibJsonWrapper(QObject *parent) : QObject(parent)
{
    td_set_log_verbosity_level(0);
    client = td_json_client_create();
    //SEG FAULT means that json has error input variable names
    QString tdlibParameters = "{\"@type\":\"setTdlibParameters\",\"parameters\":{"
                              "\"database_directory\":\"depecherDatabase\","
                              "\"api_id\":" + tdlibQt::appid + ","
                              "\"api_hash\":\"" + tdlibQt::apphash + "\","
                              "\"system_language_code\":\""
                              + QLocale::languageToString(QLocale::system().language()) + "\","
                              "\"device_model\":\"" + QSysInfo::prettyProductName() + "\","
                              "\"system_version\":\"" + QSysInfo::productVersion() + "\","
                              "\"application_version\":\"0.1\","
                              "\"use_message_database\":true,"
                              "\"use_secret_chats\":false,"
                              "\"enable_storage_optimizer\":true"
//                              ",\"use_test_dc\":true"
                              "}}";
    td_json_client_send(client, tdlibParameters.toStdString().c_str());
    //answer is - {"@type":"updateAuthorizationState","authorization_state":{"@type":"authorizationStateWaitEncryptionKey","is_encrypted":false}}
}

TdlibJsonWrapper *TdlibJsonWrapper::instance()
{
    static TdlibJsonWrapper    instance;
    return &instance;
}

TdlibJsonWrapper::~TdlibJsonWrapper()
{
    td_json_client_destroy(client);
}
void TdlibJsonWrapper::startListen()
{
    listenThread = new QThread;
    parseThread = new QThread;
    //    listenSchedulerThread = new QThread;
    //    listenScheduler = new ListenScheduler();
    //    connect(listenSchedulerThread, &QThread::started,
    //            listenScheduler, &ListenScheduler::beginForever, Qt::QueuedConnection);
    //    listenScheduler->moveToThread(listenSchedulerThread);
    //    listenSchedulerThread->start();

    listenObject = new ListenObject(client);//, listenScheduler->getCondition());
    parseObject = new ParseObject();
    listenObject->moveToThread(listenThread);
    connect(listenThread, &QThread::started,
            listenObject, &ListenObject::listen, Qt::QueuedConnection);
    connect(listenThread, &QThread::destroyed,
            listenObject, &ListenObject::deleteLater, Qt::QueuedConnection);
    connect(listenObject, &ListenObject::resultReady,
            parseObject, &ParseObject::parseResponse, Qt::QueuedConnection);

    connect(parseThread, &QThread::destroyed,
            parseObject, &ParseObject::deleteLater, Qt::QueuedConnection);
    connect(parseObject, &ParseObject::updateAuthorizationState,
            this, &TdlibJsonWrapper::setAuthorizationState);
    connect(parseObject, &ParseObject::newAuthorizationState,
            this, &TdlibJsonWrapper::newAuthorizationState);

    connect(parseObject, &ParseObject::updateConnectionState,
            this, &TdlibJsonWrapper::setConnectionState);
    connect(parseObject, &ParseObject::getChat,
            this, &TdlibJsonWrapper::getChat);

    connect(parseObject, &ParseObject::newChatReceived,
            this, &TdlibJsonWrapper::newChatGenerated);
    connect(parseObject, &ParseObject::updateFile,
            this, &TdlibJsonWrapper::updateFile);
    connect(parseObject, &ParseObject::newMessages,
            this, &TdlibJsonWrapper::newMessages);
    connect(parseObject, &ParseObject::newMessageFromUpdate,
            this, &TdlibJsonWrapper::newMessageFromUpdate);
    connect(parseObject, &ParseObject::updateChatOrder,
            this, &TdlibJsonWrapper::updateChatOrder);
    connect(parseObject, &ParseObject::updateChatLastMessage,
            this, &TdlibJsonWrapper::updateChatLastMessage);
    connect(parseObject, &ParseObject::updateChatReadInbox,
            this, &TdlibJsonWrapper::updateChatReadInbox);
    connect(parseObject, &ParseObject::updateChatReadOutbox,
            this, &TdlibJsonWrapper::updateChatReadOutbox);
    connect(parseObject, &ParseObject::updateTotalCount,
            this, &TdlibJsonWrapper::updateTotalCount);
    connect(parseObject, &ParseObject::updateChatAction,
            this, &TdlibJsonWrapper::updateUserChatAction);
    connect(parseObject, &ParseObject::updateChatMention,
            this, &TdlibJsonWrapper::updateChatMention);
    connect(parseObject, &ParseObject::updateMentionRead,
            this, &TdlibJsonWrapper::updateMentionRead);
    connect(parseObject, &ParseObject::proxyReceived,
            this, &TdlibJsonWrapper::proxyReceived);
    connect(parseObject, &ParseObject::meReceived,
            this, &TdlibJsonWrapper::meReceived);
    listenThread->start();
    parseThread->start();



}

bool TdlibJsonWrapper::isCredentialsEmpty() const
{
    return m_isCredentialsEmpty;
}

Enums::AuthorizationState TdlibJsonWrapper::authorizationState() const
{
    return m_authorizationState;
}

Enums::ConnectionState TdlibJsonWrapper::connectionState() const
{
    return m_connectionState;
}

void TdlibJsonWrapper::openChat(const QString &chat_id)
{
    std::string openChat = "{\"@type\":\"openChat\","
                           "\"chat_id\":\"" + chat_id.toStdString() + "\"}";
    td_json_client_send(client, openChat.c_str());

}

void TdlibJsonWrapper::closeChat(const QString &chat_id)
{
    std::string closeChat = "{\"@type\":\"closeChat\","
                            "\"chat_id\":\"" + chat_id.toStdString() + "\"}";
    td_json_client_send(client, closeChat.c_str());
}

void TdlibJsonWrapper::getMe()
{
    QString getMe = "{\"@type\":\"getMe\",\"@extra\":\"getMe\"}";
    td_json_client_send(client, getMe.toStdString().c_str());
}


void TdlibJsonWrapper::getProxy()
{
    QString getProxy = "{\"@type\":\"getProxy\"}";
    td_json_client_send(client, getProxy.toStdString().c_str());
}

void TdlibJsonWrapper::setProxy(const QString &type, const QString &address, const int port,
                                const QString &username, const QString &password)
{
    QString proxy = "{\"@type\":\"setProxy\","
                    "\"proxy\":{\"@type\":\"proxyEmpty\"}"
                    "}";
    if (type == "proxySocks5")
        proxy = "{\"@type\":\"setProxy\","
                "\"proxy\":"
                "{\"@type\":\"proxySocks5\","
                "\"server\":\"" + address + "\","
                "\"port\":" + QString::number(port) + ","
                "\"username\":\"" + username + "\","
                "\"password\":\"" + password + "\""
                "}"
                "}";

    td_json_client_send(client, proxy.toStdString().c_str());

}

void TdlibJsonWrapper::setEncryptionKey(const QString &key)
{
    std::string setDatabaseKey =
        "{\"@type\":\"setDatabaseEncryptionKey\","
        "\"new_encryption_key\":\"" + key.toStdString() + "\"}";
    td_json_client_send(client, setDatabaseKey.c_str());
    //Debug answer - Sending result for request 1: ok {}
}

void TdlibJsonWrapper::setPhoneNumber(const QString &phone)
{
    std::string setAuthenticationPhoneNumber =
        "{\"@type\":\"setAuthenticationPhoneNumber\","
        "\"phone_number\":\"" + phone.toStdString() + "\","
        "\"allow_flash_call\":false}";
    td_json_client_send(client, setAuthenticationPhoneNumber.c_str());

}

void TdlibJsonWrapper::setCode(const QString &code)
{
    std::string setAuthenticationCode =
        "{\"@type\":\"checkAuthenticationCode\","
        "\"code\":\"" + code.toStdString() + "\"}";

    td_json_client_send(client, setAuthenticationCode.c_str());

}

void TdlibJsonWrapper::setCodeIfNewUser(const QString &code, const QString &firstName,
                                        const QString &lastName)
{
    std::string AuthCodeIfNewUser =
        "{\"@type\":\"checkAuthenticationCode\","
        "\"code\":\"" + code.toStdString() + "\","
        "\"first_name\":\"" + firstName.toStdString() + "\","
        "\"last_name\":\"" + lastName.toStdString() + "\"}";

    td_json_client_send(client, AuthCodeIfNewUser.c_str());

}
void TdlibJsonWrapper::getChats(const qint64 offset_chat_id, const qint64 offset_order,
                                const int limit)
{
    auto max_order = std::to_string(offset_order);
    std::string getChats =
        "{\"@type\":\"getChats\","
        "\"offset_order\":\"" + max_order + "\","
        "\"offset_chat_id\":\"" + std::to_string(offset_chat_id) + "\","
        "\"limit\":" + std::to_string(limit) + "}";
    td_json_client_send(client, getChats.c_str());
}

void TdlibJsonWrapper::getChat(const qint64 chatId)
{
    std::string getChat = "{\"@type\":\"getChat\","
                          "\"chat_id\":\"" + std::to_string(chatId) + "\""
                          "}";
    td_json_client_send(client, getChat.c_str());

}

void TdlibJsonWrapper::downloadFile(int fileId, int priority, const QString &extra)
{
    if (priority > 32)
        priority = 32;
    if (priority < 1)
        priority = 1;
    QString getFile = "{\"@type\":\"downloadFile\","
                      "\"file_id\":\"" + QString::number(fileId) + "\","
                      "\"priority\":" + QString::number(priority) +
                      "}";
    if (extra != "") {
        getFile.remove(getFile.size() - 1, 1);
        getFile.append(",\"@extra\":\"" + extra + "\"}" );
    }
    td_json_client_send(client, getFile.toStdString().c_str());
}

void TdlibJsonWrapper::getChatHistory(qint64 chat_id, qint64 from_message_id,
                                      int offset,
                                      int limit, bool only_local, const QString &extra)
{
    QString getChatHistory = "{\"@type\":\"getChatHistory\","
                             "\"chat_id\":\"" + QString::number(chat_id) + "\","
                             "\"from_message_id\":\"" + QString::number(from_message_id) + "\","
                             "\"offset\":" + QString::number(offset) + ","
                             "\"limit\":" + QString::number(limit) + ",";
    if (only_local)
        getChatHistory.append("\"only_local\": true");
    else
        getChatHistory.append("\"only_local\": false");

    if (extra != "")
        getChatHistory.append(",\"@extra\": \"" + extra + "\"");
    getChatHistory.append("}");
    td_json_client_send(client, getChatHistory.toStdString().c_str());
}

void TdlibJsonWrapper::logOut()
{
    std::string logOut = "{\"@type\":\"logOut\"}";
    td_json_client_send(client, logOut.c_str());
}

void TdlibJsonWrapper::sendTextMessage(const QString &json)
{
    QString jsonStr = json;
    //Bug in TDLib
    while (jsonStr.at(jsonStr.length() - 1) == '\n')
        jsonStr = jsonStr.remove(jsonStr.length() - 1, 1);
    td_json_client_send(client, jsonStr.toStdString().c_str());
}

void TdlibJsonWrapper::viewMessages(const QString &chat_id, const QVariantList &messageIds,
                                    bool force_read)
{
    QString ids = "";
    for (auto id : messageIds)
        ids.append(QString::number((qint64)id.toDouble()) + ",");

    ids = ids.remove(ids.length() - 1, 1);

    QString force_readStr = force_read ? "true" : "false";
    QString viewMessageStr = "{\"@type\":\"viewMessages\","
                             "\"chat_id\":\"" + chat_id + "\","
                             "\"forceRead\":" + force_readStr + ","
                             "\"message_ids\":[" + ids + "]}";
    td_json_client_send(client, viewMessageStr.toStdString().c_str());


}

void TdlibJsonWrapper::setIsCredentialsEmpty(bool isCredentialsEmpty)
{
    if (m_isCredentialsEmpty == isCredentialsEmpty)
        return;

    m_isCredentialsEmpty = isCredentialsEmpty;
    emit isCredentialsEmptyChanged(isCredentialsEmpty);
}

void TdlibJsonWrapper::setAuthorizationState(Enums::AuthorizationState &authorizationState)
{

    if (m_authorizationState == authorizationState)
        return;

    m_authorizationState = authorizationState;
    emit authorizationStateChanged(authorizationState);
}

void TdlibJsonWrapper::setConnectionState(Enums::ConnectionState &connState)
{
    auto connectionState = connState;

    if (m_connectionState == connectionState)
        return;

    m_connectionState = connectionState;
    emit connectionStateChanged(connectionState);
}

}// namespace tdlibQt
