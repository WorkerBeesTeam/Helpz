#ifndef HELPZ_DTLS_BASICCREDENTIALSMANAGER_H
#define HELPZ_DTLS_BASICCREDENTIALSMANAGER_H

#include <iostream>

#include <botan/rng.h>
#include <botan/credentials_manager.h>
#include <botan/tls_session_manager.h>
#include <botan/tls_alert.h>
#include <botan/tls_policy.h>

#include <QObject>

#include <Helpz/db_connectioninfo.h>

namespace Helpz {

namespace Database {
class Base;
}

namespace DTLS {
uint64_t Mytimestamp();

class Session_Manager_SQL : public QObject, public Botan::TLS::Session_Manager
{
    Q_OBJECT
public:
      /**
      * @param db A connection to the database to use
               The table names botan_tls_sessions and
               botan_tls_sessions_metadata will be used
      * @param passphrase used to encrypt the session data
      * @param rng a random number generator
      * @param max_sessions a hint on the maximum number of sessions
      *        to keep in memory at any one time. (If zero, don't cap)
      * @param session_lifetime sessions are expired after this many
      *        seconds have elapsed from initial handshake.
      */
      Session_Manager_SQL(const std::string& passphrase,
                          Botan::RandomNumberGenerator& rng,
                          const Database::ConnectionInfo &info = {":memory:", {}, {}, {}, -1, "QSQLITE"},
                          size_t max_sessions = 1000,
                          std::chrono::seconds session_lifetime = std::chrono::seconds(7200));

      Session_Manager_SQL(const Session_Manager_SQL&) = delete;
      Session_Manager_SQL& operator=(const Session_Manager_SQL&) = delete;

      bool load_from_session_id(const std::vector<uint8_t>& session_id,
                                Botan::TLS::Session& session) override;

      bool load_from_server_info(const Botan::TLS::Server_Information& info,
                                 Botan::TLS::Session& session) override;

      void remove_entry(const std::vector<uint8_t>& session_id) override;

      size_t remove_all() override;

      void save(const Botan::TLS::Session& session_data) override;

      std::chrono::seconds session_lifetime() const override;

signals:
      bool load_session(const QString& sql, const QVariantList& values, Botan::TLS::Session* session);
      void remove_entry_signal(const QString& session_id);
      int remove_all_signal();
      void save_signal(const QVariantList& values);
private slots:
      bool load_session_slot(const QString& sql, const QVariantList& values, Botan::TLS::Session* session);
      void remove_entry_slot(const QString& session_id);
      int remove_all_slot();
      void save_slot(const QVariantList& values);
private:
      void prune_session_cache();
      bool check_db_thread_diff();

      std::shared_ptr<Database::Base> db_;
      Botan::secure_vector<uint8_t> m_session_key;
      Botan::RandomNumberGenerator& m_rng;
      size_t m_max_sessions;
      std::chrono::seconds m_session_lifetime;
   };

class Basic_Credentials_Manager : public Botan::Credentials_Manager
{
public:
    Basic_Credentials_Manager();
    Basic_Credentials_Manager(Botan::RandomNumberGenerator& rng,
                              const std::string& server_crt,
                              const std::string& server_key);

    void load_certstores();

    std::vector<Botan::Certificate_Store*>
    trusted_certificate_authorities(const std::string& type,
                                    const std::string& hostname) override;

    std::vector<Botan::X509_Certificate> cert_chain(
            const std::vector<std::string>& algos,
            const std::string& type,
            const std::string& hostname) override;

    Botan::Private_Key* private_key_for(const Botan::X509_Certificate& cert,
                                        const std::string& /*type*/,
                                        const std::string& /*context*/) override;

private:
    struct Certificate_Info
    {
        std::vector<Botan::X509_Certificate> certs;
        std::shared_ptr<Botan::Private_Key> key;
    };

    std::vector<Certificate_Info> m_creds;
    std::vector<std::shared_ptr<Botan::Certificate_Store>> m_certstores;
};

struct BotanHelpers {
    BotanHelpers(const Database::ConnectionInfo &db_info,
                    const QString& tls_policy_file_name,
                     const QString& crt_file_name = QString(),
                     const QString& key_file_name = QString());
    std::unique_ptr<Botan::RandomNumberGenerator> rng;
    std::unique_ptr<Basic_Credentials_Manager> creds;
    std::unique_ptr<Session_Manager_SQL> session_manager;
//    std::unique_ptr<Botan::TLS::Session_Manager_In_Memory> session_manager;
    std::unique_ptr<Botan::TLS::Text_Policy> policy; // TODO: read policy from file
};

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_BASICCREDENTIALSMANAGER_H

