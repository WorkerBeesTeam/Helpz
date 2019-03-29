#include <iostream>

#include <botan/pkcs8.h>
#include <botan/data_src.h>

#include "dtls_credentials_manager.h"

namespace Helpz {
namespace DTLS {

Credentials_Manager::Credentials_Manager() { load_certstores(); }
Credentials_Manager::Credentials_Manager(Botan::RandomNumberGenerator &rng, const std::string &server_crt, const std::string &server_key)
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

void Credentials_Manager::load_certstores()
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

std::vector<Botan::Certificate_Store *> Credentials_Manager::trusted_certificate_authorities(const std::string &type,
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

std::vector<Botan::X509_Certificate> Credentials_Manager::cert_chain(const std::vector<std::string> &algos, const std::string &type, const std::string &hostname)
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

Botan::Private_Key *Credentials_Manager::private_key_for(const Botan::X509_Certificate &cert, const std::string &, const std::string &)
{
    for(auto&& i : m_creds)
    {
        if(cert == i.certs[0])
            return i.key.get();
    }

    return nullptr;
}

} // namespace DTLS
} // namespace Helpz
