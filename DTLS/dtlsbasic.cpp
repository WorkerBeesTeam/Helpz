#include <QDebug>

#ifdef Q_OS_WIN32
#include <QCoreApplication>
#endif

#include <fstream>

#include <botan/version.h>
#include <botan/loadstor.h>
#include <botan/hash.h>
#include <botan/pkcs8.h>
#include <botan/hex.h>

#include <botan/x509self.h>
#include <botan/data_src.h>

#if defined(BOTAN_HAS_HMAC_DRBG)
#include <botan/hmac_drbg.h>
#endif

#if defined(BOTAN_HAS_SYSTEM_RNG)
#include <botan/system_rng.h>
#endif

#if defined(BOTAN_HAS_AUTO_SEEDING_RNG)
  #include <botan/auto_rng.h>
#endif

#include <botan/pbkdf.h>

#include <Helpz/db_base.h>
#include "dtlsbasic.h"
#include "dtlsproto.h"

QDebug operator<< (QDebug dbg, const std::string &str) { return dbg << str.c_str(); }

namespace Helpz {
namespace DTLS {

uint64_t Mytimestamp()
{
    auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
    return std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();
}

// --------------------------------------------------------------------------------

static std::unique_ptr<Database::Table> sessionsTable; // Not good way

Session_Manager_SQL::Session_Manager_SQL(Database::Base* db,
                                         const std::string& passphrase,
                                         Botan::RandomNumberGenerator& rng,
                                         size_t max_sessions,
                                         std::chrono::seconds session_lifetime) :
    m_db(db ? db->clone(db_name()) : nullptr),
    m_rng(rng),
    m_max_sessions(max_sessions),
    m_session_lifetime(session_lifetime)
{
    if (!m_db)
        m_db = new Helpz::Database::Base({":memory:", QString(), QString(), QString(), -1, "QSQLITE"}, db_name());

    if (!sessionsTable)
        sessionsTable.reset( new Database::Table{"tls_sessions", {"session_id", "session_start", "hostname", "hostport", "session"}} );
    m_db->createTable(*sessionsTable, {"VARCHAR(128) PRIMARY KEY", "INTEGER", "TEXT", "INTEGER", "BLOB"});

    Database::Table tableMetadata = {"tls_sessions_metadata", {"passphrase_salt", "passphrase_iterations", "passphrase_check"}};
    m_db->createTable(tableMetadata, {"BLOB", "INTEGER", "INTEGER"});

    const size_t salts = m_db->row_count(tableMetadata.name);
    std::unique_ptr<Botan::PBKDF> pbkdf(Botan::get_pbkdf("PBKDF2(SHA-512)"));

    if(salts == 1)
    {
        // existing db
        QSqlQuery stmt = m_db->select(tableMetadata);
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
                throw Botan::Exception("Session database password not valid");
        }
    }
    else
    {
        // maybe just zap the salts + sessions tables in this case?
        if(salts != 0)
            throw Botan::Exception("Seemingly corrupted database, multiple salts found");

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

        m_db->insert(tableMetadata, {QByteArray((const char*)salt.data(), salt.size()), (quint32)iterations, (quint32)check_val});
    }
}

Session_Manager_SQL::~Session_Manager_SQL()
{
    delete m_db;
}

bool Session_Manager_SQL::load_from_session_id(const std::vector<uint8_t>& session_id,
                                               Botan::TLS::Session& session)
{
    QSqlQuery stmt = m_db->select(*sessionsTable, "where session_id = ?", {QString::fromStdString(Botan::hex_encode(session_id))}, {4});
    while(stmt.next())
    {
        QByteArray blob = stmt.value(0).toByteArray();

        try
        {
            session = Botan::TLS::Session::decrypt((const uint8_t*)blob.constData(), blob.size(), m_session_key);
            return true;
        }
        catch(...) {}
    }

    return false;
}

bool Session_Manager_SQL::load_from_server_info(const Botan::TLS::Server_Information& server,
                                                Botan::TLS::Session& session)
{
    QSqlQuery stmt = m_db->select(*sessionsTable,
                                    "where hostname = ? and hostport = ? "
                                    "order by session_start desc",
    { QString::fromStdString(server.hostname()), server.port()}, {4});

    while(stmt.next())
    {
        QByteArray blob = stmt.value(0).toByteArray();

        try
        {
            session = Botan::TLS::Session::decrypt((const uint8_t*)blob.constData(), blob.size(), m_session_key);
            return true;
        }
        catch(...)
        {
        }
    }

    return false;
}

void Session_Manager_SQL::remove_entry(const std::vector<uint8_t>& session_id)
{
    m_db->del(sessionsTable->name, "session_id = ?",
    { QString::fromStdString(Botan::hex_encode(session_id))});
}

size_t Session_Manager_SQL::remove_all()
{
    return m_db->del(sessionsTable->name).numRowsAffected();
}

void Session_Manager_SQL::save(const Botan::TLS::Session& session)
{
    auto session_vec = session.encrypt(m_session_key, m_rng);

    const int timeval = std::chrono::duration_cast<std::chrono::seconds>(session.start_time().time_since_epoch()).count();

    QVariantList values{QString::fromStdString(Botan::hex_encode(session.session_id())),
                       timeval,
                       QString::fromStdString(session.server_info().hostname()),
                       session.server_info().port(),
                       QByteArray((const char*)session_vec.data(), session_vec.size())};
    m_db->replace(*sessionsTable, values);

    prune_session_cache();
}

void Session_Manager_SQL::prune_session_cache()
{
    // First expire old sessions
    const int timeval = std::chrono::duration_cast<std::chrono::seconds>((std::chrono::system_clock::now() - m_session_lifetime).time_since_epoch()).count();
    m_db->del(sessionsTable->name, "session_start <= ?", {timeval});

    const size_t sessions = m_db->row_count("tls_sessions");

    // Then if needed expire some more sessions at random
    if(m_max_sessions > 0 && sessions > m_max_sessions)
    {
        m_db->del(sessionsTable->name, "session_id in (select session_id from tls_sessions limit ?)",
        {quint32(sessions - m_max_sessions)});
    }
}

QString Session_Manager_SQL::db_name() const
{
    return QString("Session_Manager_SQL%1").arg((quintptr)this);
}

// --------------------------------------------------------------------------------

Basic_Credentials_Manager::Basic_Credentials_Manager() { load_certstores(); }

Basic_Credentials_Manager::Basic_Credentials_Manager(Botan::RandomNumberGenerator &rng, const std::string &server_crt, const std::string &server_key)
{
    Certificate_Info cert;
    cert.key.reset(Botan::PKCS8::load_key(server_key, rng));

    Botan::DataSource_Stream in(server_crt);
    while(!in.end_of_data())
    {
        try {
            cert.certs.push_back(Botan::X509_Certificate(in));
        }
        catch(std::exception& e) { Q_UNUSED(e) }
    }

    // TODO: attempt to validate chain ourselves
    m_creds.push_back(cert);
}

void Basic_Credentials_Manager::load_certstores()
{
    try
    {
        // TODO: make path configurable
#ifdef Q_OS_UNIX
        const std::vector<std::string> paths = { "/usr/share/ca-certificates" };
        for(const std::string& path : paths)
        {
            std::shared_ptr<Botan::Certificate_Store> cs(new Botan::Certificate_Store_In_Memory(path));
            m_certstores.push_back(cs);
        }
#elif defined(Q_OS_WIN32)
#pragma GCC warning "Not impliement"
        // TODO: impliement
        /*
        const std::vector<std::string> paths = { (qApp->applicationDirPath() + "/ca-certificates").toStdString() };
        for(const std::string& path : paths)
        {
            std::shared_ptr<Botan::Certificate_Store> cs(new Botan::Certificate_Store_In_Memory(path));
            m_certstores.push_back(cs);
        }*/
#endif
    }
    catch(std::exception& e)
    {
        qCDebug(Log) << "Fail load certstores" << e.what();
    }
}

std::vector<Botan::Certificate_Store *> Basic_Credentials_Manager::trusted_certificate_authorities(const std::string &type,
                                                                                                   const std::string &hostname)
{
    BOTAN_UNUSED(hostname);
    //        std::cerr << "trusted_certificate_authorities" << type << "|" << hostname << std::endl;
    std::vector<Botan::Certificate_Store*> v;

    // don't ask for client certs
    if(type == "tls-server")
        return v;

    for(const std::shared_ptr<Botan::Certificate_Store>& cs : m_certstores)
        v.push_back(cs.get());

    return v;
}

std::vector<Botan::X509_Certificate> Basic_Credentials_Manager::cert_chain(const std::vector<std::string> &algos, const std::string &type, const std::string &hostname)
{
    BOTAN_UNUSED(type);

    for(const Certificate_Info& i : m_creds)
    {
        if(std::find(algos.begin(), algos.end(), i.key->algo_name()) == algos.end())
            continue;

        if(hostname != "" && !i.certs[0].matches_dns_name(hostname))
            continue;

        return i.certs;
    }

    return std::vector<Botan::X509_Certificate>();
}

Botan::Private_Key *Basic_Credentials_Manager::private_key_for(const Botan::X509_Certificate &cert, const std::string &, const std::string &)
{
    for(auto&& i : m_creds)
    {
        if(cert == i.certs[0])
            return i.key.get();
    }

    return nullptr;
}

BotanHelpers::BotanHelpers(Database::Base* db, const QString &tls_policy_file_name,
                           const QString &crt_file_name, const QString &key_file_name)
{
    try {
        const std::string drbg_seed = "";

#if defined(BOTAN_HAS_HMAC_DRBG) && defined(BOTAN_HAS_SHA2_64)
        std::vector<uint8_t> seed = Botan::hex_decode(drbg_seed);
        if(seed.empty())
        {
            const uint64_t ts = Mytimestamp();
            seed.resize(8);
            Botan::store_be(ts, seed.data());
        }

        qCDebug(Log) << "rng:HMAC_DRBG with seed" << Botan::hex_encode(seed);

        // Expand out the seed to 512 bits to make the DRBG happy
        std::unique_ptr<Botan::HashFunction> sha512(Botan::HashFunction::create("SHA-512"));
        sha512->update(seed);
        seed.resize(sha512->output_length());
        sha512->final(seed.data());

        std::unique_ptr<Botan::HMAC_DRBG> drbg(new Botan::HMAC_DRBG("SHA-384"));
        drbg->initialize_with(seed.data(), seed.size());
        rng.reset(new Botan::Serialized_RNG(drbg.release()));
#else
        if(drbg_seed != "")
            throw std::runtime_error("HMAC_DRBG disabled in build, cannot specify DRBG seed");

#if defined(BOTAN_HAS_SYSTEM_RNG)
        qCDebug(NetworkLog) << " rng:system";
        rng.reset(new Botan::System_RNG);
#elif defined(BOTAN_HAS_AUTO_SEEDING_RNG)
        qCDebug(NetworkLog) << " rng:autoseeded";
        rng.reset(new Botan::Serialized_RNG(new Botan::AutoSeeded_RNG));
#endif

#endif

        if(rng.get() == nullptr)
            throw std::runtime_error("No usable RNG enabled in build, aborting tests");

//        session_manager.reset(new Botan::TLS::Session_Manager_In_Memory(*rng));
        session_manager.reset(new Session_Manager_SQL(db, "k7R2Pu90Shh4", *rng, 0));

        creds.reset(crt_file_name.isEmpty() ? new Basic_Credentials_Manager() :
                                  new Basic_Credentials_Manager(*rng, crt_file_name.toStdString(), key_file_name.toStdString()));

        // init policy ------------------------------------------>
        try {
            std::ifstream policy_is(tls_policy_file_name.toStdString());
            if (policy_is.fail())
                qCDebug(Log) << "Fail to open TLS policy file:" << tls_policy_file_name;
            else
                policy.reset(new Botan::TLS::Text_Policy(policy_is));
        }
        catch(const std::exception& e) {
            qCWarning(Log) << "Fail to read TLS policy:" << e.what();
        }

        if (!policy)
            policy.reset(new Botan::TLS::Text_Policy(std::string()));
    }
    catch(std::exception& e) { qCCritical(Log) << "[Helper]" << e.what(); }
    catch(...) { qCCritical(Log) << "Fail to init Helper"; }
}

} // namespace DTLS
} // namespace Helpz
