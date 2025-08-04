#pragma once
#include "DrawableObject.h"

class CanvasWidget;

class CanvasSerializer {
public:
	static bool serialize(const CanvasWidget* canvas, const QString& path);
	static bool deserialize(CanvasWidget* canvas, const QString& path);
};
