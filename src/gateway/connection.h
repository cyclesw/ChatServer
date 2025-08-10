#include <unordered_map>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

namespace im 
{
    using server_t = websocketpp::server<websocketpp::config::asio>;

    /*!
    *  @brief long Connection class to manage client connections and their associated data.
    */
    class Connection 
    {
    public:
        using Ptr = std::shared_ptr<Connection>;
        using server_t = websocketpp::server<websocketpp::config::asio>;

        struct Client 
        {
            std::string uid;
            std::string token;
        };

        Connection() = default;
        ~Connection() = default;

        void Insert(const server_t::connection_ptr& conn, const std::string& uid, const std::string& ssid) ;
        server_t::connection_ptr GetConnection(const std::string& uid);
        bool GetClient(const server_t::connection_ptr& conn, std::string& uid, std::string& ssid);
        void Remove(const server_t::connection_ptr& conn);

    private:
        std::mutex _mutex;
        std::unordered_map<std::string, server_t::connection_ptr> _uid_connections;
        std::unordered_map<server_t::connection_ptr, Client> _conn_clients;

    };
}