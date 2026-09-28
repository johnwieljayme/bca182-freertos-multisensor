#ifndef MOTION_H
#define MOTION_H

bool pir_motion_detected(bool outputHigh);
void motion_init(void);
void MotionTask(void *pvParameters);

#endif
