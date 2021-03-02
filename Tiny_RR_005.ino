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
//#define NUM_POSSIBLE_ANGLES   128 //512 // for trig lookup tables

// *** global variables ***

// screen settings
int screen_width = SCREEN_WIDTH,
    screen_height = SCREEN_HEIGHT,
    num_pixels = screen_width * screen_height;

// constants
const int NUM_SLIVERS = screen_width; //360;
const float FOV = 90.0; // field of view (degrees)
const float theta_inc = FOV / NUM_SLIVERS; // degrees to increment theta by (0.703125)
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
      player_turn_speed = 16.0 * theta_inc, //11.25, // degrees per turn step
      player_walk_speed = 0.1; //0.1; // map units per walk step

// drawing
char stroke_color = 1;
char fill_color = 1;

// wall textures
const char WALL_TEXTURE_X_DIM = 3,
           WALL_TEXTURE_Y_DIM = 3,
           NUM_WALL_TEXTURES = 20;
unsigned char wall_textures[NUM_WALL_TEXTURES][WALL_TEXTURE_X_DIM][WALL_TEXTURE_Y_DIM];

// control
// RetroRocket pins
int RR_PIN_A = 4,
    RR_PIN_B = 5,
    RR_PIN_UP = 6,
    RR_PIN_DOWN = 7,
    RR_PIN_LEFT = 8,
    RR_PIN_RIGHT = 9,
    RR_PIN_SOUND = 10;
// Tiny ray caster pins
int pin_left = RR_PIN_A,
    pin_right = RR_PIN_B,
    pin_forward = RR_PIN_UP,
    pin_backward = RR_PIN_DOWN,
    pin_strafe_left = RR_PIN_LEFT,
    pin_strafe_right = RR_PIN_RIGHT;

unsigned long int frame_number = 0;
unsigned long int millis_since_start = 0;

//float sin_lookup[NUM_POSSIBLE_ANGLES];
//const float sin_lookup[] = { 0,0.01227153828571993,0.02454122852291229,0.03680722294135883,0.04906767432741801,0.06132073630220858,0.07356456359966743,0.08579731234443989,0.0980171403295606,0.110222207293883,0.1224106751992162,0.1345807085071262,0.1467304744553617,0.1588581433338614,0.1709618887603012,0.183039887955141,0.1950903220161282,0.2071113761922185,0.2191012401568698,0.2310581082806711,0.2429801799032639,0.2548656596045145,0.2667127574748984,0.2785196893850531,0.2902846772544623,0.3020059493192281,0.3136817403988915,0.3253102921622629,0.3368898533922201,0.3484186802494346,0.3598950365349882,0.3713171939518375,0.3826834323650898,0.3939920400610481,0.4052413140049898,0.4164295600976372,0.427555093430282,0.4386162385385277,0.4496113296546066,0.46053871095824,0.4713967368259976,0.4821837720791228,0.4928981922297839,0.5035383837257176,0.5141027441932218,0.5245896826784689,0.5349976198870972,0.5453249884220465,0.5555702330196022,0.5657318107836131,0.5758081914178453,0.5857978574564389,0.5956993044924334,0.6055110414043255,0.6152315905806268,0.6248594881423863,0.6343932841636455,0.6438315428897914,0.6531728429537768,0.6624157775901718,0.6715589548470184,0.680600997795453,0.6895405447370668,0.6983762494089729,0.7071067811865475,0.7157308252838186,0.7242470829514669,0.7326542716724127,0.740951125354959,0.7491363945234593,0.7572088465064845,0.7651672656224588,0.7730104533627369,0.7807372285720944,0.7883464276266062,0.7958369046088835,0.8032075314806449,0.8104571982525948,0.8175848131515837,0.8245893027850252,0.8314696123025451,0.838224705554838,0.8448535652497071,0.8513551931052652,0.8577286100002719,0.8639728561215867,0.8700869911087113,0.8760700941954065,0.881921264348355,0.8876396204028539,0.8932243011955153,0.8986744656939538,0.9039892931234433,0.9091679830905224,0.9142097557035307,0.9191138516900577,0.9238795325112867,0.9285060804732155,0.9329927988347388,0.937339011912575,0.9415440651830208,0.9456073253805213,0.9495281805930367,0.9533060403541938,0.9569403357322089,0.9604305194155658,0.9637760657954398,0.9669764710448521,0.970031253194544,0.9729399522055601,0.9757021300385286,0.9783173707196277,0.9807852804032304,0.9831054874312163,0.9852776423889412,0.9873014181578584,0.989176509964781,0.99090263542778,0.99247953459871,0.9939069700023561,0.9951847266721969,0.996312612182778,0.9972904566786902,0.9981181129001492,0.9987954562051724,0.9993223845883495,0.9996988186962042,0.9999247018391445,1,0.9999247018391445,0.9996988186962042,0.9993223845883495,0.9987954562051724,0.9981181129001492,0.9972904566786902,0.996312612182778,0.9951847266721969,0.9939069700023561,0.99247953459871,0.9909026354277801,0.989176509964781,0.9873014181578584,0.9852776423889412,0.9831054874312163,0.9807852804032305,0.9783173707196277,0.9757021300385286,0.9729399522055602,0.970031253194544,0.9669764710448521,0.9637760657954398,0.9604305194155659,0.9569403357322088,0.9533060403541939,0.9495281805930367,0.9456073253805214,0.9415440651830208,0.9373390119125748,0.932992798834739,0.9285060804732156,0.9238795325112867,0.9191138516900577,0.9142097557035307,0.9091679830905225,0.9039892931234432,0.8986744656939539,0.8932243011955152,0.8876396204028539,0.8819212643483552,0.8760700941954066,0.8700869911087115,0.8639728561215868,0.8577286100002721,0.8513551931052652,0.8448535652497072,0.8382247055548382,0.8314696123025451,0.8245893027850253,0.8175848131515837,0.8104571982525948,0.8032075314806449,0.7958369046088836,0.7883464276266063,0.7807372285720946,0.7730104533627371,0.7651672656224591,0.7572088465064845,0.7491363945234596,0.740951125354959,0.7326542716724128,0.7242470829514673,0.7157308252838187,0.7071067811865476,0.6983762494089729,0.6895405447370671,0.6806009977954529,0.6715589548470186,0.662415777590172,0.6531728429537766,0.6438315428897914,0.6343932841636455,0.6248594881423863,0.6152315905806269,0.6055110414043255,0.5956993044924335,0.585797857456439,0.5758081914178454,0.565731810783613,0.5555702330196022,0.5453249884220467,0.5349976198870972,0.5245896826784689,0.5141027441932218,0.5035383837257176,0.4928981922297841,0.4821837720791229,0.4713967368259978,0.4605387109582398,0.4496113296546069,0.4386162385385279,0.427555093430282,0.4164295600976372,0.4052413140049899,0.3939920400610482,0.3826834323650898,0.3713171939518377,0.3598950365349883,0.3484186802494348,0.3368898533922203,0.3253102921622628,0.3136817403988914,0.3020059493192284,0.2902846772544623,0.2785196893850531,0.2667127574748989,0.2548656596045147,0.242980179903264,0.2310581082806713,0.21910124015687,0.2071113761922184,0.1950903220161282,0.1830398879551413,0.1709618887603012,0.1588581433338614,0.1467304744553622,0.1345807085071263,0.1224106751992163,0.1102222072938828,0.09801714032956084,0.08579731234444016,0.0735645635996673,0.06132073630220893,0.04906767432741797,0.03680722294135883,0.02454122852291277,0.01227153828572001,1.224646799147353e-16,-0.0122715382857202,-0.02454122852291208,-0.03680722294135858,-0.04906767432741817,-0.06132073630220825,-0.07356456359966705,-0.08579731234443992,-0.09801714032956059,-0.110222207293883,-0.1224106751992161,-0.1345807085071265,-0.1467304744553616,-0.1588581433338612,-0.1709618887603014,-0.1830398879551407,-0.1950903220161279,-0.2071113761922186,-0.2191012401568698,-0.2310581082806707,-0.2429801799032638,-0.2548656596045145,-0.2667127574748978,-0.2785196893850529,-0.2902846772544622,-0.3020059493192283,-0.3136817403988912,-0.325310292162263,-0.3368898533922201,-0.3484186802494342,-0.3598950365349881,-0.3713171939518375,-0.3826834323650892,-0.3939920400610479,-0.4052413140049897,-0.4164295600976374,-0.4275550934302818,-0.4386162385385273,-0.4496113296546067,-0.4605387109582397,-0.4713967368259976,-0.4821837720791227,-0.4928981922297843,-0.5035383837257175,-0.5141027441932216,-0.5245896826784691,-0.5349976198870969,-0.5453249884220461,-0.5555702330196023,-0.5657318107836129,-0.5758081914178449,-0.5857978574564389,-0.5956993044924332,-0.6055110414043254,-0.6152315905806267,-0.6248594881423866,-0.6343932841636453,-0.6438315428897913,-0.6531728429537769,-0.6624157775901718,-0.6715589548470181,-0.680600997795453,-0.6895405447370668,-0.6983762494089725,-0.7071067811865475,-0.7157308252838185,-0.7242470829514671,-0.7326542716724126,-0.7409511253549588,-0.7491363945234594,-0.7572088465064842,-0.7651672656224588,-0.7730104533627371,-0.7807372285720944,-0.7883464276266059,-0.7958369046088833,-0.8032075314806451,-0.8104571982525947,-0.8175848131515837,-0.8245893027850251,-0.8314696123025447,-0.8382247055548377,-0.8448535652497071,-0.8513551931052653,-0.857728610000272,-0.8639728561215869,-0.8700869911087113,-0.8760700941954063,-0.8819212643483549,-0.887639620402854,-0.8932243011955152,-0.8986744656939538,-0.9039892931234431,-0.9091679830905224,-0.9142097557035305,-0.9191138516900577,-0.9238795325112868,-0.9285060804732155,-0.932992798834739,-0.9373390119125748,-0.9415440651830208,-0.9456073253805212,-0.9495281805930367,-0.953306040354194,-0.9569403357322088,-0.9604305194155657,-0.9637760657954398,-0.966976471044852,-0.970031253194544,-0.9729399522055602,-0.9757021300385285,-0.9783173707196274,-0.9807852804032303,-0.9831054874312163,-0.9852776423889411,-0.9873014181578583,-0.989176509964781,-0.99090263542778,-0.99247953459871,-0.9939069700023561,-0.9951847266721969,-0.996312612182778,-0.9972904566786902,-0.9981181129001492,-0.9987954562051723,-0.9993223845883494,-0.9996988186962042,-0.9999247018391445,-1,-0.9999247018391445,-0.9996988186962042,-0.9993223845883495,-0.9987954562051724,-0.9981181129001492,-0.9972904566786902,-0.996312612182778,-0.9951847266721969,-0.9939069700023561,-0.9924795345987101,-0.99090263542778,-0.9891765099647809,-0.9873014181578584,-0.9852776423889412,-0.9831054874312164,-0.9807852804032304,-0.9783173707196278,-0.9757021300385286,-0.9729399522055601,-0.970031253194544,-0.9669764710448523,-0.96377606579544,-0.9604305194155658,-0.9569403357322089,-0.9533060403541938,-0.9495281805930368,-0.9456073253805216,-0.9415440651830209,-0.937339011912575,-0.9329927988347387,-0.9285060804732156,-0.9238795325112866,-0.9191138516900579,-0.9142097557035309,-0.9091679830905225,-0.9039892931234433,-0.898674465693954,-0.8932243011955153,-0.8876396204028542,-0.881921264348355,-0.8760700941954069,-0.8700869911087115,-0.8639728561215867,-0.8577286100002722,-0.8513551931052651,-0.8448535652497073,-0.838224705554838,-0.8314696123025456,-0.8245893027850253,-0.8175848131515835,-0.8104571982525949,-0.8032075314806453,-0.7958369046088837,-0.7883464276266067,-0.7807372285720947,-0.7730104533627369,-0.7651672656224592,-0.7572088465064846,-0.7491363945234597,-0.7409511253549592,-0.7326542716724131,-0.7242470829514671,-0.7157308252838185,-0.7071067811865477,-0.6983762494089734,-0.6895405447370672,-0.680600997795453,-0.6715589548470187,-0.6624157775901718,-0.6531728429537771,-0.6438315428897922,-0.6343932841636459,-0.6248594881423865,-0.6152315905806266,-0.6055110414043257,-0.5956993044924332,-0.5857978574564391,-0.5758081914178459,-0.5657318107836136,-0.5555702330196022,-0.545324988422046,-0.5349976198870973,-0.5245896826784694,-0.5141027441932227,-0.5035383837257181,-0.4928981922297843,-0.4821837720791226,-0.4713967368259979,-0.4605387109582399,-0.449611329654607,-0.4386162385385284,-0.4275550934302825,-0.4164295600976373,-0.4052413140049896,-0.3939920400610483,-0.3826834323650896,-0.3713171939518386,-0.3598950365349888,-0.3484186802494349,-0.33688985339222,-0.3253102921622633,-0.3136817403988915,-0.3020059493192277,-0.2902846772544633,-0.2785196893850537,-0.2667127574748986,-0.2548656596045143,-0.2429801799032642,-0.231058108280671,-0.2191012401568693,-0.2071113761922194,-0.1950903220161287,-0.183039887955141,-0.1709618887603018,-0.1588581433338616,-0.1467304744553615,-0.1345807085071273,-0.1224106751992169,-0.1102222072938834,-0.09801714032956052,-0.08579731234444028,-0.07356456359966743,-0.06132073630220817,-0.04906767432741897,-0.03680722294135939,-0.02454122852291245,-0.01227153828572057 }; 

// TINY STUFF

void generate_wall_textures()
{
  for (int i = 0; i < NUM_WALL_TEXTURES; i++)
  {
    for (int y = 0; y < WALL_TEXTURE_Y_DIM; y++)
    {
      for (int x = 0; x < WALL_TEXTURE_X_DIM; x++)
      {
        unsigned char c;
        if (random(0, 100) < (i * 5))
        {
          c = 1;
        }
        else
        {
          c = 0;
        }
        wall_textures[i][x][y] = c;
      }
    }
  }
}

/*
void compute_lookup_tables()
{
  for (int i = 0; i < NUM_POSSIBLE_ANGLES; i++)
  {
    sin_lookup[i] = sin(i * theta_inc);
  }
}

float LUT_sin(float theta)
{
  float result;
  
  // adjust to range
  while (theta < 0)
  {
    theta += 360;
  }
  while (theta >= 360)
  {
    theta -= 360;
  }
  // theta is now in the range [0,360)


  result = sin(theta * PI / 180);
  
//  if (theta < 90) // UPPER RIGHT QUADRANT
//  {
//    int i = (int)(theta / theta_inc); // access LUT normally
//    result = sin_lookup[i];
//  }
//  else
//    result = sin(theta * PI / 180);
    
//  else if (theta > 90 && theta <= 180) // UPPER LEFT QUADRANT
//  {
//    int i = (int)((180.0-theta) / theta_inc); // flip theta about y axis
//    result = sin_lookup[i];
//  }
//  else if (theta > 180 && theta <= 270) // LOWER LEFT QUADRANT
//  {
//    int i = (int)((180-theta) / theta_inc); // flip theta about y axis
//    result = -sin_lookup[i]; // flip sign of result
//  }
//  else
//    result = sin(theta * PI / 180);

  return result;
}
*/

void display_frame_number()
{
  display.setTextSize(1);      // Normal 1:1 pixel scale
  display.setTextColor(SSD1306_WHITE); // Draw white text
  display.setCursor(30, 0);     // Start at top-left corner
  display.cp437(true);         // Use full 256 char 'Code Page 437' font
  //display.write(i);
  
//  display.print("fr:");
//  display.print(frame_number);
//  Serial.print("fr:");
//  Serial.print(frame_number);
//  
//  millis_since_start = millis();
//  display.print(" ");
//  display.print("ms:");
//  display.print(millis_since_start);
//  Serial.print(" ");
//  Serial.print("ms:");
//  Serial.println(millis_since_start);

  float fps = (float)(frame_number+1.0) / (millis() / 1000.0);
  display.print("fps:");
  display.print(fps);
  Serial.print("fps:");
  Serial.println(fps);
}

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
    tone(RR_PIN_SOUND, 2000, 500); //pin, freq, duration
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
      float k = 1; //0.5;
      float r = (k * screen_height) / wall_height;
      float theta_radians = theta * PI / 180.0; // convert
      float x = player_x + r * cos(theta_radians);
      float y = player_y + r * sin(theta_radians);
      ////float y = player_y + r * LUT_sin(theta);
      // ensure we're within bounds before trying to access map
      if (x < 0 || y < 0 || x >= MAP_X_DIM || y >= MAP_Y_DIM
          || my_map[(int)x][(int)y] != 0)
      {
        d[sliver] = r;
        //float wall_height = 0.5 * screen_height / r;
        //if (wall_height > screen_height) wall_height = screen_height;
        //int wall_code;
        //if (x < 0 || y < 0 || x >= MAP_X_DIM || y >= MAP_Y_DIM)
        //  wall_code = 4; // out of bounds
        //else wall_code = (int)my_map[(int)x][(int)y];
        float wall_color = 1.0; //wall_color_map[wall_code];
        if (r > 1)
          wall_color = 1.0 / r;
        if ((int)(x * 5) % MAP_X_DIM == 0 &&
            (int)(y * 5) % MAP_Y_DIM == 0)
          wall_color = 1.0;
        randomSeed((int)((y * MAP_X_DIM + x)*100));
        render_sliver(sliver, wall_height, wall_color);
        break;
      }
      //r_inc *= 1.007;
    }
    theta += theta_inc;
  }
}


void render_sliver(int x, float wall_height, float wall_color)
{
  float screen_middle_y = screen_height / 2.0;
  float half_wall_height = wall_height / 2.0;
  float sliver_y1 = screen_middle_y - half_wall_height,
        sliver_y2 = screen_middle_y + half_wall_height;
  float wall_shade = (wall_height / screen_height);
  //if (wall_shade > 1) wall_shade = 1;
  //wall_color = 1; //wall_shade;
  put_pixel(x, screen_middle_y - half_wall_height, 1); // top of wall
  put_pixel(x, screen_middle_y + half_wall_height, 1); // bottom of wall
  
  //stroke_color = 1; //wall_color;
  //pete_line(x, screen_middle_y - half_wall_height, x, screen_middle_y + half_wall_height);

  for (int y = screen_middle_y - half_wall_height + 1; y <= screen_middle_y + half_wall_height - 1; y++)
  {
    //if (random(0,101) < (wall_color * 100.0))
    //  put_pixel(x, y, 1); // wall
    unsigned char c;
    int wi = (int)((wall_height / screen_height) * NUM_WALL_TEXTURES);
    if (wi > (NUM_WALL_TEXTURES-1))
      wi = NUM_WALL_TEXTURES-1;
    int wx = x % WALL_TEXTURE_X_DIM;
    int wy = y % WALL_TEXTURE_Y_DIM;
    c = wall_textures[wi][wx][wy];
    put_pixel(x, y, c);
  }

//  for (int y = 0; y < screen_height; y++)
//  {
//    float sky_shade = (abs(y - screen_middle_y) / screen_height) * 255;
//    int sky_color = sky_shade; //color(0, 0, sky_shade);
//    float ground_shade = (abs(y - screen_middle_y) / screen_height) * 255;
//    int ground_color = ground_shade; //color(0, ground_shade, 0);
//    if (y < screen_middle_y - half_wall_height) // sky
//      put_pixel(x, y, sky_color);
//    else if (y > screen_middle_y + half_wall_height) // ground
//      put_pixel(x, y, ground_color);
//    else
//      put_pixel(x, y, wall_color); // wall
//  }

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
  frame_number = 0;
  
  clear_map();
  randomize_map(17);
  print_map();

  // pin modes
  /*
  pinMode(pin_left, INPUT_PULLUP);
  pinMode(pin_right, INPUT_PULLUP);
  pinMode(pin_forward, INPUT_PULLUP);
  pinMode(pin_backward, INPUT_PULLUP);
  */
  
  // input pins
  pinMode(RR_PIN_A, INPUT_PULLUP);
  pinMode(RR_PIN_B, INPUT_PULLUP);
  pinMode(RR_PIN_UP, INPUT_PULLUP);
  pinMode(RR_PIN_DOWN, INPUT_PULLUP);
  pinMode(RR_PIN_LEFT, INPUT_PULLUP);
  pinMode(RR_PIN_RIGHT, INPUT_PULLUP);
  // output pins
  pinMode(RR_PIN_SOUND, OUTPUT);

  //compute_lookup_tables();
  generate_wall_textures();
  
}




void loop()
{
  display.clearDisplay();

  // TINY STUFF
  ray_cast();
  draw_map();
  //draw_map_small();
  display_frame_number();
  
  display.display();

  // momentum-based motion
//  player_x += player_xv;
//  player_y += player_yv;
//  player_xv *= 0.9; // player velocity decay
//  player_yv *= 0.9;

  // button control
  if (!digitalRead(pin_forward))
    step_forward(player_walk_speed);
  if (!digitalRead(pin_backward))
    step_backward(player_walk_speed);
  if (!digitalRead(pin_left))
    turn_left(player_turn_speed);
  if (!digitalRead(pin_right))
    turn_right(player_turn_speed);
  if (!digitalRead(pin_strafe_left))
    strafe_left(player_walk_speed);
  if (!digitalRead(pin_strafe_right))
    strafe_right(player_walk_speed);
    
  //move_randomly();

  
  //step_forward(1);
  //turn_left(player_turn_speed);
  frame_number++;

}
