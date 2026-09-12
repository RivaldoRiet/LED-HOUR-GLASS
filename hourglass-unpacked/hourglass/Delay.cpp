#include "Delay.h"
void NonBlockDelay::Delay (unsigned long t)
{
  startedAt = millis();
  duration = t;
};
bool NonBlockDelay::Timeout (void)
{
  return (unsigned long)(millis() - startedAt) >= duration;
}
unsigned long NonBlockDelay::Time(void)
 {
   return startedAt + duration;
 }
