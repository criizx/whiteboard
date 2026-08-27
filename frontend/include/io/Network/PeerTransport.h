#pragma once

#include <memory>
#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QString>

class PeerTransport final : public QObject {
    Q_OBJECT

public:
    struct Impl;

    explicit PeerTransport(QObject* parent = nullptr);
    ~PeerTransport() override;

    void connectToPeers(const QJsonArray& peerIds);
    void handleSignal(const QJsonObject& message);
    void removePeer(const QString& peerId);
    int broadcastDelta(const QJsonObject& delta);
    void sendSnapshot(const QString& peerId, const QJsonArray& snapshot);
    void reset();

signals:
    void signalToSend(const QJsonObject& message);
    void deltaReceived(const QJsonObject& delta);
    void snapshotRequested(const QString& peerId);
    void peerCountChanged(int count);

private:
    std::unique_ptr<Impl> m_impl;
};
