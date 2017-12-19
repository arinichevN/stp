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

int slaveSetStateM(Slave *item, int state) {
    int id = item->remote_id;
    I1List data = {.item = &id, .length = 1};
    if (!lockPeer(item->peer)) {
        return 0;
    }
    switch (state) {
        case ENABLED:
            if (!acp_requestSendUnrequitedI1List(ACP_CMD_PROG_ENABLE, &data, item->peer)) {
                unlockPeer(item->peer);
                return 0;
            }
            break;
        case DISABLED:
            if (!acp_requestSendUnrequitedI1List(ACP_CMD_PROG_DISABLE, &data, item->peer)) {
                unlockPeer(item->peer);
                return 0;
            }
            break;
    }
    unlockPeer(item->peer);
    return 1;
}

int slaveCheckState(Slave *item, int state) {
    ACPRequest request;
    I1List data = {.item = &item->remote_id, .length = 1};
    if (!lockPeer(item->peer)) {
        return 0;
    }
    if (!acp_requestSendI1List(ACP_CMD_PROG_GET_ENABLED, &data, &request, item->peer)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckState(): acp_requestSendI1List failed where peer.id = %s and remote_id=%d\n", item->peer->id, item->remote_id);
#endif
        unlockPeer(item->peer);
        return 0;
    }
    I2 td1;
    I2List tl1 = {&td1, 0};
    if (!acp_responseReadI2List(&tl1, 1, &request, item->peer)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckState(): acp_responseReadI2List() error where peer.id = %s and remote_id=%d\n", item->peer->id, item->remote_id);
#endif
        unlockPeer(item->peer);
        return 0;
    }
    unlockPeer(item->peer);
    if (tl1.item[0].p0 != item->remote_id) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckState(): response: peer returned id=%d but requested one was %d\n", tl1.item[0].p0, item->remote_id);
#endif
        return 0;
    }
    if (tl1.length != 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckState(): response: number of items = %d but 1 expected\n", tl1.length);
#endif
        return 0;
    }
    switch (state) {
        case ENABLED:
            if (tl1.item[0].p1 == 1) {
                return 1;
            }
            break;
        case DISABLED:
            if (tl1.item[0].p1 == 0) {
                return 1;
            }
            break;
    }

    return 0;
}

int slaveSetGoalM(Slave *item, float goal) {
    I1F1 i1f1 = {.p0 = item->remote_id, .p1 = goal};
    I1F1List data = {.item = &i1f1, .length = 1};
    if (!lockPeer(item->peer)) {
        return 0;
    }
    if (!acp_requestSendUnrequitedI1F1List(ACP_CMD_REG_PROG_SET_GOAL, &data, item->peer)) {
        unlockPeer(item->peer);
        return 0;
    }
    unlockPeer(item->peer);
    return 1;
}

int slaveCheckGoal(Slave *item, float goal) {
    ACPRequest request;
    I1List data = {.item = &item->remote_id, .length = 1};
    if (!lockPeer(item->peer)) {
        return 0;
    }
    if (!acp_requestSendI1List(ACP_CMD_REG_PROG_GET_GOAL, &data, &request, item->peer)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckGoal(): acp_requestSendI1List failed where peer.id = %s and remote_id=%d\n", item->peer->id, item->remote_id);
#endif
        unlockPeer(item->peer);
        return 0;
    }
    I1F1 td1;
    I1F1List tl1 = {&td1, 0};
    if (!acp_responseReadI1F1List(&tl1, 1, &request, item->peer)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckGoal(): acp_responseReadI1F1List() error where peer.id = %s and remote_id=%d\n", item->peer->id, item->remote_id);
#endif
        unlockPeer(item->peer);
        return 0;
    }
    unlockPeer(item->peer);
    if (tl1.item[0].p0 != item->remote_id) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckGoal(): response: peer returned id=%d but requested one was %d\n", tl1.item[0].p0, item->remote_id);
#endif
        return 0;
    }
    if (tl1.length != 1) {
#ifdef MODE_DEBUG
        fprintf(stderr, "slaveCheckGoal(): response: number of items = %d but 1 expected\n", tl1.length);
#endif
        return 0;
    }
    if (aeq(tl1.item[0].p1, goal, PRECISION)) {
        return 1;
    }
    return 0;
}

int slaveGetValue(Slave *item, float *value) {
    FTS fts;
    switch (item->state) {
        case INIT:
            item->state = DO;
            item->crepeat = 0;
        case DO:
            if (item->crepeat < item->retry_count) {
                if (acp_getFTS(&fts, item->peer, item->remote_id)) {
#ifdef MODE_DEBUG
                    printf(" value:%.3f", fts.value);
#endif
                    *value = fts.value;
                    item->state = INIT;
                    return DONE;
                } else {
                    item->crepeat++;
                    return WAIT;
                }
            }
            break;
    }

#ifdef MODE_DEBUG
    fprintf(stderr, "slaveGetValue(): failed to get value where slave.remote_id = %d\n", item->remote_id);
#endif
    item->state = INIT;
    return FAILURE;
}

int slaveSetState(Slave *item, int state) {
    switch (item->state) {
        case INIT:
            item->state = DO;
            item->crepeat = 0;
        case DO:
            if (item->check && item->crepeat >= item->retry_count) {
                item->state = INIT;
                return FAILURE;
            }
            if (slaveSetStateM(item, state)) {
                if (item->check) {
                    item->state = CHECK;
                    return WAIT;
                } else {
                    item->state = INIT;
                    return DONE;
                }
            } else {
                item->crepeat++;
            }
            break;
        case CHECK:
        {
            if (slaveCheckState(item, state)) {
                item->state = INIT;
                return DONE;
            } else {
                item->crepeat++;
                item->state = DO;
            }
            break;
        }

    }
    return FAILURE;
}

int slaveSetGoal(Slave *item, float goal) {
    switch (item->state) {
        case INIT:
            item->state = DO;
            item->crepeat = 0;
        case DO:
            if (item->check && item->crepeat >= item->retry_count) {
                item->state = INIT;
                return FAILURE;
            }
            if (slaveSetGoalM(item, goal)) {
                if (item->check) {
                    item->state = CHECK;
                    return WAIT;
                } else {
#ifdef MODE_DEBUG
                    printf(" set goal:%.3f", goal);
#endif
                    item->state = INIT;
                    return DONE;
                }
            } else {
                item->crepeat++;
            }
            break;
        case CHECK:
        {
            if (slaveCheckGoal(item, goal)) {
                item->state = INIT;
                return DONE;
            } else {
                item->crepeat++;
                item->state = DO;
            }
            break;
        }

    }
    return FAILURE;
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
    if (item->state != OFF) {
        slaveSetStateM(&item->slave, DISABLED);
    }
    item->state = OFF;
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
    snprintf(q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
            item->id,
            item->first_repeat_id,
            item->slave.peer->id,
            item->slave.remote_id,
            item->slave.check,
            item->slave.retry_count,
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
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE);
    SEND_STR(q)
    snprintf(q, sizeof q, "port: %d\n", sock_port);
    SEND_STR(q)
    snprintf(q, sizeof q, "pid_path: %s\n", pid_path);
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
    SEND_STR("+-----------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                                                            Program                                                                                      |\n")
    SEND_STR("+                                                                 +-----------------------------------------------------------------------------------------------------------------------+\n")
    SEND_STR("|                                                                 |                                                     Repeat                                                            |\n")
    SEND_STR("+                                                                 |                                               +-----------------------------------------------------------------------+\n")
    SEND_STR("|                                                                 |                                               |                               Step                                    |\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    SEND_STR("|    id     |fst_rep_id |  peer_id  | remote_id |check| retry_cnt |    id     |fst_stp_id |   count   |next_rep_id|     id    |   goal    | duration  |change_mode| stop_kind |next_stp_id|\n")
    SEND_STR("+-----------+-----------+-----------+-----------+-----+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
            char *step_goal_change_mode = getStateStr(item->c_repeat.c_step.goal_change_mode);
    char *step_stop_kind = getStateStr(item->c_repeat.c_step.stop_kind);
    snprintf(q, sizeof q, "|%11d|%11d|%11s|%11d|%5d|%11d|%11d|%11d|%11d|%11d|%11d|%11.3f|%11ld|%11s|%11s|%11d|\n",
            item->id,
            item->first_repeat_id,
            item->slave.peer->id,
            item->slave.remote_id,
            item->slave.check,
            item->slave.retry_count,
            item->c_repeat.id,
            item->c_repeat.first_step_id,
            item->c_repeat.count,
            item->c_repeat.next_repeat_id,
            item->c_repeat.c_step.id,
            item->c_repeat.c_step.goal,
            item->c_repeat.c_step.duration.tv_sec,
            step_goal_change_mode,
            step_stop_kind,
            item->c_repeat.c_step.next_step_id
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

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
    PROG_LIST_LOOP_DF
    PROG_LIST_LOOP_ST
            char *state_prog = getStateStr(item->state);
    char *state_repeat = getStateStr(item->c_repeat.state);
    char *state_step = getStateStr(item->c_repeat.c_step.state);
    char *state_step_ch = getStateStr(item->c_repeat.c_step.state_ch);
    char *state_step_sp = getStateStr(item->c_repeat.c_step.state_sp);
    struct timespec tm_rest = getTimeRestTmr(item->c_repeat.c_step.duration, item->c_repeat.c_step.tmr);
    snprintf(q, sizeof q, "|%11d|%11s|%11d|%11s|%11d|%11d|%11s|%11s|%11.3f|%11.3f|%11s|%11d|%11ld|\n",
            item->id,
            state_prog,
            item->c_repeat.id,
            state_repeat,
            item->c_repeat.c_count,
            item->c_repeat.c_step.id,
            state_step,
            state_step_ch,
            item->c_repeat.c_step.value_start,
            item->c_repeat.c_step.goal_correction,
            state_step_sp,
            item->c_repeat.c_step.wait_above,
            tm_rest.tv_sec
            );
    SEND_STR(q)
    PROG_LIST_LOOP_SP
    SEND_STR("+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n")

    acp_sendPeerListInfo(&peer_list, response, &peer_client);
    
    SEND_STR_L("_")

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
