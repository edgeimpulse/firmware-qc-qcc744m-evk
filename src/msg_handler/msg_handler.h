#ifndef _MSG_HANDLER_H_
#define _MSG_HANDLER_H_

/*
 * simpler at handler for the QCC744x firmware.
 We need:
 - start impulse
 - start impulse cont
 - stop impulse
 - set ingestion settings: sensor, sample len, sample interval
 - start ingestion
*/

#define MSG_AT_START            "AT+"
// AT RUN IMPULSE MSG
#define MSG_START_IMPULSE       "RUNIMPULSE"
#define MSG_START_IMPULSE_CONT  "RUNIMPULSECONT"
#define MSG_START_IMPULSE_DEBUG "RUNIMPULSEDEBUG"

#define MSG_STOP_IMPULSE        "b"


// AT SET INGESTION SETTINGS MSG

#ifdef __cplusplus
extern "C" {
#endif

extern void msg_handler_init(void);

#ifdef __cplusplus
}
#endif

#endif