/*
  Tiny Ray Caster
  ©2015-08-16 Peter Alaric DeSimone
  http://www.PeterAlaric.com
  
  This primitive graphics engine is similar to that of the classic
  Id Software game Wolfenstein 3D.  A 2-dimensional array of blocks
  is created, then these blocks are extruded into the third
  dimension using ray casting (a primitive 3D rendering technique).
  The player is free to explore the world using WASD or arrow key
  controls, and left/right carrot keys to strafe.
  
  NEW IN 0.010: variables in size() no longer supported by this version of Processing (P3).
*/

// *** global variables ***

// screen settings
int screen_width = 800,
    screen_height = 600,
    num_pixels = screen_width * screen_height;

// world map
int MAP_X_DIM = 10, // map dimensions
    MAP_Y_DIM = 10;
float VIEW_DIST = sqrt(pow(MAP_X_DIM, 2) + pow(MAP_Y_DIM, 2));
char[][] map = new char[MAP_X_DIM][MAP_Y_DIM]; // create map
color[] wall_color_map = new color[16]; // wall color array

// player info
float player_x = MAP_X_DIM / 2.0 + 0.5,
      player_y = MAP_Y_DIM / 2.0 + 0.5,
      player_xv = 0, // velocity
      player_yv = 0,
      player_bearing = 270,
      player_turn_speed = 11.25, // degrees per turn step
      player_walk_speed = 0.1; // map units per walk step


void setup()
{
  size(800, 600);
  screen_width = width;
  screen_height = height;
  
  randomSeed(1); // seed the PRNG for repeatable behavior
  
  clear_map();
  randomize_map(17);
  print_map();
  
  assign_wall_colors();
}


void draw()
{
  loadPixels();
  ray_cast();
  draw_map();
  updatePixels();
  if (keyPressed) scan_keyboard();
  player_x += player_xv;
  player_y += player_yv;
  player_xv *= 0.9; // player velocity decay
  player_yv *= 0.9;
}


void scan_keyboard()
{
  // walk forward
  if (key == 'w' || key == 'W' || (key == CODED && keyCode == UP))
  {
    step_forward(player_walk_speed);
  }
  // walk backward
  if (key == 's' || key == 'S' || (key == CODED && keyCode == DOWN))
  {
    step_backward(player_walk_speed);
  }
  // turn left
  if (key == 'a' || key == 'A' || (key == CODED && keyCode ==LEFT))
  {
    turn_left(player_turn_speed);
  }
  // turn right
  if (key == 'd' || key == 'D' || (key == CODED && keyCode == RIGHT))
  {
    turn_right(player_turn_speed);
  }
  // strafe left
  if (key == '<' || key == ',')
  {
    strafe_left(player_walk_speed);
  }
  // strafe right
  if (key == '>' || key == '.')
  {
    strafe_right(player_walk_speed);
  }
  // screen shot
  if (key == ' ')
  {
    save("screenshot.png");
  }
}


// map the CGA color palette to the various wall colors
void assign_wall_colors()
{
  wall_color_map[0] = color(0x00, 0x00, 0x00); // 0=black
  wall_color_map[1] = color(0x00, 0x00, 0xAA); // 1=blue
  wall_color_map[2] = color(0x00, 0xAA, 0x00); // 2=green
  wall_color_map[3] = color(0x00, 0xAA, 0xAA); // 3=cyan
  wall_color_map[4] = color(0xAA, 0x00, 0x00); // 4=red
  wall_color_map[5] = color(0xAA, 0x00, 0xAA); // 5=magenta
  wall_color_map[6] = color(0xAA, 0x55, 0x00); // 6=brown
  wall_color_map[7] = color(0xAA, 0xAA, 0xAA); // 7=light gray
  wall_color_map[8] = color(0x55, 0x55, 0x55); // 8=gray
  wall_color_map[9] = color(0x55, 0x55, 0xFF); // 9=light blue
  wall_color_map[10] = color(0x55, 0xFF, 0x55); // 10=light green
  wall_color_map[11] = color(0x55, 0xFF, 0xFF); // 11=light cyan
  wall_color_map[12] = color(0xFF, 0x55, 0x55); // 12=light red
  wall_color_map[13] = color(0xFF, 0x55, 0xFF); // 13=light magenta
  wall_color_map[14] = color(0xFF, 0xFF, 0x55); // 14=yellow
  wall_color_map[15] = color(0xFF, 0xFF, 0xFF); // 15=white(intense)
}


void step_forward(float distance)
{
  float theta_radians = player_bearing * PI / 180.0; // convert 
  float delta_x = distance * cos(theta_radians);
  float delta_y = distance * sin(theta_radians);
  player_xv = delta_x;
  player_yv = delta_y;
  //player_x += delta_x;
  //player_y += delta_y;
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
      map[x][y] = 0;
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
        map[x][y] = (char)(random(12) + 3);
      else
        map[x][y] = 0;
    }
  }
  // clear spot for player
  map[(int)player_x][(int)player_y] = 0;
}

void print_map()
{
  for (int y = 0; y < MAP_Y_DIM; y++)
  {
    for (int x = 0; x < MAP_X_DIM; x++)
    {
      if (map[x][y] != 0)
        print(hex((int)map[x][y], 1));
      else
        print(".");
    }
    println();
  }
}

void draw_map()
{
  int map_block_dim = 20;
  color c;
  for (int y = 0; y < MAP_Y_DIM; y++)
  {
    for (int x = 0; x < MAP_X_DIM; x++)
    {
      c = wall_color_map[(int)map[x][y]];
      int x1 = x * map_block_dim,
          y1 = y * map_block_dim,
          x2 = (x + 1) * map_block_dim,
          y2 = (y + 1) * map_block_dim;
      noStroke();
      fill(c);
      peter_rect(x1, y1, x2, y2);
    }
  }
  int display_player_x = (int)(player_x * map_block_dim),
      display_player_y = (int)(player_y * map_block_dim);
  c = wall_color_map[15];
  stroke(c);
  float player_look_x = display_player_x + 10 * cos(player_bearing * PI / 180),
        player_look_y = display_player_y + 10 * sin(player_bearing * PI / 180);
  peter_line(display_player_x, display_player_y, (int)player_look_x, (int)player_look_y);
  put_pixel(display_player_x, display_player_y, c);
}

void peter_rect(int x1, int y1, int x2, int y2)
{
  /*
  from https://processing.org/discourse/beta/num_1205522677.html
  Q: Is there a way to get the actual fill and stroke color of a
  PApplet object? A: yeah... seems to be undocumented but works:
  access g.fillColor and g.strokeColor directly
  */
  color c = g.fillColor;
  for (int y = y1; y < y2; y++)
  {
    for (int x = x1; x < x2; x++)
    {
      put_pixel(x, y, c);
    }
  }
}

void peter_line(int x1, int y1, int x2, int y2)
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
  color c = g.strokeColor;
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
      color c;
      //if (map[x][y] != 0)
      //{
        c = wall_color_map[(int)map[x][y]];
      //}
      //else
      put_pixel(x, y, c);
    }
  }
}


void put_pixel(int x, int y, color c)
{
  int i = y * screen_width + x;
  if (i >= 0 && i < num_pixels)
    pixels[i] = c;
}

color get_pixel(int x, int y)
{
  //return get(x, y);
  int i = y * screen_width + x;
  return pixels[i];
}


void ray_cast()
{
  int NUM_SLIVERS = screen_width; //360;
  float FOV = 90.0; // field of view (degrees)
  float theta_inc = FOV / NUM_SLIVERS; // degrees to increment theta by
  float theta = player_bearing - (FOV / 2);
  float[] d = new float[NUM_SLIVERS];
  
  for (int sliver = 0; sliver < NUM_SLIVERS; sliver++)
  {
    float r_inc = 0.001; // amount to increment r by
    for (float r = 0.01; r < VIEW_DIST; r += r_inc)
    {
      float theta_radians = theta * PI / 180.0; // convert
      float x = player_x + r * cos(theta_radians);
      float y = player_y + r * sin(theta_radians);
      // ensure we're within bounds before trying to access map
      if (x < 0 || y < 0 || x >= MAP_X_DIM || y >= MAP_Y_DIM
          || map[(int)x][(int)y] != 0)
      {
        d[sliver] = r;
        float wall_height = 0.5 * screen_height / r;
        if (wall_height > screen_height) wall_height = screen_height;
        int wall_code;
        if (x < 0 || y < 0 || x >= MAP_X_DIM || y >= MAP_Y_DIM)
          wall_code = 4; // out of bounds
        else wall_code = (int)map[(int)x][(int)y];
        color wall_color = wall_color_map[wall_code];
        render_sliver(sliver, wall_height, wall_color);
        break;
      }
      r_inc *= 1.007;
    }
    theta += theta_inc;
  }
}


void render_sliver(int x, float wall_height, color wall_color)
{
  float screen_middle_y = screen_height / 2.0;
  float half_wall_height = wall_height / 2.0;
  float sliver_y1 = screen_middle_y - half_wall_height,
        sliver_y2 = screen_middle_y + half_wall_height;
  float wall_shade = (wall_height / screen_height);
  if (wall_shade > 1) wall_shade = 1;
  wall_color = color(red(wall_color) * wall_shade,
                     green(wall_color) * wall_shade,
                     blue(wall_color) * wall_shade);
  for (int y = 0; y < screen_height; y++)
  {
    float sky_shade = (abs(y - screen_middle_y) / screen_height) * 255;
    color sky_color = color(0, 0, sky_shade);
    float ground_shade = (abs(y - screen_middle_y) / screen_height) * 255;
    color ground_color = color(0, ground_shade, 0);
    if (y < screen_middle_y - half_wall_height) // sky
      put_pixel(x, y, sky_color);
    else if (y > screen_middle_y + half_wall_height)
      put_pixel(x, y, ground_color);
    else
      put_pixel(x, y, wall_color);
  }
}