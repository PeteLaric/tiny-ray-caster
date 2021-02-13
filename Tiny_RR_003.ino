/*
  Tiny Ray Caster
  ©2015-08-16 Pete Laric / http://www.PeteLaric.com
  Ported to RetroRocket ©2021-02-11

  RetroRocket code description:
  This 3D ray casting engine was originally written in Borland
  Turbo C in the mid-1990s, then ported to Processing/Java in
  2016, and finally ported to the RetroRocket (Arduino Pro Micro
  with 1" monochrome OLED) in 2021.  The code is currently being
  optimized, to bring the frame rate up to a usable speed.  Rather
  than increment the radius of each ray according to a fixed value,
  we now compute r as a function of wall height, and iterate
  through all possible wall heights, starting at the screen height
  (small radius), and ending when the wall height reaches zero
  (large radius).  Although less intuitive to understand, this
  system is superior performance-wise, because it only scans radii
  that would result in a different wall height than the previously
  scanned radii, thereby scanning the minimum number of radii
  necessary to utilize the maximum available resolution.  This
  results in a significant frame rate increase, from one frame
  every 4-6 seconds, to several frames per second.  Further
  optimizations should allow the frame rate to increase to 20 fps
  or more, which is the goal.

  Processing code description:
  This primitive graphics engine is similar to that of the classic
  Id Software game Wolfenstein 3D.  A 2-dimensional array of blocks
  is created, then these blocks are extruded into the third
  dimension using ray casting (a primitive 3D rendering technique).
  The player is free to explore the world using WASD or arrow key
  controls, and left/right carrot keys to strafe.
*/

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>
#include <Adafruit_SSD1306.h>

#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET     4 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_ADDRESS 0x3C ///< See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32
// PETE'S NOTE: 0x3C is correct address for 128x64 OLEDs I bought on Amazon
Adafruit_SSD1306 display(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);



// TINY STUFF

// *** defines ***
#define MAP_X_DIM   10 // map dimensions
#define MAP_Y_DIM   10

// *** global variables ***

// screen settings
int screen_width = SCREEN_WIDTH,
    screen_height = SCREEN_HEIGHT,
    num_pixels = screen_width * screen_height;

// constants
const int NUM_SLIVERS = screen_width; //360;
const float FOV = 90.0; // field of view (degrees)
const float theta_inc = FOV / NUM_SLIVERS; // degrees to increment theta by
//float theta = player_bearing - (FOV / 2);
//float[] d = new float[NUM_SLIVERS];

int map_block_dim = 2; //6 is max for 128x64 with 10x10 map

// world map
float VIEW_DIST = sqrt(pow(MAP_X_DIM, 2) + pow(MAP_Y_DIM, 2));
char my_map[MAP_X_DIM][MAP_Y_DIM]; // create map ARDUINO
//char[][] map = new char[MAP_X_DIM][MAP_Y_DIM]; // create map PROCESSING
//color[] wall_color_map = new color[16]; // wall color array

// player info
float player_x = MAP_X_DIM / 2.0 + 0.5,
      player_y = MAP_Y_DIM / 2.0 + 0.5,
      player_xv = 0, // velocity
      player_yv = 0,
      player_bearing = 270,
      player_turn_speed = 11.25, //11.25, // degrees per turn step
      player_walk_speed = 0.1; //0.1; // map units per walk step

// drawing
char stroke_color = 1;
char fill_color = 1;

// control
int pin_left = 6,
    pin_right = 7,
    pin_forward = 4;



// TINY STUFF

void step_forward(float distance)
{
  float theta_radians = player_bearing * PI / 180.0; // convert 
  float delta_x = distance * cos(theta_radians);
  float delta_y = distance * sin(theta_radians);
//  player_xv = delta_x; // momentum-based
//  player_yv = delta_y;
  float player_x_new = player_x + delta_x;
  float player_y_new = player_y + delta_y;
  if (my_map[(int)player_x_new][(int)player_y_new] == 0 &&
      player_x_new > 0 &&
      player_y_new > 0 &&
      player_x_new < MAP_X_DIM &&
      player_y_new < MAP_Y_DIM)
  {
    // the coast is clear!
    player_x += delta_x; // position-based
    player_y += delta_y;
  }
  else
  {
    // you can't walk through a wall!
    Serial.println("Attempting to walk through wall... I can't do it!");
  }
}

void step_backward(float distance)
{
  step_forward(-distance);
}

void turn_left(float speed)
{
  player_bearing -= speed;
}

void turn_right(float speed)
{
  player_bearing += speed;
}

void strafe_left(float distance)
{
  turn_left(90);
  step_forward(distance);
  turn_right(90);
}

void strafe_right(float distance)
{
  turn_right(90);
  step_forward(distance);
  turn_left(90);
}


void clear_map()
{
  for (int y = 0; y < MAP_Y_DIM; y++)
  {
    for (int x = 0; x < MAP_X_DIM; x++)
    {
      my_map[x][y] = 0;
    }
  }
}

void randomize_map(int density)
{
  for (int y = 0; y < MAP_Y_DIM; y++)
  {
    for (int x = 0; x < MAP_X_DIM; x++)
    {
      if (random(100) < density)
        my_map[x][y] = 1;
        //my_map[x][y] = (char)(random(12) + 3);
      else
        my_map[x][y] = 0;
    }
  }
  // clear spot for player
  my_map[(int)player_x][(int)player_y] = 0;
}

void print_map()
{
  for (int y = 0; y < MAP_Y_DIM; y++)
  {
    for (int x = 0; x < MAP_X_DIM; x++)
    {
      if (my_map[x][y] != 0)
      {
        Serial.print((int)my_map[x][y], 1);
      }
      else
      {
        Serial.print(".");
      }
    }
    Serial.println();
  }
}

void draw_map()
{
  int c;
  for (int y = 0; y < MAP_Y_DIM; y++)
  {
    for (int x = 0; x < MAP_X_DIM; x++)
    {
      //c = 1; //wall_color_map[(int)my_map[x][y]];
      c = my_map[x][y];
      int x1 = x * map_block_dim,
          y1 = y * map_block_dim,
          x2 = (x + 1) * map_block_dim,
          y2 = (y + 1) * map_block_dim;
      //noStroke();
      //fill(c);
      fill_color = c;
      pete_rect(x1, y1, x2, y2);
    }
  }

  // show player pos on map
  int display_player_x = (int)(player_x * map_block_dim),
      display_player_y = (int)(player_y * map_block_dim);
  c = 1; //wall_color_map[15];
  stroke_color = c;
  float look_dist = 2; // length of line representing player's line of sight
  float player_look_x = display_player_x + look_dist * cos(player_bearing * PI / 180),
        player_look_y = display_player_y + look_dist * sin(player_bearing * PI / 180);
  pete_line(display_player_x, display_player_y, (int)player_look_x, (int)player_look_y);
  put_pixel(display_player_x, display_player_y, c);
}

void pete_rect(int x1, int y1, int x2, int y2)
{
  int c = fill_color;
  for (int y = y1; y < y2; y++)
  {
    for (int x = x1; x < x2; x++)
    {
      put_pixel(x, y, c);
    }
  }
}

void pete_line(int x1, int y1, int x2, int y2)
{
  // x2 must >= x1
  if (x2 < x1)
  {
    int temp = x1; // swap 'em
    x1 = x2;
    x2 = temp;
    // also swap y's!
    temp = y1;
    y1 = y2;
    y2 = temp;
  }
  int c = stroke_color; //g.strokeColor;
  float x_dist = (float)x2 - (float)x1,
        y_dist = (float)y2 - (float)y1;
  if (x1 != x2) // non-vertical line
  {
    float m = y_dist / x_dist; // slope = rise over run
    //m = 2;
    // y = m * x + b
    // b = y - m * x
    float b = y1 - m * x1;
    for (int x = x1; x < x2; x++)
    {
      float y = m * x + b;
      put_pixel((int)x, (int)y, c);
    }
  }
  else // vertical line
  {
    float x = x1;
    float y_start,
          y_end;
    if (y1 < y2)
    {
      y_start = y1;
      y_end = y2;
    }
    else
    {
      y_start = y2;
      y_end = y1;
    }
    for (float y = y_start; y < y_end; y++)
    {
      put_pixel((int)x, (int)y, c);
    }
  }    
}

void draw_map_small()
{
  for (int y = 0; y < MAP_Y_DIM; y++)
  {
    for (int x = 0; x < MAP_X_DIM; x++)
    {
      int c; //color c;
      //if (my_map[x][y] != 0)
      //{
        c = my_map[x][y]; //wall_color_map[(int)my_map[x][y]];
      //}
      //else
      put_pixel(x, y, c);
    }
  }
}


void put_pixel(int x, int y, int c)
{
//  int i = y * screen_width + x;
//  if (i >= 0 && i < num_pixels)
//    pixels[i] = c;
  if (c > 0)
    display.drawPixel(x, y, SSD1306_WHITE);
  else
    display.drawPixel(x, y, SSD1306_BLACK);
}



void ray_cast()
{
  float theta = player_bearing - (FOV / 2);
  float d[NUM_SLIVERS];
  
  for (int sliver = 0; sliver < NUM_SLIVERS; sliver++)
  {
    //float r_inc = 0.001; // amount to increment r by
    //for (float r = 0.01; r < VIEW_DIST; r += r_inc)
    for (float wall_height = screen_height; wall_height > 0; wall_height-=2)
    {
      float k = 0.5;
      float r = (k * screen_height) / wall_height;
      float theta_radians = theta * PI / 180.0; // convert
      float x = player_x + r * cos(theta_radians);
      float y = player_y + r * sin(theta_radians);
      // ensure we're within bounds before trying to access map
      if (x < 0 || y < 0 || x >= MAP_X_DIM || y >= MAP_Y_DIM
          || my_map[(int)x][(int)y] != 0)
      {
        d[sliver] = r;
        //float wall_height = 0.5 * screen_height / r;
        //if (wall_height > screen_height) wall_height = screen_height;
        int wall_code;
        if (x < 0 || y < 0 || x >= MAP_X_DIM || y >= MAP_Y_DIM)
          wall_code = 4; // out of bounds
        else wall_code = (int)my_map[(int)x][(int)y];
        int wall_color = 1; //wall_color_map[wall_code];
        render_sliver(sliver, wall_height, wall_color);
        break;
      }
      //r_inc *= 1.007;
    }
    theta += theta_inc;
  }
}


void render_sliver(int x, float wall_height, int wall_color)
{
  float screen_middle_y = screen_height / 2.0;
  float half_wall_height = wall_height / 2.0;
  float sliver_y1 = screen_middle_y - half_wall_height,
        sliver_y2 = screen_middle_y + half_wall_height;
  float wall_shade = (wall_height / screen_height);
  //if (wall_shade > 1) wall_shade = 1;
  wall_color = 1; //wall_shade;
  //put_pixel(x, screen_middle_y - half_wall_height, wall_color); // top of wall
  //put_pixel(x, screen_middle_y + half_wall_height, wall_color); // bottom of wall
  stroke_color = wall_color;
  pete_line(x, screen_middle_y - half_wall_height, x, screen_middle_y + half_wall_height);
  /*
  for (int y = 0; y < screen_height; y++)
  {
    float sky_shade = (abs(y - screen_middle_y) / screen_height) * 255;
    int sky_color = sky_shade; //color(0, 0, sky_shade);
    float ground_shade = (abs(y - screen_middle_y) / screen_height) * 255;
    int ground_color = ground_shade; //color(0, ground_shade, 0);
    if (y < screen_middle_y - half_wall_height) // sky
      put_pixel(x, y, sky_color);
    else if (y > screen_middle_y + half_wall_height) // ground
      put_pixel(x, y, ground_color);
    else
      put_pixel(x, y, wall_color); // wall
  }
  */
}








void move_randomly()
{
  int r = random(0, 4);
  switch (r)
  {
    case 0:
      Serial.println("TURNING LEFT");
      turn_left(player_turn_speed);
      break;
    case 1:
      Serial.println("TURNING RIGHT");
      turn_right(player_turn_speed);
      break;
    case 2:
      Serial.println("STEPPING FORWARD");
      step_forward(player_walk_speed);
      break;
    case 3:
      Serial.println("STEPPING BACKWARD");
      step_backward(player_walk_speed);
      break;
  }
}




void setup()
{
  Serial.begin(9600);

  // SSD1306_SWITCHCAPVCC = generate display voltage from 3.3V internally
  if(!display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS)) {
    Serial.println(F("SSD1306 allocation failed"));
    for(;;); // Don't proceed, loop forever
  }

  // TINY STUFF

  randomSeed(1); // seed the PRNG for repeatable behavior
  
  clear_map();
  randomize_map(17);
  print_map();

  // pin modes
  pinMode(pin_left, INPUT_PULLUP);
  pinMode(pin_right, INPUT_PULLUP);
  pinMode(pin_forward, INPUT_PULLUP);
  
}




void loop()
{
  display.clearDisplay();

  // TINY STUFF
  ray_cast();
  draw_map();
  //draw_map_small();
  
  display.display();

  // momentum-based motion
//  player_x += player_xv;
//  player_y += player_yv;
//  player_xv *= 0.9; // player velocity decay
//  player_yv *= 0.9;

  // button control
  if (!digitalRead(pin_forward))
    step_forward(player_walk_speed);
  if (!digitalRead(pin_left))
    turn_left(player_turn_speed);
  if (!digitalRead(pin_right))
    turn_right(player_turn_speed);
    
  //move_randomly();

  
  //step_forward(1);
  //turn_left(player_turn_speed);

}
