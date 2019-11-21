#ifndef PROTOCOL_IMPL_H
#define PROTOCOL_IMPL_H

#include <iostream>
#include <fstream>

#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>

#include <botan/tls_server.h>
#include <botan/tls_callbacks.h>

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
#include <botan/credentials_manager.h>

#include <botan/tls_exceptn.h>
#include <botan/x509path.h>
#include <botan/hex.h>

#include <QTimeZone>
#include <QElapsedTimer>


#include <Helpz/net_protocol.h>

struct server_config {
    server_config(quint16 port = 25588, const QString &crt = {}, const QString &key = {},
                  const QString &tls_policy = {}, int eventTimeout = 5 * 60 * 1000) :
        port(port), crt(crt), key(key), tls_policy(tls_policy), eventTimeout(eventTimeout) {}
    uint16_t port;
    QString crt;
    QString key;
    QString tls_policy;
    int eventTimeout;
};

extern server_config *serv_conf;

namespace project {

struct info {
    info(info* obj) : id_(obj->id_), teams_(obj->teams_) {}
    info(uint32_t id, uint32_t team_id_) :
        id_(id)
    {
        if (team_id_)
            teams_.push_back(team_id_);
    }
    info() = default;
    info(info&&) = default;
    info(const info&) = default;

    uint32_t id() const { return id_; }
    void set_id(uint32_t id) { id_ = id; }

    const std::vector<uint32_t>& teams() const { return teams_; }
    void set_teams(const std::vector<uint32_t>& teams) { teams_ = teams; }

    uint32_t id_;
    std::vector<uint32_t> teams_;
};

class base : public info
{
public:
    using udp = boost::asio::ip::udp;

    base(const udp::endpoint &endpoint) :
        remote_endpoint_(endpoint) {}
    base() = default;
    base(const base&) = default;
    base(base&&) = default;

    const std::string& name() const { return name_; }
    void set_name(const std::string& name) { name_ = name; }

    const std::string& title() const
    {
        if (title_.empty())
            const_cast<std::string&>(title_) = address();
        return title_;
    }
    void set_title(const std::string& title) { title_ = title; }

    const udp::endpoint& remote_endpoint() const { return remote_endpoint_; }
    void set_remote_endpoint(const udp::endpoint& endpoint) { remote_endpoint_ = endpoint; }

    std::string address() const
    {
        std::stringstream title_s; title_s << remote_endpoint_;
        return title_s.str();
    }

    struct TimeInfo {
        QTimeZone zone;
        qint64 offset = 0;
    };

    const TimeInfo& time() const { return time_; }
    void set_time_param(const QTimeZone& zone, qint64 offset)
    {
        if (time_.zone != zone)
            time_.zone = zone;
        if (time_.offset != offset)
            time_.offset = offset;
    }

protected:
    std::string name_, title_;

    udp::endpoint remote_endpoint_;

    TimeInfo time_;

//    friend class database::global;
//    friend class database::base;
};

} // namespace project

namespace dtls {

class credentials_manager : public Botan::Credentials_Manager
{
public:
    credentials_manager() { load_certstores(); }
    credentials_manager(Botan::RandomNumberGenerator& rng,
                              const std::string& server_crt,
                              const std::string& server_key)
    {
        Certificate_Info cert;
        cert.key.reset(Botan::PKCS8::load_key(server_key, rng));

        Botan::DataSource_Stream in(server_crt);
        while(!in.end_of_data())
        {
            try {
                cert.certs.push_back(Botan::X509_Certificate(in));
            }
            catch(std::exception& e) { (void)e; }
        }

        // TODO: attempt to validate chain ourselves
        m_creds.push_back(cert);
    }

    void load_certstores()
    {
        try
        {
            // TODO: make path configurable
    #ifdef __linux__
            const std::vector<std::string> paths = { "/usr/share/ca-certificates" };
            for(const std::string& path : paths)
            {
                std::shared_ptr<Botan::Certificate_Store> cs(new Botan::Certificate_Store_In_Memory(path));
                m_certstores.push_back(cs);
            }
    #elif _WIN32
    #pragma GCC warning "Not impliement"
            // TODO: impliement
            /*
            const std::vector<std::string> paths = { (qApp->applicationDirPath() + "/ca-certificates").toStdString() };
            for(const std::string& path : paths)
            {
                std::shared_ptr<Botan::Certificate_Store> cs(new Botan::Certificate_Store_In_Memory(path));
                m_certstores.push_back(cs);
            }*/
    #else
            throw std::runtime_error("Not supported os");
    #endif
        }
        catch(std::exception& e)
        {
            std::cout << "Fail load certstores" << e.what() << std::endl;
        }
    }

    std::vector<Botan::Certificate_Store*>
        trusted_certificate_authorities(const std::string& type,
                                        const std::string& hostname) override
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

    std::vector<Botan::X509_Certificate> cert_chain(
            const std::vector<std::string>& algos,
            const std::string& type,
            const std::string& hostname) override
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

    Botan::Private_Key* private_key_for(const Botan::X509_Certificate& cert,
                                        const std::string& /*type*/,
                                        const std::string& /*context*/) override
    {
        for(auto&& i : m_creds)
        {
            if(cert == i.certs[0])
                return i.key.get();
        }

        return nullptr;
    }

private:
    struct Certificate_Info
    {
        std::vector<Botan::X509_Certificate> certs;
        std::shared_ptr<Botan::Private_Key> key;
    };

    std::vector<Certificate_Info> m_creds;
    std::vector<std::shared_ptr<Botan::Certificate_Store>> m_certstores;
};

class tools
{
public:
    tools()
    {
        try {
            const std::string drbg_seed = "";

    #if defined(BOTAN_HAS_HMAC_DRBG) && defined(BOTAN_HAS_SHA2_64)
            std::vector<uint8_t> seed = Botan::hex_decode(drbg_seed);
            if(seed.empty())
            {
                auto now = std::chrono::high_resolution_clock::now().time_since_epoch();
                const uint64_t ts = std::chrono::duration_cast<std::chrono::nanoseconds>(now).count();

                seed.resize(8);
                Botan::store_be(ts, seed.data());
            }

            std::cout << "rng:HMAC_DRBG with seed " << Botan::hex_encode(seed) << std::endl;

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
            std::cout << " rng:system" << std::endl;
            rng.reset(new Botan::System_RNG);
    #elif defined(BOTAN_HAS_AUTO_SEEDING_RNG)
            std::cout << " rng:autoseeded" << std::endl;
            rng.reset(new Botan::Serialized_RNG(new Botan::AutoSeeded_RNG));
    #endif

    #endif

            if(rng.get() == nullptr)
                throw std::runtime_error("No usable RNG enabled in build, aborting tests");

            session_manager.reset(new Botan::TLS::Session_Manager_In_Memory(*rng));

            creds.reset(new credentials_manager(*rng, serv_conf->crt.toStdString(), serv_conf->key.toStdString()));

            // init policy ------------------------------------------>
            try {
                std::string tls_policy_file_name = serv_conf->tls_policy.toStdString();
                std::ifstream policy_is(tls_policy_file_name);
                if (policy_is.fail())
                    std::cout << "Fail to open TLS policy file:" << tls_policy_file_name << std::endl;
                else
                    policy.reset(new Botan::TLS::Text_Policy(policy_is));
            }
            catch(const std::exception& e) {
                std::cerr << "Fail to read TLS policy:" << e.what() << std::endl;
            }

            if (!policy)
                policy.reset(new Botan::TLS::Text_Policy(std::string()));
        }
        catch(std::exception& e) { std::cerr << "[Helper]" << e.what() << std::endl; }
        catch(...) { std::cerr << "Fail to init Helper" << std::endl; }
    }

    std::unique_ptr<Botan::RandomNumberGenerator> rng;
    std::unique_ptr<credentials_manager> creds;
    std::unique_ptr<Botan::TLS::Session_Manager_In_Memory> session_manager;
    std::unique_ptr<Botan::TLS::Text_Policy> policy; // TODO: read policy from file
};

extern tools* dtls_tools;

struct data_pack
{
    data_pack(std::size_t size, std::unique_ptr<uint8_t[]>&& buffer, boost::asio::ip::udp::endpoint remote_endpoint) :
        size(size), buffer(std::move(buffer)), remote_endpoint(remote_endpoint) {}

    data_pack() = default;
    data_pack(data_pack&&) = default;
    data_pack& operator =(data_pack&&) = default;
    data_pack(data_pack& o) : data_pack{o.size, std::move(o.buffer), o.remote_endpoint} {}
    data_pack& operator =(data_pack& o) {
        size = o.size;
        buffer = std::move(o.buffer);
        remote_endpoint = o.remote_endpoint;
        return *this;
    }
    data_pack(const data_pack&) = delete;
    data_pack& operator =(const data_pack&) = delete;

    std::size_t size;
    std::unique_ptr<uint8_t[]> buffer;
    boost::asio::ip::udp::endpoint remote_endpoint;
};


class server_thread;
class server;
extern server* dtls_server;

class protocol_base : public project::base, public Helpz::Network::Protocol, public Botan::TLS::Callbacks
{
public:

//    using udp = boost::asio::ip::udp;
    std::mutex mutex_;
//    boost::signals2::signal<void (const udp::endpoint&, const uint8_t*, std::size_t size)> send;
//    boost::signals2::signal<void (const project::base&)> closed_signal;

    protocol_base(const udp::endpoint &remote_endpoint);

    int version() const;

    dtls::server_thread* thread();
    void set_thread(dtls::server_thread* th);

    void init();

    virtual void ready_write();
    virtual void closed();

    void process_data(uint8_t *data, std::size_t size);

private:
    void write(const quint8 *data, std::size_t size) override;
    void process_message(quint16 cmd, QByteArray &&data) override;
    void process_answer_message(quint16 cmd, QByteArray &&data) override;
private:
    void tls_emit_data(const uint8_t data[], size_t size) override;
    void tls_record_received(Botan::u64bit, const uint8_t data[], size_t size) override;
    void tls_alert(Botan::TLS::Alert alert) override;

    void tls_verify_cert_chain(
       const std::vector<Botan::X509_Certificate>& cert_chain,
       const std::vector<std::shared_ptr<const Botan::OCSP::Response>>& ocsp,
       const std::vector<Botan::Certificate_Store*>& trusted_roots,
       Botan::Usage_Type usage,
       const std::string& hostname,
       const Botan::TLS::Policy& policy) override;
    bool tls_session_established(const Botan::TLS::Session &session) override;
    std::string tls_server_choose_app_protocol(const std::vector<std::string> &client_protos) override;

    std::unique_ptr< Botan::TLS::Server > dtls_;

    dtls::server_thread* thread_;

    int version_ = 0;
};

class protocol_impl : public protocol_base
{
public:
    using protocol_base::protocol_base;
    virtual ~protocol_impl();
//    std::shared_ptr<protocol_impl> i_am;

public: // for websocket

    virtual void send_value(quint32 item_id, const QVariant &raw_data);
    virtual void send_mode(quint32 mode_id, quint32 group_id);
    virtual void send_param_values(const QByteArray &msg_buff);
    virtual void send_code(quint32 code_id, const QString &text);
    virtual void send_script(const QString &script);
    virtual void send_restart();
    // protocol interface
public:
    void closed() override;
    void process_message_stream(uint16_t cmd, QDataStream &msg);
private:

    bool authenticated_ = false;
};

} // namespace dtls

#endif // PROTOCOL_IMPL_H
