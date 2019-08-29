#ifndef HELPZ_DTLS_CREDENTIALS_MANAGER_H
#define HELPZ_DTLS_CREDENTIALS_MANAGER_H

#include <botan-2/botan/credentials_manager.h>

namespace Helpz {
namespace DTLS {

class Credentials_Manager : public Botan::Credentials_Manager
{
public:
    Credentials_Manager();
    Credentials_Manager(Botan::RandomNumberGenerator& rng,
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

} // namespace DTLS
} // namespace Helpz

#endif // HELPZ_DTLS_CREDENTIALS_MANAGER_H
