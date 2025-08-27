#include <QFile>
#include <QDataStream>
#include <QDebug>
#include <memory>

#include <io/Serialization/Serialization.h>
#include <DrawingLogic/CanvasWidget.h>

bool CanvasSerializer::serialize(const CanvasWidget* canvas, const QString& path) {
    if (!canvas) {
        qWarning() << "Cannot serialize null canvas";
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        qWarning() << "Failed to open file for writing:" << path << file.errorString();
        return false;
    }

    QDataStream stream(&file);

    try {
        stream << MAGIC_NUMBER;
        stream << FILE_VERSION;

        const auto& objects = canvas->objects();
        stream << static_cast<qint32>(objects.size());

        for (const auto& obj : objects) {
            if (!obj) {
                qWarning() << "Skipping null object during serialization";
                continue;
            }

            QByteArray objData = obj->toBin();
            stream << objData;
        }

        file.close();
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception during serialization:" << e.what();
        file.close();
        return false;
    }
    catch (...) {
        qWarning() << "Unknown exception during serialization";
        file.close();
        return false;
    }
}

bool CanvasSerializer::deserialize(CanvasWidget* canvas, const QString& path) {
    if (!canvas) {
        qWarning() << "Cannot deserialize to null canvas";
        return false;
    }

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        qWarning() << "Failed to open file for reading:" << path << file.errorString();
        return false;
    }

    QDataStream stream(&file);

    try {
        qint32 magic, version;
        stream >> magic;
        stream >> version;

        if (magic != MAGIC_NUMBER) {
            qWarning() << "Invalid file format - wrong magic number";
            file.close();
            return false;
        }

        if (version != FILE_VERSION) {
            qWarning() << "Unsupported file version:" << version;
            file.close();
            return false;
        }

        canvas->clear_all();

        qint32 objectCount;
        stream >> objectCount;

        if (objectCount < 0) {
            qWarning() << "Invalid object count:" << objectCount;
            file.close();
            return false;
        }

        for (qint32 i = 0; i < objectCount; ++i) {
            QByteArray objData;
            stream >> objData;

            QDataStream objStream(&objData, QIODevice::ReadOnly);

            qint32 typeInt;
            objStream >> typeInt;

            if (typeInt < 1 || typeInt > 4) {
                qWarning() << "Invalid object type:" << typeInt << "for object" << i;
                continue;
            }

            ObjType type = static_cast<ObjType>(typeInt);

            auto obj = DrawableObject::fromBin(objStream, type);

            if (obj) {
                canvas->addObject(obj);
            }
            else {
                qWarning() << "Failed to deserialize object" << i << "of type" << static_cast<int>(type);
            }
        }

        file.close();
        return true;
    }
    catch (const std::exception& e) {
        qWarning() << "Exception during deserialization:" << e.what();
        file.close();
        return false;
    }
    catch (...) {
        qWarning() << "Unknown exception during deserialization";
        file.close();
        return false;
    }
}