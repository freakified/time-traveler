#include <pebble.h>
#include "pdc-transform.h"

typedef struct similarity_transform_data {
  GPoint offset;
  int scale10;
  int32_t c;
  int32_t s;
  GPoint rotation_offset;
  int custom_stroke_width;
}similarity_transform_data;

typedef struct recolor_data {
    GColor fill_color;
    GColor stroke_color;
} recolor_data;

static bool prv_recolor_iterator(GDrawCommand *command, uint32_t index, void *context) {
    recolor_data *data = (recolor_data *) context;
    gdraw_command_set_stroke_color(command, data->stroke_color);
    gdraw_command_set_fill_color(command, data->fill_color);
    return true;
}

static bool prv_similarity_iterator(GDrawCommand *command,uint32_t index, void* context) {
  // get transformation data
  similarity_transform_data *data = (similarity_transform_data*) context;

  // scale stroke width
  
  if (data->custom_stroke_width > 0)
  {  
    gdraw_command_set_stroke_width(command,data->custom_stroke_width);
  }
  else
  {
    gdraw_command_set_stroke_width(command,gdraw_command_get_stroke_width(command)*data->scale10/10);
  }

  // GDrawCommandTypePrecisePath has to be treated differently
  GDrawCommandType type = gdraw_command_get_type(command);
  int scale_factor = (type==GDrawCommandTypePrecisePath)? 8:1;

  GPoint temp_point;
  uint16_t num_points = gdraw_command_get_num_points(command);
  for(uint16_t i=0;i<num_points;i++) {
    temp_point = gdraw_command_get_point(command,i);
    // rotation around point
    const int x = temp_point.x-data->rotation_offset.x*scale_factor;
    const int y = temp_point.y-data->rotation_offset.y*scale_factor;
    temp_point.x = x*data->c/ TRIG_MAX_RATIO - y*data->s/ TRIG_MAX_RATIO + data->rotation_offset.x*scale_factor;
    temp_point.y = x*data->s/ TRIG_MAX_RATIO + y*data->c/ TRIG_MAX_RATIO + data->rotation_offset.y*scale_factor;
    // scaling and transformation
    temp_point.x = (temp_point.x * data->scale10) / 10 + data->offset.x*scale_factor;
    temp_point.y = (temp_point.y * data->scale10) / 10 + data->offset.y*scale_factor;
    gdraw_command_set_point(command,i,temp_point);
  }
  return true;
}

static void prv_gdraw_command_list_draw_transformed(GContext * ctx, GDrawCommandList * list,
                                               GPoint offset, int scale10, GPoint rotation_offset, int rotation, GColor fill_color, GColor stroke_color, int custom_stroke_width) {
  const int32_t angle = DEG_TO_TRIGANGLE(rotation);
  similarity_transform_data data = {
    .scale10 = scale10,
    .offset = offset,
    .c = cos_lookup(angle),
    .s = sin_lookup(angle),
    .rotation_offset = rotation_offset,
    .custom_stroke_width = custom_stroke_width
  };
  gdraw_command_list_iterate(list,prv_similarity_iterator,(void*)&data);

  recolor_data rd = {
    .fill_color = fill_color,
    .stroke_color = stroke_color
  };
  gdraw_command_list_iterate(list,prv_recolor_iterator,(void*)&rd);

  gdraw_command_list_draw(ctx,list);
}

void pdc_transform_gdraw_command_image_draw_transformed(GContext * ctx, GDrawCommandImage * image,
                                     GPoint offset, int scale10, int rotation,
                                     GColor fill_color, GColor stroke_color, int custom_stroke_width) {
  GDrawCommandImage *temp_image = gdraw_command_image_clone(image);
  GDrawCommandList *list = gdraw_command_image_get_command_list(temp_image);
  GSize img_size = gdraw_command_image_get_bounds_size(temp_image);
  GPoint rotation_offset = { .x= img_size.w/2,.y=img_size.h/2};
//  GPoint rotation_offset = { .x= 0,.y=0};
  prv_gdraw_command_list_draw_transformed(ctx,list,offset,scale10,rotation_offset,rotation,fill_color,stroke_color,custom_stroke_width);
  gdraw_command_image_destroy(temp_image);
}

void pdc_transform_scale_image(GDrawCommandImage * image, int scale10) {
  similarity_transform_data data = {
    .scale10 = scale10,
    .offset = GPointZero,
    .c = cos_lookup(0),
    .s = sin_lookup(0),
    .rotation_offset = GPointZero,
    .custom_stroke_width = 0
  };
  GDrawCommandList *list = gdraw_command_image_get_command_list(image);
  gdraw_command_list_iterate(list,prv_similarity_iterator,(void*)&data);

  GSize img_size = gdraw_command_image_get_bounds_size(image);
  img_size.w = (img_size.w * scale10) / 10;
  img_size.h = (img_size.h * scale10) / 10;
  gdraw_command_image_set_bounds_size(image, img_size);
}

void pdc_transform_rotate_image(GDrawCommandImage * image, int rotation) {
  const int32_t angle = DEG_TO_TRIGANGLE(rotation);
  GSize img_size = gdraw_command_image_get_bounds_size(image);
  GPoint rotation_offset = { .x= img_size.w/2,.y=img_size.h/2};
  similarity_transform_data data = {
    .scale10 = 10,
    .offset = GPointZero,
    .c = cos_lookup(angle),
    .s = sin_lookup(angle),
    .rotation_offset = rotation_offset,
    .custom_stroke_width = 0
  };
  GDrawCommandList *list = gdraw_command_image_get_command_list(image);
  gdraw_command_list_iterate(list,prv_similarity_iterator,(void*)&data);
}

void pdc_transform_recolor_image(GDrawCommandImage * image, GColor fill_color, GColor stroke_color) {
  recolor_data data = {
    .fill_color = fill_color,
    .stroke_color = stroke_color
  };
  GDrawCommandList *list = gdraw_command_image_get_command_list(image);
  gdraw_command_list_iterate(list,prv_recolor_iterator,(void*)&data);
}
