#include "main.h"

FUN_LLIST_GET_BY_ID(Prog)

int lockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_lock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("lockProgList: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_trylock(&(progl_mutex.self)) != 0) {
        return 0;
    }
    return 1;
}

int unlockProgList() {
    extern Mutex progl_mutex;
    if (pthread_mutex_unlock(&(progl_mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("unlockProgList: error unlocking mutex (CMD_GET_ALL)");
#endif 
        return 0;
    }
    return 1;
}

int lockProg(Prog *item) {
    if (pthread_mutex_lock(&(item->mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("lockProg: error locking mutex");
#endif 
        return 0;
    }
    return 1;
}

int tryLockProg(Prog *item) {
    if (pthread_mutex_trylock(&(item->mutex.self)) != 0) {
        return 0;
    }
    return 1;
}

int unlockProg(Prog *item) {
    if (pthread_mutex_unlock(&(item->mutex.self)) != 0) {
#ifdef MODE_DEBUG
        perror("unlockProg: error unlocking mutex (CMD_GET_ALL)");
#endif 
        return 0;
    }
    return 1;
}

int getProgEnabled(const Prog *item) {
    if (item->state == OFF) {
        return 0;
    }
    return 1;
}

char * getStateStr(char state) {
    switch (state) {
        case ON:
            return "ON";
        case OFF:
            return "OFF";
        case DO:
            return "DO";
        case INIT:
            return "INIT";
        case RUN:
            return "RUN";
        case DISABLE:
            return "DISABLE";
        case DONE:
            return "DONE";
        case CLEAR:
            return "CLEAR";
        case CREP:
            return "CREP";
        case NSTEP:
            return "NSTEP";
        case FSTEP:
            return "FSTEP";
        case BYTIME:
            return "BYTIME";
        case BYGOAL:
            return "BYGOAL";
        case STOP:
            return "STOP";
        case UNKNOWN:
            return "UNKNOWN";
        case FAILURE:
            return "FAILURE";
        case CHANGE_MODE_EVEN:
            return CHANGE_MODE_EVEN_STR;
        case CHANGE_MODE_INSTANT:
            return CHANGE_MODE_INSTANT_STR;
        case STOP_KIND_TIME:
            return STOP_KIND_TIME_STR;
        case STOP_KIND_GOAL:
            return STOP_KIND_GOAL_STR;
    }
    return "?";
}

char * getStopKindStr(char state) {
    switch (state) {

    }
    return "?";
}

int slaveEnable(Slave *slave) {
    extern unsigned int retry_count;
    for (int i = 0; i < retry_count; i++) {
        int id = slave->remote_id;
        I1List data = {.item = &id, .length = 1};
        acp_requestSendUnrequitedI1List(ACP_CMD_PROG_ENABLE, &data, slave->peer);
        int enabled = 0;
        if (acp_sendCmdGetInt(slave->peer, ACP_CMD_PROG_GET_ENABLED, &enabled)) {
            if (enabled) {
                return 1;
            }
        }
    }
    return 0;
}

int slaveDisable(Slave *slave) {
    extern unsigned int retry_count;
    for (int i = 0; i < retry_count; i++) {
        int id = slave->remote_id;
        I1List data = {.item = &id, .length = 1};
        acp_requestSendUnrequitedI1List(ACP_CMD_PROG_DISABLE, &data, slave->peer);
        int enabled = 1;
        if (acp_sendCmdGetInt(slave->peer, ACP_CMD_PROG_GET_ENABLED, &enabled)) {
            if (!enabled) {
                return 1;
            }
        }
    }
    return 0;
}

int slaveSetGoal(Slave *slave, float goal) {
    extern unsigned int retry_count;
    for (int i = 0; i < retry_count; i++) {
        I1F1 i1f1 = {.p0 = slave->remote_id, .p1 = goal};
        I1F1List data = {.item = &i1f1, .length = 1};
        acp_requestSendUnrequitedI1F1List(ACP_CMD_REG_PROG_SET_GOAL, &data, slave->peer);
        float f_goal = 0;
        if (acp_sendCmdGetFloat(slave->peer, ACP_CMD_REG_PROG_GET_GOAL, &f_goal)) {
            if (goal == f_goal) {
                return 1;
            }
        }
    }
    return 0;
}

int slaveGetValue(Slave *slave, float *value) {
    extern unsigned int retry_count;
    for (int i = 0; i < retry_count; i++) {
        FTS fts;
        if (acp_getFTS(&fts, slave->peer, slave->remote_id)) {
            *value = fts.value;
            return 1;
        }
    }
    return 0;
}

void progEnable(Prog *item) {
    if (!getProgEnabled(item)) {
        item->state = INIT;
    }
}

void progDisable(Prog *item) {
    if (getProgEnabled(item)) {
        item->state = DISABLE;
    }
}

void secure() {
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
    curr->state = OFF;
    slaveDisable(&curr->slave);
    PROG_LIST_LOOP_SP
}

int bufCatProgRuntime(const Prog *item, ACPResponse *response) {
    char q[LINE_SIZE];
    char *state_prog = getStateStr(item->state);
    char *state_repeat = getStateStr(item->c_repeat.state);
    char *state_step = getStateStr(item->c_repeat.c_step.state);
    char *state_step_ch = getStateStr(item->c_repeat.c_step.state_ch);
    char *state_step_sp = getStateStr(item->c_repeat.c_step.state_sp);
    struct timespec tm_rest = getTimeRestTmr(item->c_repeat.c_step.duration, item->c_repeat.c_step.tmr);
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%s" FLOAT_NUM ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_ROW_STR,
            item->id,
            state_prog,
            state_repeat,
            item->c_repeat.c_count,
            state_step,

            state_step_ch,
            item->c_repeat.c_step.value_start,
            item->c_repeat.c_step.goal_correction,

            state_step_sp,
            item->c_repeat.c_step.wait_above,
            tm_rest.tv_sec
            );
    return acp_responseStrCat(response, q);
}

int bufCatProgInit(const Prog *item, ACPResponse *response) {
    char q[LINE_SIZE];
    char *step_goal_change_mode = getStateStr(item->c_repeat.c_step.goal_change_mode);
    char *step_stop_kind = getStateStr(item->c_repeat.c_step.stop_kind);
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            item->id,
            item->first_repeat_id,
            item->slave.peer->id,
            item->slave.remote_id,
            item->c_repeat.first_step_id,
            item->c_repeat.count,
            item->c_repeat.next_repeat_id,
            item->c_repeat.c_step.goal,
            item->c_repeat.c_step.duration.tv_sec,
            step_goal_change_mode,
            step_stop_kind,
            item->c_repeat.c_step.next_step_id

            );
    return acp_responseStrCat(response, q);
}

int bufCatProgEnabled(const Prog *item, ACPResponse *response) {
    char q[LINE_SIZE];
    int enabled = getProgEnabled(item);
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            item->id,
            enabled
            );
    return acp_responseStrCat(response, q);
}

void printData(ACPResponse *response) {
    ProgList *list = &prog_list;
    PeerList *pl = &peer_list;
    char q[LINE_SIZE];
    size_t i;
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "retry_count: %d\n", retry_count);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration sec: %ld\n", cycle_duration.tv_sec);
    SEND_STR(q)
    snprintf(q, sizeof q, "cycle_duration nsec: %ld\n", cycle_duration.tv_nsec);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_data_path: %s\n", db_data_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "db_public_path: %s\n", db_public_path);
    SEND_STR(q)
    snprintf(q, sizeof q, "app_state: %s\n", getAppState(app_state));
    SEND_STR(q)
    snprintf(q, sizeof q, "PID: %d\n", proc_id);
    SEND_STR(q)
    snprintf(q, sizeof q, "prog_list length: %d\n", list->length);
    SEND_STR(q)
    SEND_STR("initial data:\n")
    SEND_STR("+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                                                    Program                                                                            |\n")
    SEND_STR("+                                               +-----------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                               |                                                     Repeat                                                            |\n")
    SEND_STR("+                                               |                                               +-----------------------------------------------------------------------+\n")
    SEND_STR("|                                               |                                               |                               Step                                    |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|    id     |fst_rep_id |  peer_id  | remote_id |    id     |fst_stp_id |   count   |next_rep_id|     id    |   goal    | duration  |change_mode| stop_kind |next_stp_id|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
            char *step_goal_change_mode = getStateStr(curr->c_repeat.c_step.goal_change_mode);
    char *step_stop_kind = getStateStr(curr->c_repeat.c_step.stop_kind);
    snprintf(q, sizeof q, "|%11d|%11d|%11s|%11d|%11d|%11d|%11d|%11d|%11d|%11.3f|%11ld|%11s|%11s|%11d|\n",
            curr->id,
            curr->first_repeat_id,
            curr->slave.peer->id,
            curr->slave.remote_id,
            curr->c_repeat.id,
            curr->c_repeat.first_step_id,
            curr->c_repeat.count,
            curr->c_repeat.next_repeat_id,
            curr->c_repeat.c_step.id,
            curr->c_repeat.c_step.goal,
            curr->c_repeat.c_step.duration.tv_sec,
            step_goal_change_mode,
            step_stop_kind,
            curr->c_repeat.c_step.next_step_id
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    
    SEND_STR("runtime data:\n")
    SEND_STR("+-----------------------------------------------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                                              Program                                                                      |\n")
    SEND_STR("+                       +-----------------------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                       |                                                               Repeat                                                              |\n")
    SEND_STR("+                       |                                   +-----------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                       |                                   |                                          Step                                                 |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|    id     |  state    |    id     |  state    |  c_count  |    id     |   state   | state_ch  | value_st  |goal_correc| state_sp  |wait_above |  tm_rest  |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    curr=prog_list.top;
    PROG_LIST_LOOP_ST
    char *state_prog = getStateStr(curr->state);
    char *state_repeat = getStateStr(curr->c_repeat.state);
    char *state_step = getStateStr(curr->c_repeat.c_step.state);
    char *state_step_ch = getStateStr(curr->c_repeat.c_step.state_ch);
    char *state_step_sp = getStateStr(curr->c_repeat.c_step.state_sp);
    struct timespec tm_rest = getTimeRestTmr(curr->c_repeat.c_step.duration, curr->c_repeat.c_step.tmr);
    snprintf(q, sizeof q, "|%11d|%11s|%11d|%11s|%11d|%11d|%11s|%11s|%11.3f|%11.3f|%11s|%11d|%11ld|\n",
            curr->id,
            state_prog,
            curr->c_repeat.id,
            state_repeat,
            curr->c_repeat.c_count,
            curr->c_repeat.c_step.id,
            state_step,
            state_step_ch,
            curr->c_repeat.c_step.value_start,
            curr->c_repeat.c_step.goal_correction,
            state_step_sp,
            curr->c_repeat.c_step.wait_above,
            tm_rest.tv_sec
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    SEND_STR("+-------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                       Peer                                          |\n")
    SEND_STR("+--------------------------------+-----------+-----------+----------------+-----------+\n")
    SEND_STR("|               id               |   link    | sock_port |      addr      |     fd    |\n")
    SEND_STR("+--------------------------------+-----------+-----------+----------------+-----------+\n")
    for (i = 0; i < pl->length; i++) {
        snprintf(q, sizeof q, "|%32s|%11p|%11u|%16u|%11d|\n",
                pl->item[i].id,
                (void *) &pl->item[i],
                pl->item[i].addr.sin_port,
                pl->item[i].addr.sin_addr.s_addr,
                *pl->item[i].fd
                );
        SEND_STR(q)
    }
    SEND_STR_L("+--------------------------------+-----------+-----------+----------------+-----------+\n")
}

void printHelp(ACPResponse *response) {
    char q[LINE_SIZE];
    SEND_STR("COMMAND LIST\n")
    snprintf(q, sizeof q, "%s\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tfirst stop and then start process\n", ACP_CMD_APP_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tterminate process\n", ACP_CMD_APP_EXIT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tload prog into RAM and start its execution; program id expected\n", ACP_CMD_PROG_START);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tunload program from RAM; program id expected\n", ACP_CMD_PROG_STOP);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tunload program from RAM, after that load it; program id expected\n", ACP_CMD_PROG_RESET);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tenable running program; program id expected\n", ACP_CMD_PROG_ENABLE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tdisable running program; program id expected\n", ACP_CMD_PROG_DISABLE);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog state (1-enabled, 0-disabled); program id expected\n", ACP_CMD_PROG_GET_ENABLED);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog runtime data; program id expected\n", ACP_CMD_PROG_GET_DATA_RUNTIME);
    SEND_STR(q)
    snprintf(q, sizeof q, "%s\tget prog initial data; program id expected\n", ACP_CMD_PROG_GET_DATA_INIT);
    SEND_STR_L(q)
}
