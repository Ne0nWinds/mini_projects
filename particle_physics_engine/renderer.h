#pragma once
#include "general.h"
#include "vector.h"

typedef struct circle circle;

enum shape_id {
  SHAPE_BOX,
  SHAPE_TRIANGLE,
};

bool InitRenderer();

void AddNewShape(enum shape_id shape_id, u32 count, v2 *sizes);
void RenderShapes(v2 *positions, u32 count);

void AddNewCircles(f32 *widths, u32 count);
void RenderCircles(v2 *circle_positions, u32 count);
void ResizeWindow(u32 window_width, u32 window_height);
void ScreenToWorld(v2 *mouse_pos, u32 window_width, u32 window_height);

extern v2 renderer_coordinates;
