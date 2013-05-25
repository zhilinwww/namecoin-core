// Copyright (c) 2009-2011 Satoshi Nakamoto & Bitcoin developers
// Distributed under the MIT/X11 software license, see the accompanying
// file license.txt or http://www.opensource.org/licenses/mit-license.php.
#ifndef BITCOIN_KEYSTORE_H
#define BITCOIN_KEYSTORE_H

#include <boost/signals2/signal.hpp>
#include <boost/signals2/last_value.hpp>
#include "crypter.h"

// Currently CPrivKey is just std::vector<unsigned char>
typedef std::map<std::vector<unsigned char>, CPrivKey> KeyMap;
typedef std::map<std::vector<unsigned char>, std::vector<unsigned char> > CryptedKeyMap;

class CKeyStore
{
private:
    KeyMap mapKeys;
    CryptedKeyMap mapCryptedKeys;

    CKeyingMaterial vMasterKey;

    // if fUseCrypto is true, mapKeys must be empty
    // if fUseCrypto is false, vMasterKey must be empty
    bool fUseCrypto;

public:

    CKeyStore()
        : fUseCrypto(false)
    {
    }
    
    virtual ~CKeyStore()
    {
    }

    void DebugPrint()
    {
        printf("mapKeys.size() = %d\n",         mapKeys.size());
        printf("mapCryptedKeys.size() = %d\n",  mapCryptedKeys.size());
    }

    mutable CCriticalSection cs_mapKeys;
    
    virtual bool AddKey(const CKey& key);
    virtual bool AddCryptedKey(const std::vector<unsigned char> &vchPubKey, const std::vector<unsigned char> &vchCryptedSecret);

    virtual bool HaveKey(const std::vector<unsigned char> &vchPubKey) const
    {
        CRITICAL_BLOCK(cs_mapKeys)
        {
            if (IsCrypted())
                return (mapCryptedKeys.count(vchPubKey) > 0);
            else
                return (mapKeys.count(vchPubKey) > 0);
        }
    }

    virtual bool GetPrivKey(const std::vector<unsigned char> &vchPubKey, CPrivKey& keyOut) const
    {
        CRITICAL_BLOCK(cs_mapKeys)
        {
            if (!IsCrypted())
            {
                std::map<std::vector<unsigned char>, CPrivKey>::const_iterator mi = mapKeys.find(vchPubKey);
                if (mi != mapKeys.end())
                {
                    keyOut = (*mi).second;
                    return true;
                }
            }
            else
            {
                CryptedKeyMap::const_iterator mi = mapCryptedKeys.find(vchPubKey);
                if (mi != mapCryptedKeys.end())
                {
                    const std::vector<unsigned char> &vchCryptedSecret = (*mi).second;
                    if (!DecryptSecret(vMasterKey, vchCryptedSecret, Hash(vchPubKey.begin(), vchPubKey.end()), keyOut))
                        return false;
                    if (keyOut.size() != CSECRET_SIZE)
                        return false;
                    return true;
                }
            }
            return false;
        }
    }

    std::vector<unsigned char> GenerateNewKey();
    
    bool IsCrypted() const
    {
        return fUseCrypto;
    }

    bool IsLocked() const
    {
        if (!IsCrypted())
            return false;
        bool result;
        CRITICAL_BLOCK(cs_mapKeys)
            result = vMasterKey.empty();
        return result;
    }

    bool Lock();

#ifdef GUI
    boost::signals2::signal<void (CKeyStore* wallet)> NotifyStatusChanged;
#endif

protected:
    bool SetCrypted();

    // will encrypt previously unencrypted keys
    bool EncryptKeys(CKeyingMaterial& vMasterKeyIn);

    virtual bool Unlock(const CKeyingMaterial& vMasterKeyIn); 
};

#endif
