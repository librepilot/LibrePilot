/* Dervied from minimosd-extra code at https://github.com/diydrones/MinimOSD-Extra/blob/master/MinimOsd-Extra/OSD_Panels.ino (GPLv3) */


#ifndef _CUSTOM_TYPES_H
#define _CUSTOM_TYPES_H

#define CUSTOM_MODE_STAB     0 /* manual airframe angle with manual throttle */
#define CUSTOM_MODE_ACRO     1 /* manual body-frame angular rate with manual throttle */
#define CUSTOM_MODE_ALTH     2 /* manual airframe angle with automatic throttle */
#define CUSTOM_MODE_AUTO     3 /* fully automatic waypoint control using mission commands */
#define CUSTOM_MODE_GUIDED   4 /* fully automatic fly to coordinate or fly at velocity/direction using GCS immediate commands */
#define CUSTOM_MODE_LOITER   5 /* automatic horizontal acceleration with automatic throttle */
#define CUSTOM_MODE_RTL      6 /* automatic return to launching point */
#define CUSTOM_MODE_CIRCLE   7 /* automatic circular flight with automatic throttle */
#define CUSTOM_MODE_POSH     8 /* Position: auto control; deprecated? */
#define CUSTOM_MODE_LAND     9 /* automatic landing with horizontal position control */
#define CUSTOM_MODE_OFLOITER 10 /* deprecated */
#define CUSTOM_MODE_DRIFT    11 /* semi-automous position, yaw and throttle control */
#define CUSTOM_MODE_SPORT    13 /* manual earth-frame angular rate control with manual throttle */
#define CUSTOM_MODE_FLIP     14 /* automatically flip the vehicle on the roll axis */
#define CUSTOM_MODE_ATUN     15 /* automatically tune the vehicle's roll and pitch gains */
#define CUSTOM_MODE_PHLD     16 /* automatic position hold with manual override, with automatic throttle */
#define CUSTOM_MODE_BRAK     17 /* full-brake using inertial/GPS system, no pilot input */

#endif // ifndef _CUSTOM_TYPES_H
