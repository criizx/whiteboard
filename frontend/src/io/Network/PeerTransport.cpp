#include <io/Network/PeerTransport.h>

#include <QJsonDocument>
#include <QMetaObject>
#include <QPointer>

#ifdef WHITEBOARD_HAS_WEBRTC
#include <rtc/rtc.hpp>
#include <mutex>
#include <unordered_map>
#include <variant>

struct PeerTransport::Impl {
    rtc::Configuration configuration;
    std::unordered_map<std::string, std::shared_ptr<rtc::PeerConnection>> connections;
    std::unordered_map<std::string, std::shared_ptr<rtc::DataChannel>> channels;
    std::mutex mutex;
};

static void postJson(PeerTransport* transport, const std::string& peerId, const std::string& message) {
    QPointer<PeerTransport> guard(transport);
    QMetaObject::invokeMethod(transport, [guard, peerId, message] {
        if (!guard) return;
        QJsonParseError error;
        const auto document = QJsonDocument::fromJson(QByteArray::fromStdString(message), &error);
        if (error.error != QJsonParseError::NoError || !document.isObject()) return;
        const auto frame = document.object();
        const QString type = frame.value("type").toString();
        if (type == "delta") {
            emit guard->deltaReceived(frame.value("payload").toObject());
        } else if (type == "snapshot-request") {
            emit guard->snapshotRequested(QString::fromStdString(peerId));
        } else if (type == "snapshot") {
            for (const auto& item : frame.value("payload").toArray()) {
                const auto delta = item.toObject();
                if (!delta.isEmpty()) emit guard->deltaReceived(delta);
            }
        }
    }, Qt::QueuedConnection);
}

static void attachChannel(PeerTransport* transport,
                          PeerTransport::Impl* impl,
                          const std::string& peerId,
                          const std::shared_ptr<rtc::DataChannel>& channel,
                          bool requestSnapshot) {
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        impl->channels[peerId] = channel;
    }
    QPointer<PeerTransport> guard(transport);
    channel->onOpen([guard, impl, peerId, channel, requestSnapshot] {
        if (requestSnapshot) channel->send(R"({"type":"snapshot-request"})");
        QMetaObject::invokeMethod(guard, [guard, impl] {
            if (!guard) return;
            int count = 0;
            std::lock_guard<std::mutex> lock(impl->mutex);
            for (const auto& [id, item] : impl->channels) {
                if (item && item->isOpen()) ++count;
            }
            emit guard->peerCountChanged(count);
        }, Qt::QueuedConnection);
    });
    channel->onMessage([transport, peerId](const rtc::message_variant& data) {
        if (std::holds_alternative<std::string>(data)) {
            postJson(transport, peerId, std::get<std::string>(data));
        }
    });
    channel->onClosed([guard, impl, peerId] {
        {
            std::lock_guard<std::mutex> lock(impl->mutex);
            impl->channels.erase(peerId);
        }
        QMetaObject::invokeMethod(guard, [guard, impl] {
            if (!guard) return;
            int count = 0;
            std::lock_guard<std::mutex> lock(impl->mutex);
            for (const auto& [id, item] : impl->channels) {
                if (item && item->isOpen()) ++count;
            }
            emit guard->peerCountChanged(count);
        }, Qt::QueuedConnection);
    });
}

static std::shared_ptr<rtc::PeerConnection> createConnection(PeerTransport* transport,
                                                              PeerTransport::Impl* impl,
                                                              const std::string& peerId) {
    auto connection = std::make_shared<rtc::PeerConnection>(impl->configuration);
    QPointer<PeerTransport> guard(transport);
    connection->onLocalDescription([guard, peerId](rtc::Description description) {
        QJsonObject signal;
        signal["type"] = QString::fromStdString(description.typeString());
        signal["targetClientId"] = QString::fromStdString(peerId);
        signal["description"] = QString::fromStdString(std::string(description));
        QMetaObject::invokeMethod(guard, [guard, signal] {
            if (guard) emit guard->signalToSend(signal);
        }, Qt::QueuedConnection);
    });
    connection->onLocalCandidate([guard, peerId](rtc::Candidate candidate) {
        QJsonObject signal;
        signal["type"] = "candidate";
        signal["targetClientId"] = QString::fromStdString(peerId);
        signal["candidate"] = QString::fromStdString(std::string(candidate));
        signal["mid"] = QString::fromStdString(candidate.mid());
        QMetaObject::invokeMethod(guard, [guard, signal] {
            if (guard) emit guard->signalToSend(signal);
        }, Qt::QueuedConnection);
    });
    connection->onDataChannel([transport, impl, peerId](std::shared_ptr<rtc::DataChannel> channel) {
        attachChannel(transport, impl, peerId, channel, false);
    });
    {
        std::lock_guard<std::mutex> lock(impl->mutex);
        impl->connections[peerId] = connection;
    }
    return connection;
}

PeerTransport::PeerTransport(QObject* parent) : QObject(parent), m_impl(std::make_unique<Impl>()) {
    m_impl->configuration.iceServers.emplace_back("stun:stun.l.google.com:19302");
}

PeerTransport::~PeerTransport() {
    reset();
}

void PeerTransport::connectToPeers(const QJsonArray& peerIds) {
    for (const auto& value : peerIds) {
        const std::string peerId = value.toString().toStdString();
        if (peerId.empty()) continue;
        auto connection = createConnection(this, m_impl.get(), peerId);
        attachChannel(this, m_impl.get(), peerId, connection->createDataChannel("whiteboard"), true);
    }
}

void PeerTransport::handleSignal(const QJsonObject& message) {
    const std::string peerId = message.value("clientId").toString().toStdString();
    const QString type = message.value("type").toString();
    if (peerId.empty()) return;

    std::shared_ptr<rtc::PeerConnection> connection;
    {
        std::lock_guard<std::mutex> lock(m_impl->mutex);
        auto found = m_impl->connections.find(peerId);
        if (found != m_impl->connections.end()) connection = found->second;
    }
    if (!connection && type == "offer") connection = createConnection(this, m_impl.get(), peerId);
    if (!connection) return;

    if (type == "offer" || type == "answer") {
        connection->setRemoteDescription(rtc::Description(
            message.value("description").toString().toStdString(), type.toStdString()));
    } else if (type == "candidate") {
        connection->addRemoteCandidate(rtc::Candidate(
            message.value("candidate").toString().toStdString(),
            message.value("mid").toString().toStdString()));
    }
}

void PeerTransport::removePeer(const QString& peerId) {
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->channels.erase(peerId.toStdString());
    m_impl->connections.erase(peerId.toStdString());
}

int PeerTransport::broadcastDelta(const QJsonObject& delta) {
    QJsonObject frame;
    frame["type"] = "delta";
    frame["payload"] = delta;
    const std::string message = QJsonDocument(frame).toJson(QJsonDocument::Compact).toStdString();
    int sent = 0;
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    for (const auto& [id, channel] : m_impl->channels) {
        if (channel && channel->isOpen()) {
            channel->send(message);
            ++sent;
        }
    }
    return sent;
}

void PeerTransport::sendSnapshot(const QString& peerId, const QJsonArray& snapshot) {
    QJsonObject frame;
    frame["type"] = "snapshot";
    frame["payload"] = snapshot;
    const std::string message = QJsonDocument(frame).toJson(QJsonDocument::Compact).toStdString();
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    auto found = m_impl->channels.find(peerId.toStdString());
    if (found != m_impl->channels.end() && found->second && found->second->isOpen()) {
        found->second->send(message);
    }
}

void PeerTransport::reset() {
    if (!m_impl) return;
    std::lock_guard<std::mutex> lock(m_impl->mutex);
    m_impl->channels.clear();
    m_impl->connections.clear();
}

#else

struct PeerTransport::Impl {};

PeerTransport::PeerTransport(QObject* parent) : QObject(parent), m_impl(std::make_unique<Impl>()) {}
PeerTransport::~PeerTransport() = default;
void PeerTransport::connectToPeers(const QJsonArray&) {}
void PeerTransport::handleSignal(const QJsonObject&) {}
void PeerTransport::removePeer(const QString&) {}
int PeerTransport::broadcastDelta(const QJsonObject&) { return 0; }
void PeerTransport::sendSnapshot(const QString&, const QJsonArray&) {}
void PeerTransport::reset() {}

#endif
