#include <iostream>
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

#include "dtls_tools.h"

namespace Helpz {
namespace DTLS {

Tools::Tools(const std::string &tls_policy_file_name, const std::string &crt_file_name, const std::string &key_file_name)
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
        rng_.reset(new Botan::Serialized_RNG(drbg.release()));
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

        if(!rng_)
            throw std::runtime_error("No usable RNG enabled in build, aborting tests");

        session_manager_.reset(new Botan::TLS::Session_Manager_In_Memory(*rng_));

        creds_.reset(crt_file_name.empty() ? new Credentials_Manager() :
                                            new Credentials_Manager(*rng_, crt_file_name, key_file_name));

        // init policy ------------------------------------------>
        try {
            std::ifstream policy_is(tls_policy_file_name);
            if (policy_is.fail())
                std::cout << "Fail to open TLS policy file: " << tls_policy_file_name << std::endl;
            else
                policy_.reset(new Botan::TLS::Text_Policy(policy_is));
        }
        catch(const std::exception& e) {
            std::cerr << "Fail to read TLS policy: " << e.what() << std::endl;
        }

        if (!policy_)
            policy_.reset(new Botan::TLS::Text_Policy(std::string()));
    }
    catch(std::exception& e) { std::cerr << "[Helper]" << e.what() << std::endl; }
    catch(...) { std::cerr << "Fail to init Helper" << std::endl; }
}

} // namespace DTLS
} // namespace Helpz
