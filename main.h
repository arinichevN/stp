
#ifndef STP_MAIN_H
#define STP_MAIN_H


#include "lib/dbl.h"
#include "lib/util.h"
#include "lib/app.h"
#include "lib/configl.h"
#include "lib/timef.h"
#include "lib/acp/main.h"
#include "lib/acp/app.h"
#include "lib/acp/regulator.h"
#include "lib/acp/prog.h"
#include "lib/acp/stp.h"

#define APP_NAME stp
#define APP_NAME_STR TOSTRING(APP_NAME)

#ifdef MODE_FULL
#define CONF_DIR "/etc/controller/" APP_NAME_STR "/"
#endif
#ifndef MODE_FULL
#define CONF_DIR "./"
#endif
#define CONFIG_FILE "" CONF_DIR "config.tsv"

#define PROG_FIELDS "id,peer_id,first_repeat_id,remote_id,\"check\",retry_count,enable,load"

#define WAIT_RESP_TIMEOUT 3
#define DBC 30000
#define MODE_SIZE 3
#define PRECISION 0.01 

#define NANO_FACTOR 0.000000001

#define FLOAT_NUM "%.2f"

#define STOP_KIND_TIME_STR "time"
#define STOP_KIND_GOAL_STR "goal"

#define CHANGE_MODE_EVEN_STR "even"
#define CHANGE_MODE_INSTANT_STR "instant"

#define PROG_LIST_LOOP_DF {Prog *item = prog_list.top;
#define PROG_LIST_LOOP_ST while (item != NULL) {
#define PROG_LIST_LOOP_SP item = item->next; } item = prog_list.top;}

typedef struct {
    Peer *peer;
    int remote_id;
    int check;
    int retry_count;
    int state;
    int crepeat;
} Slave;

typedef struct {
    int id;
    struct timespec duration;
    float goal;
    int goal_change_mode; //even or instant
    int stop_kind; //by time or by goal
    int next_step_id;

    int state;
    Ton_ts tmr;
    int state_ch;
    float goal_correction;
    float value_start;

    char state_sp;
    int wait_above;
} Step;

typedef struct {
    int id;
    int count;
    int first_step_id;
    int next_repeat_id;
    Step c_step;
    int state;
    int c_count;
} Repeat;

struct prog_st {
    int id;
    int first_repeat_id;
    Slave slave;
    Repeat c_repeat;

    int state;
    Mutex mutex;
    struct prog_st *next;
};

typedef struct prog_st Prog;

DEC_LLIST(Prog)

typedef struct {
    sqlite3 *db;
    PeerList *peer_list;
    ProgList *prog_list;
} ProgData;

enum {
    ON = 1,
    OFF,
    DO,
    INIT,
    RUN,
    DISABLE,
    DONE,
    CLEAR,
    WAIT,
    CHECK,
    CREP,
    NSTEP,
    FSTEP,
    BYTIME,
    BYGOAL,
    STOP,
    CHANGE_MODE_EVEN,
    CHANGE_MODE_INSTANT,
    STOP_KIND_TIME,
    STOP_KIND_GOAL,
    UNKNOWN,
    FAILURE,
    ENABLED,DISABLED
} States;



extern int readSettings();

extern void initApp();

extern int initData();

extern void serverRun(int *state, int init_state);

extern void progControl(Prog *item, const char * db_path);

extern void *threadFunction(void *arg);

extern int createThread_ctl();

extern void freeProg(ProgList * list);

extern void freeData();

extern void freeApp();

extern void exit_nicely();

extern void exit_nicely_e(char *s);

#endif 

