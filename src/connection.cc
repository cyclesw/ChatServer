#include "connection.h"
#include "log.hpp"

using namespace im;

void Connection::Insert(const server_t::connection_ptr& conn, const std::string& uid, const std::string& ssid) 
{
    std::lock_guard<std::mutex> lock(_mutex);
    _uid_connections[uid] = conn;
    _conn_clients[conn] = {uid, ssid};
}

server_t::connection_ptr Connection::GetConnection(const std::string& uid) 
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _uid_connections.find(uid);
    if (it != _uid_connections.end()) {
        return it->second;
    }
    LOG_ERROR("...Connection::GetConnection: uid not found: %s", uid.c_str());
    return nullptr;
}

bool Connection::GetClient(const server_t::connection_ptr& conn, std::string& uid, std::string& ssid) 
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _conn_clients.find(conn);
    if (it != _conn_clients.end()) {
        uid = it->second.uid;
        ssid = it->second.token;
        return true;
    }

    LOG_ERROR("...Connection::GetClient: conn not found");
    return false;
}

void Connection::Remove(const server_t::connection_ptr& conn) 
{
    std::lock_guard<std::mutex> lock(_mutex);
    auto it = _conn_clients.find(conn);
    if (it != _conn_clients.end()) {
        _uid_connections.erase(it->second.uid);
        _conn_clients.erase(it);
    }
    
    LOG_WARN("...Connection::Remove: conn not found");
}