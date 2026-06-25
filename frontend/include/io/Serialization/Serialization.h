
#pragma once

#include <QString>

class CanvasWidget;

class CanvasSerializer {
public:
    static bool serialize(const CanvasWidget* canvas, const QString& path);
    static bool deserialize(CanvasWidget* canvas, const QString& path);

private:
    static constexpr qint32 FILE_VERSION = 1;
    static constexpr qint32 MAGIC_NUMBER = 0x43415356;
};