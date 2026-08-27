#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/dispatch.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/json.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <chrono>
#include <iomanip>
#include <mutex>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
namespace net = boost::asio;
using tcp = boost::asio::ip::tcp;

class Server;

void logMessage(const std::string& level, const std::string& message) {
    auto now = std::chrono::system_clock::now();
    auto time_t = std::chrono::system_clock::to_time_t(now);
    std::tm tm;
#if defined(_WIN32)
    localtime_s(&tm, &time_t);
#else
    localtime_r(&time_t, &tm);
#endif
    std::cout << std::put_time(&tm, "%H:%M:%S") << " [" << level << "] " << message << std::endl;
}

class Session : public std::enable_shared_from_this<Session> {
    websocket::stream<beast::tcp_stream> ws_;
    beast::flat_buffer buffer_;
    Server& server_;
    std::string room_id_;
    std::string client_id_;
    bool joined_ = false;
    std::vector<std::shared_ptr<std::string const>> write_queue_;

public:
    explicit Session(tcp::socket&& socket, Server& server)
        : ws_(std::move(socket)), server_(server) {}

    ~Session() = default;

    void run() {
        ws_.set_option(websocket::stream_base::timeout::suggested(beast::role_type::server));
        ws_.read_message_max(1024 * 1024);
        ws_.async_accept(
            beast::bind_front_handler(&Session::on_accept, shared_from_this())
        );
    }

    void send(const std::shared_ptr<std::string const>& message) {
        net::post(
            ws_.get_executor(),
            beast::bind_front_handler(&Session::on_send, shared_from_this(), message)
        );
    }

    const std::string& client_id() const { return client_id_; }
    const std::string& room_id() const { return room_id_; }
    bool is_joined() const { return joined_; }

    void set_joined(const std::string& room_id, const std::string& client_id) {
        room_id_ = room_id;
        client_id_ = client_id;
        joined_ = true;
    }

private:
    void cleanup_from_server();

    void on_accept(beast::error_code ec) {
        if (ec) {
            logMessage("ERROR", "Accept failed: " + ec.message());
            return;
        }
        ws_.text(true);
        do_read();
    }

    void do_read() {
        ws_.async_read(
            buffer_,
            beast::bind_front_handler(&Session::on_read, shared_from_this())
        );
    }

    void on_read(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec == websocket::error::closed) {
            logMessage("INFO", "WebSocket connection closed cleanly");
            if (joined_) {
                cleanup_from_server();
            }
            return;
        }
        if (ec) {
            logMessage("ERROR", "Read error: " + ec.message());
            if (joined_) {
                cleanup_from_server();
            }
            return;
        }

        std::string message_str = beast::buffers_to_string(buffer_.data());
        buffer_.consume(buffer_.size());

        handle_message(message_str);
        do_read();
    }

    void handle_message(const std::string& message);

    void on_send(const std::shared_ptr<std::string const>& message) {
        if (write_queue_.size() >= 256) {
            logMessage("WARNING", "Closing overrun protection: peer write queue is full");
            return;
        }
        write_queue_.push_back(message);
        if (write_queue_.size() == 1) {
            do_write();
        }
    }

    void do_write() {
        ws_.async_write(
            net::buffer(*write_queue_.front()),
            beast::bind_front_handler(&Session::on_write, shared_from_this())
        );
    }

    void on_write(beast::error_code ec, std::size_t bytes_transferred) {
        boost::ignore_unused(bytes_transferred);

        if (ec) {
            logMessage("ERROR", "Write failed: " + ec.message());
            if (joined_) {
                cleanup_from_server();
            }
            return;
        }

        write_queue_.erase(write_queue_.begin());
        if (!write_queue_.empty()) {
            do_write();
        }
    }
};

class Server {
    std::unordered_map<std::string, std::vector<std::shared_ptr<Session>>> room_clients_;
    std::unordered_map<std::string, std::vector<boost::json::object>> room_histories_;
    std::mutex mutex_;

public:
    void join_room(const std::string& room_id, const std::string& client_id, const std::shared_ptr<Session>& session) {
        std::lock_guard<std::mutex> lock(mutex_);
        if (session->is_joined()) {
            logMessage("WARNING", "Ignoring repeated join from client '" + session->client_id() + "'");
            return;
        }

        boost::json::array peers;
        for (const auto& peer : room_clients_[room_id]) {
            peers.push_back(boost::json::string(peer->client_id()));
        }
        session->set_joined(room_id, client_id);
        room_clients_[room_id].push_back(session);
        logMessage("INFO", "Client '" + client_id + "' joined room '" + room_id + "' (Total clients: " + std::to_string(room_clients_[room_id].size()) + ")");

        boost::json::object snapshot;
        snapshot["type"] = "snapshot";
        boost::json::array payload;
        for (const auto& delta : room_histories_[room_id]) {
            payload.push_back(delta);
        }
        snapshot["payload"] = std::move(payload);

        auto msg = std::make_shared<std::string>(boost::json::serialize(snapshot));
        session->send(msg);
        logMessage("INFO", "Sent room snapshot (" + std::to_string(room_histories_[room_id].size()) + " deltas) to client '" + client_id + "'");

        boost::json::object peer_list;
        peer_list["type"] = "peers";
        peer_list["payload"] = std::move(peers);
        session->send(std::make_shared<std::string>(boost::json::serialize(peer_list)));

        boost::json::object joined;
        joined["type"] = "peer-joined";
        joined["clientId"] = client_id;
        auto joined_message = std::make_shared<std::string>(boost::json::serialize(joined));
        for (const auto& peer : room_clients_[room_id]) {
            if (peer != session) peer->send(joined_message);
        }
    }

    void relay_delta(const std::shared_ptr<Session>& sender, const boost::json::object& payload) {
        std::lock_guard<std::mutex> lock(mutex_);
        const std::string& room_id = sender->room_id();
        const std::string& sender_id = sender->client_id();
        room_histories_[room_id].push_back(payload);
        logMessage("INFO", "Relaying delta from client '" + sender_id + "' in room '" + room_id + "'");

        boost::json::object envelope;
        envelope["type"] = "delta";
        envelope["room"] = room_id;
        envelope["clientId"] = sender_id;
        envelope["payload"] = payload;
        auto msg = std::make_shared<std::string>(boost::json::serialize(envelope));
        auto it = room_clients_.find(room_id);
        if (it != room_clients_.end()) {
            for (const auto& client : it->second) {
                if (client->client_id() != sender_id) {
                    client->send(msg);
                }
            }
        }
    }

    void relay_signal(const std::shared_ptr<Session>& sender,
                      const std::string& target_id,
                      const std::string& type,
                      const boost::json::object& source) {
        std::lock_guard<std::mutex> lock(mutex_);
        auto room = room_clients_.find(sender->room_id());
        if (room == room_clients_.end()) return;

        boost::json::object message = source;
        message["type"] = type;
        message["clientId"] = sender->client_id();
        message["targetClientId"] = target_id;
        message.erase("room");

        auto serialized = std::make_shared<std::string>(boost::json::serialize(message));
        for (const auto& peer : room->second) {
            if (peer->client_id() == target_id) {
                peer->send(serialized);
                logMessage("INFO", "Forwarded " + type + " from '" + sender->client_id() + "' to '" + target_id + "'");
                return;
            }
        }
        logMessage("WARNING", "Signaling target '" + target_id + "' is not in sender room");
    }

    void leave_room(const std::shared_ptr<Session>& session) {
        std::lock_guard<std::mutex> lock(mutex_);
        std::string room_id = session->room_id();
        std::string client_id = session->client_id();

        auto it = room_clients_.find(room_id);
        if (it != room_clients_.end()) {
            auto& clients = it->second;
            auto pos = std::find(clients.begin(), clients.end(), session);
            if (pos != clients.end()) {
                clients.erase(pos);
                logMessage("INFO", "Client '" + client_id + "' left room '" + room_id + "' (Remaining clients: " + std::to_string(clients.size()) + ")");
            }
            boost::json::object left;
            left["type"] = "peer-left";
            left["clientId"] = client_id;
            auto message = std::make_shared<std::string>(boost::json::serialize(left));
            for (const auto& peer : clients) peer->send(message);
            if (clients.empty()) room_clients_.erase(it);
        }
    }
};

void Session::cleanup_from_server() {
    server_.leave_room(shared_from_this());
    joined_ = false;
}

void Session::handle_message(const std::string& message) {
    try {
        boost::json::value val = boost::json::parse(message);
        if (!val.is_object()) {
            logMessage("WARNING", "Received invalid non-JSON text frame (not a JSON object)");
            return;
        }
        auto const& obj = val.as_object();
        auto type_it = obj.find("type");
        if (type_it == obj.end() || !type_it->value().is_string()) {
            logMessage("WARNING", "Received message without string 'type'");
            return;
        }
        std::string msg_type = type_it->value().get_string().c_str();

        if (msg_type == "join") {
            std::string room_id = "default";
            auto room_it = obj.find("room");
            if (room_it != obj.end() && room_it->value().is_string()) {
                room_id = room_it->value().get_string().c_str();
            }
            std::string client_id;
            auto client_it = obj.find("clientId");
            if (client_it != obj.end() && client_it->value().is_string()) {
                client_id = client_it->value().get_string().c_str();
            }
            if (client_id.empty() || client_id.size() > 128 || room_id.empty() || room_id.size() > 128) {
                logMessage("WARNING", "Client tried to join room without clientId");
                return;
            }
            server_.join_room(room_id, client_id, shared_from_this());
        } else if (msg_type == "delta") {
            if (!joined_) {
                logMessage("WARNING", "Ignoring delta from a client that has not joined a room");
                return;
            }
            auto payload_it = obj.find("payload");
            if (payload_it == obj.end() || !payload_it->value().is_object()) {
                logMessage("WARNING", "Received delta event with missing or empty payload");
                return;
            }
            boost::json::object payload = payload_it->value().as_object();
            server_.relay_delta(shared_from_this(), payload);
        } else if (msg_type == "offer" || msg_type == "answer" || msg_type == "candidate") {
            if (!joined_) {
                logMessage("WARNING", "Ignoring signaling message from a client that has not joined a room");
                return;
            }
            auto target = obj.find("targetClientId");
            if (target == obj.end() || !target->value().is_string()) {
                logMessage("WARNING", "Signaling message has no targetClientId");
                return;
            }
            std::string target_id = target->value().get_string().c_str();
            if (target_id.empty() || target_id.size() > 128 || target_id == client_id_) {
                logMessage("WARNING", "Invalid signaling target");
                return;
            }
            server_.relay_signal(shared_from_this(), target_id, msg_type, obj);
        } else {
            logMessage("WARNING", "Unhandled message type received: '" + msg_type + "'");
        }
    } catch (const std::exception& e) {
        logMessage("WARNING", "Failed to parse message: " + std::string(e.what()));
    }
}

class Listener : public std::enable_shared_from_this<Listener> {
    net::io_context& ioc_;
    tcp::acceptor acceptor_;
    Server& server_;

public:
    Listener(net::io_context& ioc, tcp::endpoint endpoint, Server& server)
        : ioc_(ioc), acceptor_(net::make_strand(ioc)), server_(server) {
        beast::error_code ec;
        acceptor_.open(endpoint.protocol(), ec);
        if (ec) {
            throw std::runtime_error("open: " + ec.message());
        }
        acceptor_.set_option(net::socket_base::reuse_address(true), ec);
        if (ec) {
            throw std::runtime_error("set_option: " + ec.message());
        }
        acceptor_.bind(endpoint, ec);
        if (ec) {
            throw std::runtime_error("bind: " + ec.message());
        }
        acceptor_.listen(net::socket_base::max_listen_connections, ec);
        if (ec) {
            throw std::runtime_error("listen: " + ec.message());
        }
    }

    void run() {
        do_accept();
    }

private:
    void do_accept() {
        acceptor_.async_accept(
            net::make_strand(ioc_),
            beast::bind_front_handler(&Listener::on_accept, shared_from_this())
        );
    }

    void on_accept(beast::error_code ec, tcp::socket socket) {
        if (ec) {
            logMessage("ERROR", "accept: " + ec.message());
            return;
        } else {
            std::make_shared<Session>(std::move(socket), server_)->run();
        }
        do_accept();
    }
};

int main(int argc, char* argv[]) {
    try {
        unsigned short port = 8080;
        if (argc > 1) {
            port = static_cast<unsigned short>(std::atoi(argv[1]));
        }
        net::io_context ioc;
        Server server;
        auto const address = net::ip::make_address("0.0.0.0");
        auto listener = std::make_shared<Listener>(ioc, tcp::endpoint{address, port}, server);
        listener->run();
        logMessage("INFO", "Collaboration WebSocket server running on ws://localhost:" + std::to_string(port));
        net::signal_set signals(ioc, SIGINT, SIGTERM);
        signals.async_wait(
            [&](beast::error_code const&, int) {
                logMessage("INFO", "WebSocket server stopped manually.");
                ioc.stop();
            }
        );
        ioc.run();
    } catch (std::exception const& e) {
        logMessage("ERROR", std::string("Exception: ") + e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}
