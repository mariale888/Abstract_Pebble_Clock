/**
  @author Maria Alejandra Montenegro
  Implementation of an Abstract clock.
  
  Consists of cubes showig the number of hours
  and a cube that grows after every minute.
  
  The user can also use the accelerometer to move
  the cubes around into funny puzzles.
  
  Collision detection was also implemented only
  for the hour/ minute cubes.
**/

#include <pebble.h>

#define NUM_CUBES 12
#define MATH_PI 3.141592653589793238462
#define CUBE_DENSITY 0.25
#define CUBE_MASS 50
#define ACCEL_RATIO 0.05
#define ACCEL_STEP_MS 50
  
typedef struct Vec2d {
  double x;
  double y;
} Vec2d;

typedef struct MyTime {
  int hour;
  int minutes;
  int minIndex;
} MyTime;

typedef struct Cube {
  GRect rec;
  //Vec2d pos;
  Vec2d vel;
  double curHeight;
  double height;
  double width;
} Cube;

static Cube cubes[NUM_CUBES];
static Cube mainCube;
static Cube secondCube;

static Window *my_window;
static TextLayer *my_time_layer;
static AppTimer *timer;

static Layer *cube_layer;
static Layer *secondCube_layer;

static GRect window_frame;
static GRect hour_frame;

static MyTime myTime;

//function to init all the main cubes
static void cube_init(Cube *cube, GRect frame, double height) {
  cube->rec    = frame;
  cube->width  = frame.size.w;
  cube->height = frame.size.h;
  cube->curHeight = height;
  cube->vel.x    = 0;
  cube->vel.y    = 0;
}

//function to set up the hour cube grid
static void clock_cube_init(){
  double w = hour_frame.size.w / 3.0;
  double h = hour_frame.size.h / 4.0;
  int x = 0; 
  int y = -1;
  for (int i = 0; i < NUM_CUBES; i++) {
    x = i%3;
    if(x == 0)y++;
    GRect frame = GRect(w * x, h * y, w, h);
    cube_init(&cubes[i], frame, 0);
  }
}

//function that is in charge of just drawing all the cubes
static void cubes_draw(GContext *ctx, Cube *cube, GColor color, bool isFill ) {
  graphics_context_set_fill_color(ctx, color);
  if(isFill) {
    graphics_fill_rect (ctx, GRect(0,0, cube->width,cube->curHeight), 0,GCornerNone);
  }
  else {
     graphics_fill_rect(ctx,GRect(cube->rec.origin.x, cube->rec.origin.y, cube->width,cube->curHeight),0, GCornerNone);
     graphics_context_set_stroke_color(ctx, GColorWhite);
     graphics_draw_round_rect(ctx,GRect(cube->rec.origin.x, cube->rec.origin.y, cube->width,cube->curHeight),0);
  }
}

//function callback update to update cubes
static void secondCube_layer_update_callback(Layer *me, GContext *ctx) {
  cubes_draw(ctx,&secondCube,GColorBlack, true);
}

//function callback update to update cubes
static void cube_layer_update_callback(Layer *me, GContext *ctx) {
  cubes_draw(ctx,&mainCube,GColorWhite, true);
  for (int i = 0; i < NUM_CUBES; i++) {
    cubes_draw(ctx, &cubes[i], GColorBlack, false);
  }
}

//function to set hour cubes
static void update_hour(){
   // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

   //set hours and min cubes
  int hrTime = 12;
  int count = 0;
  int hr = tick_time->tm_hour;
  //converting to 12 hr clock
  if(hr > hrTime )hr -= hrTime; 
  myTime.hour = hr;
  
  //reset cubes
  for(int i=0;i< NUM_CUBES;i++){
    cubes[i].curHeight = 0;
  }
  if(hr == hrTime) {
    for(int i = 0; i < hr; i++) 
      cubes[i].curHeight = cubes[i].height;
    hr = 0;
  }
  
  for(int i = 0; i < hr; i++){
    int ran = rand()%hrTime;
    count = 0;
    while(cubes[ran].curHeight > 0 && count < 10){
      ran = rand()%hrTime;
      count++;
    }
    cubes[ran].curHeight = cubes[ran].height;
  }
  //reseting minuteIndex bc new hr
   myTime.minIndex = -1;
}
//function to set min cubes
static void update_minutes(){
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);
  int min = tick_time->tm_min;
  myTime.minutes = min;
  
  //if setting up
  if( myTime.minIndex == -1){
     //setting minutes
    int hrTime = 12;
    int count = 0;
  
    int ran = rand()%hrTime;
    while(cubes[ran].curHeight > 0 && count < 10) {
      count++;
      ran = rand()%hrTime;
    }
     myTime.minIndex = ran;
  }
  cubes[ myTime.minIndex].curHeight = (min * cubes[ myTime.minIndex].height)/60.0;
}

// function that init the clock at load
static void init_time() {
  /* Intializes random number generator */
  time_t t;
  srand((unsigned) time(&t));
  //set seconds
 // cube_init(&secondCube, window_frame, 50);
  
  //set hours & minutes
  update_hour();
  update_minutes();
}

// function that updates interface every second
static void update_time() {
  // Get a tm structure
  time_t temp = time(NULL); 
  struct tm *tick_time = localtime(&temp);

  // Create a long-lived buffer
  static char buffer[] = "";
  // Display this time on the TextLayer
  text_layer_set_text(my_time_layer, buffer);
  //updating sec.
  secondCube.curHeight = (tick_time->tm_sec * window_frame.size.h)/60.0;
  
  //checking to update other time handles (hours & minutes)
  int hr = tick_time->tm_hour;
  if(hr > 12)hr -= 12;
 
  if(myTime.hour != hr)
    update_hour();
  if(myTime.minutes != tick_time->tm_min)
    update_minutes();
}

//function to give the service to call whenever the time changes
static void tick_handler_sec(struct tm *tick_time, TimeUnits units_changed) {
  //calls main function that chages graphics
  update_time();
}

//------------
// Accelerometer / CollisFunctions 
//------------


static void cube_apply_force(Cube *cube, Vec2d force) {
  cube->vel.x += force.x / CUBE_MASS;
  cube->vel.y += force.y / CUBE_MASS;
}

static void cube_apply_accel(Cube *cube, AccelData accel) {
  Vec2d force;
  force.x = accel.x * ACCEL_RATIO;
  force.y = -accel.y * ACCEL_RATIO;
  cube_apply_force(cube, force);
}

static void cube_update(Cube *cube) {
  const GRect frame = hour_frame; // frame of where hr cubes should be!
  double e = 0.5;
  if ((cube->rec.origin.x < 0 && cube->vel.x < 0)
    || (cube->rec.origin.x + cube->width > frame.size.w && cube->vel.x > 0)) {
    cube->vel.x = -cube->vel.x * e;
  }
  if ((cube->rec.origin.y < 0 && cube->vel.y < 0)
    || (cube->rec.origin.y + cube->height > frame.size.h && cube->vel.y > 0)) {
    cube->vel.y = -cube->vel.y * e;
  }
  cube->rec.origin.x += cube->vel.x;
  cube->rec.origin.y += cube->vel.y;
}


static void timer_callback(void *data) {
  AccelData accel = (AccelData) { .x = 0, .y = 0, .z = 0 };
  accel_service_peek(&accel);
 //sending accelerometer info to all the hr cubes
  for (int i = 0; i < NUM_CUBES; i++) {
    Cube *cube = &cubes[i];
    cube_apply_accel(cube, accel);
    cube_update(cube);
  }

  //mark layer dirty so it calld calls draw fun. again
  layer_mark_dirty(cube_layer);
  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

//-------------------
// LOAD/UNLOAD FUNC.
//-------------------
//function called when window loads
static void main_window_load(Window *window) {
  Layer *window_layer = window_get_root_layer(window);
  GRect window_bounds = layer_get_bounds(window_layer);
  GRect frame = window_frame = layer_get_frame(window_layer);

  // Create time TextLayer
  my_time_layer = text_layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  // Add it as a child layer to the Window's root layer
  layer_add_child(window_get_root_layer(window), text_layer_get_layer(my_time_layer));

  //------
  //adding cube layers
  //------
  //fist adding seconds layers
  secondCube_layer = layer_create(GRect(0, 0, window_bounds.size.w, window_bounds.size.h));
  layer_set_update_proc(secondCube_layer, secondCube_layer_update_callback);
  layer_add_child(window_layer, secondCube_layer);
 
  //second adding rest of cubes
  hour_frame = GRect(20, 20, frame.size.w - 41, frame.size.h - 40);
  cube_layer = layer_create(hour_frame);
  layer_set_update_proc(cube_layer, cube_layer_update_callback);
  layer_add_child(secondCube_layer, cube_layer);
  // update time
  update_time();
  
  //init squares
  cube_init(&secondCube, window_frame, 0);
  cube_init(&mainCube, hour_frame, hour_frame.size.h);
  clock_cube_init();
  
  ////Set clock:
  init_time();
}

// function called on window unload
static void main_window_unload(Window *window) {
  // Destroy TextLayer
 // text_layer_destroy(my_time_layer);
  layer_destroy(secondCube_layer);
  //layer_destroy(cube_layer);
}

static void init() {
  // Create main Window element and assign to pointer
  my_window = window_create();

  // Set handlers to manage the elements inside the Window
  window_set_window_handlers(my_window, (WindowHandlers) {
    .load = main_window_load,
    .unload = main_window_unload
  });

  // Show the Window on the watch, with animated=true
  window_stack_push(my_window, true);
  
  // Register with TickTimerService
  tick_timer_service_subscribe(SECOND_UNIT, tick_handler_sec);
 
  //accelerometer init
  accel_data_service_subscribe(0, NULL);
  timer = app_timer_register(ACCEL_STEP_MS, timer_callback, NULL);
}

static void deinit() {
  // Destroy Window
  window_destroy(my_window);
  accel_data_service_unsubscribe();
}

int main(void) {
  //setting minIndex to null from beginning 
  myTime.minIndex = -1;
  init();
  app_event_loop();
  deinit();
}
