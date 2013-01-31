#include "SDL_config.h"

/*  Fake joystick for QNX/BB10 devices. It's mapped into SDL like so:
 *
 *  joy0 8 buttons  ( a,b,x,y,select,start,left-trig,right-trig )
 *  joy0 2 axis     ( 4 way directional pad )
 *
 *  Can be invoked from key event input and map SYM scan codes into joystick events.
 *  Can be triggered directly for hooking into generic input processing
 *
 *  Internal State:
 *
 *  1 bit per button/action in lower 16 bits of unsigned int 32 bit word.
 *  bits 0-3 represent up,down,left,right
 *  bits 4-12 represent a,b,x,y,select,start,left-trig,right-trig
 *
*/


#include "SDL_error.h"
#include "SDL_events.h"
#include "SDL_joystick.h"
#include "../SDL_sysjoystick.h"
#include "../SDL_joystick_c.h"

#define BITSET(v,x)  (v |= 1 << x)
#define BITCHK(v,x)  (v & (1 << x))
#define BITCLR(v,x)  (v &= ~(1 << x))

#define NUM_JOYSTICKS 1
#define NUM_BUTTONS   8
#define NUM_AXI       2

// bit positions for each key/button ...
// So currently only the lower 13 bits occupied, we can
// pack in joy1 into the upper 16 bits.

#define JOY_UP          1
#define JOY_DOWN        2
#define JOY_LEFT        3
#define JOY_RIGHT       4

#define JOY_a           0
#define JOY_b           1
#define JOY_x           2
#define JOY_y           3
#define JOY_select      4
#define JOY_start       5
#define JOY_trig_left   6
#define JOY_trig_right  7

#define JOY_MAX_ACTIONS 12
#define JOY_BUTTON_START 0
#define JOY_BUTTON_MAX   8

static int joy_initialized_s = 0;
static unsigned int j0PosMask = 0;  /*  DPAD up,down,left,right for joy 0 */
static unsigned int j0ButtonMask = 0; /* A,B,X,Y, SELECT,START,RIGHT-TRIG,LEFT-TRIG */

/* See SDL_playbookevents.c  we take the keysym codes and map them to the joypad ...*/
enum {
    joy_sym_code_up=72,
    joy_sym_code_down=75,
    joy_sym_code_right=77,
    joy_sym_code_left=80,
    joy_sym_code_a = 65,
    joy_sym_code_b = 66,
    joy_sym_code_x = 67,
    joy_sym_code_y = 68,
    joy_sym_code_select=69,
    joy_sym_code_start=70,
    joy_sym_code_trig_left=71,
    joy_sym_code_trig_right=73,

    joy_sym_code_min=65,
    joy_sym_code_max=80
};


/*
 *  display 32bits of uint as binary pattern.
 */
static void dbg_show_bits( unsigned int w)
{
   int i = 0;
   printf("0x%x\t",w);

   if( w == 0)
	   return;

   for(i =0; i < 32; i++)
   {
	   if( BITCHK(w,i) )
		   printf("1");
	   else
		   printf("0");
   }
   printf("\n");
}


int SDL_SYS_QNX_JoyIsInitialized()
{
	return joy_initialized_s;
}

/* Function to scan the system for joysticks.
 * This function should set SDL_numjoysticks to the number of available
 * joysticks.  Joystick 0 should be the system default joystick.
 * It should return 0, or -1 on an unrecoverable fatal error.
 */
int SDL_SYS_JoystickInit(void)
{
	SDL_numjoysticks =  1;
	return(1);
	/* note that until joystick is 'opened' no events will be processed */

}

/* Function to get the device-dependent name of a joystick */
const char *SDL_SYS_JoystickName(int index)
{
	switch(index)
	{
	   case 0: return "joy0";
	           break;

	/* case 1: return "joy1";
	 *         break;
	 */
	}

	SDL_SetError("only joystick0 is supported at present.");
	return (NULL);
}

/* Function to open a joystick for use.
   The joystick to open is specified by the index field of the joystick.
   This should fill the nbuttons and naxes fields of the joystick structure.
   It returns 0, or -1 if there is an error.
 */
int SDL_SYS_JoystickOpen(SDL_Joystick *joystick)
{

	joystick->nbuttons= NUM_BUTTONS;
	joystick->nhats=0;
	joystick->nballs=0;
	joystick->naxes= NUM_AXI;
	fprintf(stderr,"SDL_SYS_JoystickOpen: initialized\n");

	joy_initialized_s = 1;
	return 0;
}






/*
 *  debug routine to display joypad button pressed.
 */
char *getButtonStr(int b)
{
	switch(b) {
	   case JOY_x: return "button-x";
	   case JOY_y: return "button-y";
	   case JOY_a: return "button-a";
	   case JOY_b: return "button-b";
	   case JOY_select: return "select";
	   case JOY_start: return "start";
	   case JOY_trig_left: return "left-trigger";
	   case JOY_trig_right: return "right-trigger";
	   default : return "-";
    }
}

void dbg_joy_show(unsigned int bmask, int b)
{
  if( BITCHK( bmask, b))
	  fprintf(stderr,"  [%02d] %s\n", b,getButtonStr(b) );
}

void dbg_joy_show_all()
{
  int i = 0;

  for(i=0;i < 4;i++)
	  dbg_joy_show(j0PosMask,i);

  for(i=0;i < NUM_BUTTONS;i++)
	  dbg_joy_show(j0ButtonMask,i);

}

/* Future: This is for setting joystick state from screen event 14
 * to map Wiimote dpad+buttons to the joystick here
 * bit 0 up
 * bit 1 down
 * bit 2 left
 * bit 3 right
 * bit 4-12  buttons 0-8
 * Upper 16bits reserved for 2nd joypad or other stuff.
 */
void SDL_SYS_QnxSetJoyMask(int joy, unsigned int pos_mask, unsigned int button_mask)
{
  switch(joy)
  {
      case 0:  j0PosMask    = pos_mask;
               j0ButtonMask = button_mask;
               break;
   /* case 1: j1buttonmask = mask */
  }
}



/*
 *  This function is hijacking the keyboard input processing and generating
 *  joystick data.  This allows TCO sdl-controls.xml to define a 'joypad' layout
 *  that is mapped to a virtual joystick.

 The joystick is a bit like a SNES pad , at least the idea is
 to represent something like the following, an 8 button 4 way joypad.

   * L1 ************** L2 *
  *     UP                  *
  * LEFT RIGHT        a b   *
  *    DOWN            x y  *
   *************************
*/
void SDL_SYS_QnxDpadEvent(Uint8 state, SDL_keysym *keysym)
{

   int i = 0;

   if(!joy_initialized_s)
	    return;

   if(  (keysym->scancode < joy_sym_code_min) ||
		(keysym->scancode > joy_sym_code_max) )
   {
	       return;
   }


   if( state == SDL_PRESSED )
   {
	switch((int) keysym->scancode)
	{
	     // Numbers here SDLK_xxxx are expected to be defined in the TCO sdl-controls.xml
   	       case joy_sym_code_up: BITSET(j0PosMask, JOY_UP   );  break;
   	       case joy_sym_code_down: BITSET(j0PosMask, JOY_DOWN );  break;
   	       case joy_sym_code_right: BITSET(j0PosMask, JOY_RIGHT);  break;
   	       case joy_sym_code_left: BITSET(j0PosMask, JOY_LEFT );  break;
   	       case joy_sym_code_a: BITSET(j0ButtonMask, JOY_a );  break;
   	       case joy_sym_code_b: BITSET(j0ButtonMask, JOY_b );    break;
   	       case joy_sym_code_x: BITSET(j0ButtonMask, JOY_x );      break;
   	       case joy_sym_code_y: BITSET(j0ButtonMask, JOY_y );       break;
   	       case joy_sym_code_select: BITSET(j0ButtonMask, JOY_select);    break;
   	       case joy_sym_code_start: BITSET(j0ButtonMask, JOY_start);      break;
   	       case joy_sym_code_trig_left: BITSET(j0ButtonMask, JOY_trig_left);   break;
   	       case joy_sym_code_trig_right: BITSET(j0ButtonMask, JOY_trig_right);  break;

   	       default: break; // fprintf(stderr,"key %d\n", keysym->scancode );
   	                 break;
	}
   }
    else /* SDL_RELEASE */
   {
	switch((int) keysym->scancode)
    {
	   	   case 72: BITCLR(j0PosMask, JOY_UP); break;
	   	   case 75: BITCLR(j0PosMask, JOY_DOWN); break;
	   	   case 77: BITCLR(j0PosMask, JOY_RIGHT); break;
	   	   case 80: BITCLR(j0PosMask, JOY_LEFT); break;
   	       case 65: BITCLR(j0ButtonMask, JOY_a); break;
   	       case 66: BITCLR(j0ButtonMask, JOY_b); break;
  	       case 67: BITCLR(j0ButtonMask, JOY_x); break;
   	       case 68: BITCLR(j0ButtonMask, JOY_y); break;
   	       case 69: BITCLR(j0ButtonMask, JOY_select); break;
   	       case 70: BITCLR(j0ButtonMask, JOY_start);     break;
   	       case 71: BITCLR(j0ButtonMask, JOY_trig_left); break;
   	       case 73: BITCLR(j0ButtonMask, JOY_trig_right); break;

   	       default:
   	                break;
	}
   }

   // dbg_joy_show_all();
}


/* Function to update the state of a joystick - called as a device poll.
 * This function shouldn't update the joystick structure directly,
 * but instead should call SDL_PrivateJoystick*() to deliver events
 * and update joystick device state.
 *
 *  jState stores the last event, we compare this with the latest event from j0buttonmask and only update SDL activity
 *  on a change of state.
 *
 *  If j0PosMask is TRUE and jState FALSE then set jState TRUE and trigger JoyAxis or JoyButton  = Set the new event in SDL
 *  If j0PosMask is TRUE and jState TRUE do nothing.  = We already processed the event in the first step, do nothing.
 *  If j0PosMask is FALSE and jState is TRUE then clear the event, clear the jState
 *
 *  ax=0  is X left/right  We can set any integer from -327658 thru to +327658 on an axis to represent a grid point.
 *  ax=1  is Y up/down     Since this is a 'dpad' we have 4 quadrants to set/clear
 *
 *  AXIS VALUE  STATE
 *  ---------------------
 *  1    -1    y up     ON
 *  1     1    y down   ON
 *  1     0    y center OFF
 *  0    -1    x left   ON
 *  0     1    x right  ON
 *  0     0    x center OFF
 */
void SDL_SYS_JoystickUpdate(SDL_Joystick *joystick)
{
    int i = 0;
	short ax=0,x=0,y=0;
    const int jRange = 32767;
    static unsigned int jState = 0;
    static unsigned int jButtonState =0;

    /* dpad set */
	if( BITCHK(j0PosMask,JOY_UP)    && !BITCHK(jState, JOY_UP)   ) { ax=1;  y=-jRange; SDL_PrivateJoystickAxis(joystick,ax, y); BITSET(jState,JOY_UP); }
	if( BITCHK(j0PosMask,JOY_DOWN)  && !BITCHK(jState, JOY_DOWN) ) { ax=1;  y=jRange;  SDL_PrivateJoystickAxis(joystick,ax, y); BITSET(jState,JOY_DOWN); }
	if( BITCHK(j0PosMask,JOY_LEFT)  && !BITCHK(jState, JOY_LEFT) ) { ax=0;  x=-jRange; SDL_PrivateJoystickAxis(joystick,ax, x); BITSET(jState,JOY_LEFT); }
	if( BITCHK(j0PosMask,JOY_RIGHT) && !BITCHK(jState, JOY_RIGHT)) { ax=0;  x=jRange;  SDL_PrivateJoystickAxis(joystick,ax, x); BITSET(jState,JOY_RIGHT); }
	/* dpad release  */
	if( !BITCHK(j0PosMask,JOY_UP)   &&  BITCHK(jState, JOY_UP)  )  { ax=1;  y=0;  SDL_PrivateJoystickAxis(joystick,ax, y); BITCLR(jState,JOY_UP);    }
    if( !BITCHK(j0PosMask,JOY_DOWN) &&  BITCHK(jState, JOY_DOWN))  { ax=1;  y=0;  SDL_PrivateJoystickAxis(joystick,ax, y); BITCLR(jState,JOY_DOWN);  }
	if( !BITCHK(j0PosMask,JOY_LEFT) &&  BITCHK(jState, JOY_LEFT))  { ax=0;  x=0;  SDL_PrivateJoystickAxis(joystick,ax, x); BITCLR(jState,JOY_LEFT);  }
	if( !BITCHK(j0PosMask,JOY_RIGHT) && BITCHK(jState, JOY_RIGHT)) { ax=0;  x=0;  SDL_PrivateJoystickAxis(joystick,ax, x); BITCLR(jState,JOY_RIGHT);  }

	/* button press/release  */
	for( i = 0; i < NUM_BUTTONS;i++)
	{
	  if( BITCHK(j0ButtonMask,i) && !BITCHK(jButtonState,i) )
	  {
          SDL_PrivateJoystickButton(joystick, i, SDL_PRESSED );
	      BITSET(jButtonState,i);
	  }

	  if( !BITCHK(j0ButtonMask,i) && BITCHK(jButtonState,i) )
	  {
		  SDL_PrivateJoystickButton(joystick, i, SDL_RELEASED);
	      BITCLR(jButtonState,i);
	  }
	}


}

/* Function to close a joystick after use */
void SDL_SYS_JoystickClose(SDL_Joystick *joystick)
{
  fprintf(stderr,"SDL_SYS_JoystickClose\n");
}

/* Function to perform any system-specific joystick related cleanup */
void SDL_SYS_JoystickQuit(void)
{
}

