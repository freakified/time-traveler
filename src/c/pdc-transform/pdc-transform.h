#pragma once

#include <pebble.h>

void pdc_transform_gdraw_command_image_draw_transformed(GContext *ctx, GDrawCommandImage *image,
                                                        GPoint offset, int scale10, int rotation,
                                                        GColor fill_color, GColor stroke_color,
                                                        int custom_stroke_width);
void pdc_transform_scale_image(GDrawCommandImage *image, int scale10);
void pdc_transform_rotate_image(GDrawCommandImage *image, int rotation);
void pdc_transform_recolor_image(GDrawCommandImage *image, GColor fill_color, GColor stroke_color);
