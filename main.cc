
#include <etcd/Client.hpp>

#define HOST "http://8.138.164.251:2379"

int main()
{
    etcd::Client client(HOST);
    auto keep_alive = client.leasekeepalive(3).get();
    
    // auto resp = client.put("/service/user", "test", lease_id);

    return 0;
}