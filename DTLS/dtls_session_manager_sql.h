#ifndef HELPZ_DTLS_SESSIONMANAGER_SQL_H
#define HELPZ_DTLS_SESSIONMANAGER_SQL_H

#include <iostream>

#include <botan-2/botan/rng.h>
#include <botan-2/botan/credentials_manager.h>
#include <botan-2/botan/tls_session_manager.h>
#include <botan-2/botan/tls_alert.h>
#include <botan-2/botan/tls_policy.h>

#include <QObject>

#include <Helpz/db_connection_info.h>
#include <Helpz/dtls_credentials_manager.h>

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
                          const Database::Connection_Info &info = {":memory:", {}, {}, {}, -1, "QSQLITE"},
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
      bool load_session(const QString& sql, Botan::TLS::Session* session);
      void remove_entry_signal(const QString& session_id);
      int remove_all_signal();
      void save_signal(const QVariantList& values);
private slots:
      bool load_session_slot(const QString& sql, Botan::TLS::Session* session);
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

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_SESSIONMANAGER_SQL_H

