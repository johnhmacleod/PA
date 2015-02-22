#include "pebble.h"

#include <stdlib.h>

static Window *window;

static TextLayer *text_layer;

static PropertyAnimation *prop_animation;

static int toggle;


//Layerbounds Animation

int16_t getLayerBounds(void *subject)
{
  APP_LOG(APP_LOG_LEVEL_DEBUG, "In bounds getter function:");
  GRect temp = layer_get_bounds((Layer*)subject);
  return(temp.origin.x);
};
 
void setLayerBounds(void *subject, int16_t new_bounds)
{
    // APP_LOG(APP_LOG_LEVEL_INFO, "In setter function: x=%d y=%d w=%d h=%d",new_bounds.origin.x,new_bounds.origin.y,new_bounds.size.w,new_bounds.size.h);
  APP_LOG(APP_LOG_LEVEL_INFO, "In setter with %d", new_bounds);
  GRect b;
  b = layer_get_bounds(subject);
  b.origin.x = new_bounds;
  layer_set_bounds((Layer *)subject, b);
};

int global_fr, global_to;

void myUpdateInt16(PropertyAnimation *anim, int norm)
  {
  int fr, to, val, dif;
  // fr = global_fr;  // Cheat to get the values passed in
  // to = global_to;  // Cheat to get the values passed in

  fr = anim->values.from.int16;
  to = anim->values.to.int16;
  dif = to - fr;
  APP_LOG(APP_LOG_LEVEL_INFO, "In updater with norm=%d fr=%d to=%d", norm, fr, to);
  val = ((float)norm / (ANIMATION_NORMALIZED_MAX - ANIMATION_NORMALIZED_MIN)) * dif + fr;
  ((PropertyAnimationImplementation *)(anim->animation.implementation))->accessors.setter.int16(anim->subject, val);
      
}

static const PropertyAnimationImplementation my_implementation = {
  .base = {
    // using the "stock" update callback:
   // .update = (AnimationUpdateImplementation) property_animation_update_int16,
    .update = (AnimationUpdateImplementation) myUpdateInt16,
  },
  .accessors = {
    // my accessors that get/set a GRect from/onto my subject:
    .setter = { .int16 = (const Int16Setter)setLayerBounds, },
    .getter = { .int16 = (const Int16Getter)getLayerBounds, },
  },
};
 
static void animation_stopped(PropertyAnimation *animation, bool finished, void *data) {
  property_animation_destroy(animation);
}



static void animate_layer_bounds(PropertyAnimation **anim, Layer* layer, GRect *start, GRect *finish, int duration, int delay)
{
  static int s, f;
  s = start->origin.x;
  f = finish->origin.x;
  global_fr = s;  // Cheat to get the values passed in
  global_to = f;  // Cheat to get the values passed in
  
  
  APP_LOG(APP_LOG_LEVEL_INFO, "animate_layer_bounds -> start: x:y w:h %d:%d %d:%d finish: %d:%d %d:%d", 
          (int)start->origin.x, (int)start->origin.y,(int)start->size.w, (int)start->size.h,
          (int)finish->origin.x, (int)finish->origin.y,(int)finish->size.w, (int)finish->size.h);
  *anim = property_animation_create(&my_implementation, layer, &s, &f);
  
  AnimationHandlers handlers = {
        //The reference to the stopped handler is the only one in the array
        .stopped = (AnimationStoppedHandler) animation_stopped
    };
  
  animation_set_handlers((Animation*) *anim, handlers, NULL);
  animation_set_duration((Animation*) *anim, duration);
  animation_set_delay((Animation*) *anim, delay);
  animation_schedule((Animation*) *anim);
}

static void destroy_property_animation(PropertyAnimation **prop_animation) {
  if (*prop_animation == NULL) {
    return;
  }

  if (animation_is_scheduled((Animation*) *prop_animation)) {
    animation_unschedule((Animation*) *prop_animation);
  }

  property_animation_destroy(*prop_animation);
  *prop_animation = NULL;
}

static void click_handler(ClickRecognizerRef recognizer, Window *window) {
  Layer *layer = text_layer_get_layer(text_layer);

  static PropertyAnimation *p1;
  GRect start, finish;
  start = GRect(0,0,144,60); 
  finish = GRect(-30,0,144,60);
  
  switch (click_recognizer_get_button_id(recognizer)) {
    case BUTTON_ID_UP:
          animate_layer_bounds(&p1, layer, &start, &finish, 1000, 0);
      break;
    
    default:
      break;
  }

  }

static void config_provider(Window *window) {
  window_single_click_subscribe(BUTTON_ID_UP, (ClickHandler) click_handler);
}

static void init(void) {
  window = window_create();
  window_set_click_config_provider(window, (ClickConfigProvider) config_provider);
  window_stack_push(window, false);

  text_layer = text_layer_create(GRect(0, 0, 144, 60));
  layer_set_frame((Layer *)text_layer, GRect(0, 0, 60, 60)); //Frame smaller than the bounds
  text_layer_set_text(text_layer, "01234567890ABCDEFGHIJ");
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(text_layer));


}

static void deinit(void) {
  destroy_property_animation(&prop_animation);

  window_stack_remove(window, false);
  window_destroy(window);
  text_layer_destroy(text_layer);
}

int main(void) {
  init();
  app_event_loop();
  deinit();
}

