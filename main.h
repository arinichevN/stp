
#ifndef STPw_MAIN_H
#define STPw_MAIN_H

#include "lib/dbl.h"
#include "lib/util.h"
#include "lib/crc.h"
#include "lib/app.h"
#include "lib/configl.h"
#include "lib/timef.h"
#include "lib/udp.h"
#include "lib/tsv.h"
#include "lib/green_light.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/acp/regulator.h"
#include "lib/acp/stepped_setter.h"
#include "lib/acp/channel.h"

#define APP_NAME stp
#define APP_NAME_STR TOSTRING(APP_NAME)

#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./"
#endif
#define CONFIG_FILE "" CONF_DIR "config.tsv"

#define WAIT_RESP_TIMEOUT 3

#define MODE_SIZE 3

#define FLOAT_NUM "%.2f"

#define MIN_ALLOC_STEP_LIST 8

#define SECURE_TRY_NUM 3
#define SENSOR_RETRY_NUM 3

enum {
    ON = 1,
    OFF,
    DO,
    INIT,
    RUN,
    REACH,
    HOLD,
    DISABLE,
    DONE,
    CLEAR,
    WAIT,
    CHECK,
    CREP,
    NSTEP,
    FSTEP,
    STOP,
    WAIT_GREEN_LIGHT,
    UNKNOWN,
    FAILURE,
    ENABLED,DISABLED,
    SLAVE_ENABLE,
    SET_GOAL,
    GET_VALUE
} States;

typedef struct {
    RChannel remote_channel;
    FTS input;
    int retry_num;
} Sensor;

typedef struct {
    RChannel remote_channel;
    int retry_num;
} Slave;

typedef struct {
    int id;
    double goal;
    struct timespec reach_duration;
    struct timespec hold_duration;
    int next_step_id;

    int state;
    Ton tmr;
    double goal_correction;
    double goal_out;
    double value_start;
} Step;

DEC_LIST ( Step )

typedef struct {
    int first_step_id;
    StepList step_list;
    Slave slave;
    Sensor sensor;
    Step *step;
    GreenLight green_light;
    int state;
} Prog;

struct channel_st {
    int id;
    Prog prog;
    int save;
    uint32_t error_code;
    int sock_fd;
    Mutex mutex;
    char *db_schema;
    pthread_t thread;
    struct timespec cycle_duration;
    struct channel_st *next;
};

typedef struct channel_st Channel;

DEC_LLIST ( Channel )

extern char *db_path;
extern char *peer_id;
extern int app_state;
extern int sock_port;
extern Peer peer_client ;
extern ChannelLList channel_list;

extern int readSettings() ;

extern int initApp() ;

extern int initData() ;

extern void serverRun ( int *state, int init_state ) ;

extern void *threadFunction ( void *arg ) ;

extern void freeData() ;

extern void freeApp() ;

extern void exit_nicely() ;

#endif

