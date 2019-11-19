/////////////////////////////////
// File: stest.c
// Desc: Stage library test program
// Created: 2004.9.15
// Author: Richard Vaughan <vaughan@sfu.ca>
// CVS: $Id$
// License: GPL
/////////////////////////////////

#include "stage.h"

const size_t MAX_LASER_SAMPLES = 361;

double minfrontdistance = 0.750;
double speed = 0.200;
double avoidspeed = 0; // -150;
double turnrate = DTOR(40);

int randint;
int randcount = 0;
int avoidcount = 0;
int obs = FALSE;

int main( int argc, char* argv[] )
{ 
  stg_world_t* world;
  char* robotname;
  char sonarname[64];
  char lasername[64];
  stg_model_t* position;
  stg_model_t* laser;
  stg_model_t* sonar;
  double newspeed = 0.0;
  double newturnrate = 0.0;

  printf( "Stage v%s test program.", VERSION );

  if( argc < 3 )
    puts( "Usage: stest [gtk args] worldfile robotname" );

  // initialize libstage
  stg_init( argc, argv );

  world = stg_world_create_from_file(argv[1], 0, 0);
  
  robotname = argv[2];

  // generate the name of the laser and sonar devices attached to the robot
  snprintf( lasername, 63, "%s.laser:0", robotname ); 
  snprintf( sonarname, 63, "%s.ranger:0", robotname ); 
  
  position = stg_world_model_name_lookup( world, robotname );  
  laser = stg_world_model_name_lookup( world, lasername );
  sonar = stg_world_model_name_lookup( world, sonarname );

  // subscribe to the laser - starts it collecting data
  stg_model_subscribe( laser );
  stg_model_subscribe( position);
  stg_model_subscribe( sonar );

  stg_model_print( laser );

  puts( "starting clock" );
  // start the clock
  stg_world_start( world );
  puts( "done" );



  stg_world_set_interval_real( world, 0 );

  while( (stg_world_update( world,0,0 )==0) )
    {
      stg_position_cmd_t cmd;
      stg_velocity_t vel;
      size_t laser_sample_count = 0;
      stg_laser_sample_t* laserdata;
      int i;
      
      stg_velocity_t* pose = stg_model_get_property_fixed( position, "pose", sizeof(pose));

      stg_model_get_velocity( position, &vel );

      //printf( "position velocity: (%.2f,%.2f,%.2f)\n",
      //      vel.x, vel.y, vel.a );          
      
      //printf( "position pose: (%.2f,%.2f,%.2f)\n",
      //      pose->x, pose->y, pose->a );          

      // get some laser data
      laserdata = stg_model_get_property( laser, "laser_data", &laser_sample_count );
      laser_sample_count /= sizeof(stg_laser_sample_t);
      
      //printf( "obtained %d laser samples\n", laser_sample_count );

      //int i;
      //for( i=0; i<laser_sample_count; i++ )
      //printf( "[%d %d]", i, laserdata[i].range );

      // THIS IS ADAPTED FROM PLAYER'S RANDOMWALK C++ EXAMPLE

      /* See if there is an obstacle in front */
      obs = FALSE;
      for(i = 0; i < laser_sample_count; i++)
	{
	  if(laserdata[i].range/1000.0 < minfrontdistance)
	    obs = TRUE;
	}
      
      if(obs || avoidcount )
	{
	  newspeed = avoidspeed; 
	  
	  /* once we start avoiding, continue avoiding for 2 seconds */
	  /* (we run at about 10Hz, so 20 loop iterations is about 2 sec) */
	  if(!avoidcount)
	    {
	      // find the minimum on the left and right
	      
	      double min_left = 1e9;
	      double min_right = 1e9;

	      avoidcount = 15;
	      randcount = 0;
	      
	      for(i=0; i<laser_sample_count; i++ )
		{
		  if(i>(laser_sample_count/2) && laserdata[i].range < min_left)
		    min_left = laserdata[i].range;
		  else if(i<(laser_sample_count/2) && laserdata[i].range < min_right)
		    min_right = laserdata[i].range;
		}

	      if( min_left < min_right)
		newturnrate = -turnrate;
	      else
		newturnrate = turnrate;
	    }
	  
	  avoidcount--;
	}
      else
	{
	  avoidcount = 0;
	  newspeed = speed;
	  
	  /* update turnrate every 3 seconds */
	  if(!randcount)
	    {
	      /* make random int tween -20 and 20 */
	      randint = rand() % 41 - 20;
	      
	      newturnrate = DTOR(randint);
	      randcount = 20;
	    }
	  randcount--;
	}
      
      cmd.x = newspeed;
      cmd.y = 0;
      cmd.a = newturnrate;

      stg_model_set_property( position, "position_cmd", &cmd, sizeof(cmd));

    }
  
  stg_world_destroy( world );
  
  exit( 0 );
}
