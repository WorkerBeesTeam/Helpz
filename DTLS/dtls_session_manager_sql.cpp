#include <QDebug>
#include <QThread>

#ifdef Q_OS_WIN32
#include <QCoreApplication>
#endif

#include <fstream>

#include <botan-2/botan/version.h>
#include <botan-2/botan/loadstor.h>
#include <botan-2/botan/hash.h>
#include <botan-2/botan/pkcs8.h>
#include <botan-2/botan/hex.h>

#include <botan-2/botan/x509self.h>
#include <botan-2/botan/data_src.h>

#if defined(BOTAN_HAS_HMAC_DRBG)
#include <botan-2/botan/hmac_drbg.h>
#endif

#if defined(BOTAN_HAS_SYSTEM_RNG)
#include <botan-2/botan/system_rng.h>
#endif

#if defined(BOTAN_HAS_AUTO_SEEDING_RNG)
  #include <botan-2/botan/auto_rng.h>
#endif

#include <botan-2/botan/pbkdf.h>

#include <Helpz/db_base.h>

#include "dtls_session_manager_sql.h"

namespace Helpz {
namespace DTLS {

uint64_t Mytimestamp()
{
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

// --------------------------------------------------------------------------------

static std::unique_ptr<Database::Table> sessionsTable; // Not good way

Session_Manager_SQL::Session_Manager_SQL(const std::string& passphrase,
                                         Botan::RandomNumberGenerator& rng,
                                         const Database::Connection_Info& info,
                                         size_t max_sessions,
                                         std::chrono::seconds session_lifetime) :
    db_(new Database::Base(info, "DTLSSessions_SQL" + QString::number((quintptr)this))),
    m_rng(rng), m_max_sessions(max_sessions), m_session_lifetime(session_lifetime)
{
    if (!sessionsTable)
        sessionsTable.reset( new Database::Table{"tls_sessions", "ts", {"session_id", "session_start", "hostname", "hostport", "session"}} );
    db_->create_table(*sessionsTable, {"VARCHAR(128) PRIMARY KEY", "INTEGER", "TEXT", "INTEGER", "BLOB"});

    Database::Table tableMetadata = {"tls_sessions_metadata", "tsm", {"passphrase_salt", "passphrase_iterations", "passphrase_check"}};
    db_->create_table(tableMetadata, {"BLOB", "INTEGER", "INTEGER"});

    const size_t salts = db_->row_count(tableMetadata.name());
    std::unique_ptr<Botan::PBKDF> pbkdf(Botan::get_pbkdf("PBKDF2(SHA-512)"));

    if(salts == 1)
    {
        // existing db
        QSqlQuery stmt = db_->select(tableMetadata);
        if(stmt.next())
        {
            QByteArray salt = stmt.value(0).toByteArray();
            const size_t iterations = stmt.value(1).toUInt();
            const size_t check_val_db = stmt.value(2).toUInt();

            Botan::secure_vector<uint8_t> x = pbkdf->pbkdf_iterations(32 + 2,
                                                               passphrase,
                                                               (const uint8_t*)salt.constData(), salt.size(),
                                                               iterations);

            const size_t check_val_created = Botan::make_uint16(x[0], x[1]);
            m_session_key.assign(x.begin() + 2, x.end());

            if(check_val_created != check_val_db)
                throw std::runtime_error("Session database password not valid");
        }
    }
    else
    {
        // maybe just zap the salts + sessions tables in this case?
        if(salts != 0)
            throw std::runtime_error("Seemingly corrupted database, multiple salts found");

        // new database case

        std::vector<uint8_t> salt = Botan::unlock(rng.random_vec(16));
        size_t iterations = 0;

        Botan::secure_vector<uint8_t> x = pbkdf->pbkdf_timed(32 + 2,
                                                      passphrase,
                                                      salt.data(), salt.size(),
                                                      std::chrono::milliseconds(100),
                                                      iterations);

        size_t check_val = Botan::make_uint16(x[0], x[1]);
        m_session_key.assign(x.begin() + 2, x.end());

        db_->insert(tableMetadata, {QByteArray((const char*)salt.data(), salt.size()), (quint32)iterations, (quint32)check_val});
    }

    qRegisterMetaType<Botan::TLS::Session*>("Botan::TLS::Session*");
    connect(this, &Session_Manager_SQL::load_session,
            this, &Session_Manager_SQL::load_session, Qt::BlockingQueuedConnection);
    connect(this, &Session_Manager_SQL::remove_entry_signal,
            this, &Session_Manager_SQL::remove_entry_slot, Qt::QueuedConnection);
    connect(this, &Session_Manager_SQL::remove_all_signal,
            this, &Session_Manager_SQL::remove_all_slot, Qt::BlockingQueuedConnection);
    connect(this, &Session_Manager_SQL::save_signal,
            this, &Session_Manager_SQL::save_slot, Qt::BlockingQueuedConnection);
}

bool Session_Manager_SQL::load_from_session_id(const std::vector<uint8_t>& session_id, Botan::TLS::Session& session) {
    QString session_where = "where session_id = '" + QString::fromStdString(Botan::hex_encode(session_id)) + '\'';
    return check_db_thread_diff() ?
                load_session(session_where, &session) :
                load_session_slot(session_where, &session);
}

bool Session_Manager_SQL::load_from_server_info(const Botan::TLS::Server_Information& server, Botan::TLS::Session& session)
{
    QString where = "where hostname = '%1' and hostport = %2 order by session_start desc";
    where = where.arg(QString::fromStdString(server.hostname())).arg(server.port());
    return check_db_thread_diff() ? load_session(where, &session) :
                                    load_session_slot(where, &session);
}

void Session_Manager_SQL::remove_entry(const std::vector<uint8_t>& session_id) {
    if (check_db_thread_diff())
        remove_entry_signal(QString::fromStdString(Botan::hex_encode(session_id)));
    else
        remove_entry_slot(QString::fromStdString(Botan::hex_encode(session_id)));
}

size_t Session_Manager_SQL::remove_all() {
    if (check_db_thread_diff())
        return remove_all_signal();
    else
        return remove_all_slot();
}

void Session_Manager_SQL::save(const Botan::TLS::Session& session) {
    auto session_vec = session.encrypt(m_session_key, m_rng);

    const int timeval = std::chrono::duration_cast<std::chrono::seconds>(session.start_time().time_since_epoch()).count();

    QVariantList values{QString::fromStdString(Botan::hex_encode(session.session_id())),
                       timeval,
                       QString::fromStdString(session.server_info().hostname()),
                       session.server_info().port(),
                       QByteArray((const char*)session_vec.data(), session_vec.size())};
    if (check_db_thread_diff())
        save_signal(values);
    else
        save_slot(values);
}

std::chrono::seconds Session_Manager_SQL::session_lifetime() const { return m_session_lifetime; }

bool Session_Manager_SQL::load_session_slot(const QString &sql, Botan::TLS::Session *session)
{
    QSqlQuery stmt = db_->select(*sessionsTable, sql, {}, {4});
    QByteArray blob;
    while(stmt.next()) {
        blob = stmt.value(0).toByteArray();
        try {
            *session = Botan::TLS::Session::decrypt((const uint8_t*)blob.constData(), blob.size(), m_session_key);
            return true;
        } catch(...) {}
    }
    return false;
}

void Session_Manager_SQL::remove_entry_slot(const QString &session_id) {
    db_->del(sessionsTable->name(), "session_id = '" + session_id + '\'');
}

int Session_Manager_SQL::remove_all_slot() {
    return db_->del(sessionsTable->name()).numRowsAffected();
}

void Session_Manager_SQL::save_slot(const QVariantList &values) {
    db_->replace(*sessionsTable, values);
    prune_session_cache();
}

void Session_Manager_SQL::prune_session_cache() {
    // First expire old sessions
    const int timeval = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() - m_session_lifetime).time_since_epoch()).count();
    db_->del(sessionsTable->name(), "session_start <= " + QString::number(timeval));

    const size_t sessions = db_->row_count("tls_sessions");

    // Then if needed expire some more sessions at random
    if(m_max_sessions > 0 && sessions > m_max_sessions)
    {
        db_->del(sessionsTable->name(),
                 "session_id in (select session_id from tls_sessions limit " +
                 QString::number(quint32(sessions - m_max_sessions)) + ")");
    }
}

bool Session_Manager_SQL::check_db_thread_diff() {
    return thread() != QThread::currentThread();
}

} // namespace DTLS
} // namespace Helpz
