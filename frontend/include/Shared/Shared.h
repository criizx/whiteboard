#pragma once

#include <QString>
#include <QJsonObject>

enum class ObjType {
    Line = 1,
    BrokenLine = 2,
    Rectangle = 3,
    AssistCircle = 4
};


struct DrawableObjectData {
    QString id;
    ObjType type;
    QJsonObject properties;
    qint64 timestamp = 0;

    DrawableObjectData() = default;
};

