//
// Created by lang liu on 2024/9/2.
//

#include "Registry.h"

Registry::Registry(const std::string &hostname)
    :_client(std::make_shared<etcd::Client>(hostname))
    ,_keepAlive(_client->leasekeepalive(3).get())
    ,_leaseId(_keepAlive->Lease())
{}

Registry::~Registry()
{
    _keepAlive->Cancel();
}

bool Registry::RegistryServer(const std::string &key, const std::string &value)
{
    auto resp = _client->put(key, value, _leaseId).get();
    if (!resp.is_ok())
    {
        
    }
}



