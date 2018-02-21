
#include "main.h"

int app_state = APP_INIT;

char db_data_path[LINE_SIZE];
char db_public_path[LINE_SIZE];

int sock_port = -1;
int sock_fd = -1;

Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};

Mutex progl_mutex = MUTEX_INITIALIZER;
Mutex db_data_mutex = MUTEX_INITIALIZER;

PeerList peer_list;
ProgList prog_list = {NULL, NULL, 0};

#include "util.c"
#include "db.c"

int readSettings() {
#ifdef MODE_DEBUG
    printf("readSettings: configuration file to read: %s\n", CONFIG_FILE);
#endif
    FILE* stream = fopen(CONFIG_FILE, "r");
    if (stream == NULL) {
#ifdef MODE_DEBUG
        perror("readSettings()");
#endif
        return 0;
    }
    skipLine(stream);
    int n;
    n = fscanf(stream, "%d\t%ld\t%ld\t%255s\t%255s\n",
            &sock_port,
            &cycle_duration.tv_sec,
            &cycle_duration.tv_nsec,
            db_data_path,
            db_public_path
            );
    if (n != 5) {
        fclose(stream);
#ifdef MODE_DEBUG
        fputs("ERROR: readSettings: bad format\n", stderr);
#endif
        return 0;
    }
    fclose(stream);
#ifdef MODE_DEBUG
    printf("readSettings: \n\tsock_port: %d, \n\tcycle_duration: %ld sec %ld nsec, \n\tdb_data_path: %s, \n\tdb_public_path: %s\n", sock_port, cycle_duration.tv_sec, cycle_duration.tv_nsec, db_data_path, db_public_path);
#endif
    return 1;
}

void initApp() {
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    if (!initMutex(&progl_mutex)) {
        exit_nicely_e("initApp: failed to initialize prog mutex\n");
    }
    if (!initMutex(&db_data_mutex)) {
        exit_nicely_e("initApp: failed to initialize prog mutex\n");
    }
    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }
}

int initData() {
    if (!config_getPeerList(&peer_list, NULL, db_public_path)) {
        return 0;
    }
    if (lockMutex(&db_data_mutex)) {
        if (!loadActiveProg(&prog_list, &peer_list, db_data_path)) {
            freeProgList(&prog_list);
            freePeerList(&peer_list);
            return 0;
        }
        unlockMutex(&db_data_mutex);
    }

    return 1;
}

#define PARSE_I1LIST acp_requestDataToI1List(&request, &i1l);if (i1l.length <= 0) {return;}
#define PARSE_I2LIST acp_requestDataToI2List(&request, &i2l);if (i2l.length <= 0) {return;}

void serverRun(int *state, int init_state) {
    SERVER_HEADER
    SERVER_APP_ACTIONS
    DEF_SERVER_I1LIST
    DEF_SERVER_I2LIST
    if (ACP_CMD_IS(ACP_CMD_PROG_STOP)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    slaveSetStateM(&item->slave, DISABLED);
                    unlockMutex(&item->mutex);
                }
                if (lockMutex(&db_data_mutex)) {
                    deleteProgById(i1l.item[i], &prog_list, db_data_path);
                    unlockMutex(&db_data_mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_START)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            if (lockMutex(&db_data_mutex)) {
                addProgById(i1l.item[i], &prog_list, &peer_list, NULL, db_data_path);
                unlockMutex(&db_data_mutex);
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_RESET)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    slaveSetStateM(&item->slave, DISABLED);
                    unlockMutex(&item->mutex);
                }
                if (lockMutex(&db_data_mutex)) {
                    deleteProgById(i1l.item[i], &prog_list, db_data_path);
                    unlockMutex(&db_data_mutex);
                }
            }
        }
        for (int i = 0; i < i1l.length; i++) {
            if (lockMutex(&db_data_mutex)) {
                addProgById(i1l.item[i], &prog_list, &peer_list, NULL, db_data_path);
                unlockMutex(&db_data_mutex);
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_ENABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    progEnable(item);
                    if (item->save)db_saveTableFieldInt("prog", "enable", item->id, 1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_DISABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    progDisable(item);
                    if (item->save)db_saveTableFieldInt("prog", "enable", item->id, 0, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_SET_SAVE)) {
        PARSE_I2LIST
        for (int i = 0; i < i2l.length; i++) {
            Prog *item = getProgById(i2l.item[i].p0, &prog_list);
            if (item != NULL) {
                if (lockMutex(&item->mutex)) {
                    item->save = i2l.item[i].p1;
                    if (item->save)db_saveTableFieldInt("prog", "save", item->id, i2l.item[i].p1, NULL, db_data_path);
                    unlockMutex(&item->mutex);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_RUNTIME)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgRuntime(item, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_INIT)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgInit(item, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_ENABLED)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *item = getProgById(i1l.item[i], &prog_list);
            if (item != NULL) {
                if (!bufCatProgEnabled(item, &response)) {
                    return;
                }
            }
        }

    }
    acp_responseSend(&response, &peer_client);
}

int stepControl(Step *item, Slave *slave, Mutex *db_mutex, const char * db_path) {
#ifdef MODE_DEBUG
    char *state = getStateStr(item->state);
    char *state_ch = getStateStr(item->state_ch);
    char *state_sp = getStateStr(item->state_sp);
    char *ch_mode = getStateStr(item->goal_change_mode);
    char *st_kind = getStateStr(item->stop_kind);
    struct timespec tm_rest = getTimeRestTmr(item->duration, item->tmr);
    printf("--step_ini: id:%d goal:%.3f duration:%ld change_mode:%s stop_kind:%s\n", item->id, item->goal, item->duration.tv_sec, ch_mode, st_kind);
    printf("--step_run: state:%s state_ch:%s value_start:%.3f goal_corr:%.3f state_sp:%s wait_above:%d tm_rest:%ld", state, state_ch, item->value_start, item->goal_correction, state_sp, item->wait_above, tm_rest.tv_sec);

#endif
    switch (item->state) {
        case INIT:
            repeaterReset(&slave->r2);
            repeaterReset(&slave->r3);
            ton_ts_reset(&item->tmr);
            item->state_ch = OFF;
            item->state_sp = OFF;
            if (item->goal_change_mode == CHANGE_MODE_INSTANT) {
                item->state = RUN;
            }
            if (item->goal_change_mode == CHANGE_MODE_EVEN && item->stop_kind == STOP_KIND_TIME) {
                item->state = GET_VALUE;
            }
            switch (item->stop_kind) {
                case STOP_KIND_TIME:
                    item->state_sp = BYTIME;
                    break;
                case STOP_KIND_GOAL:
                    item->state = GET_VALUE;
                    break;
            }
            break;
        case GET_VALUE:
        {
            float value;
            switch (slaveGetValue(slave, &slave->r2, &value)) {
                case DONE:
                    if (item->goal_change_mode == CHANGE_MODE_EVEN && item->stop_kind == STOP_KIND_TIME) {
                        item->goal_correction = 0;
                        item->value_start = value;
                        if (item->duration.tv_sec > 0) {
                            item->goal_correction = (item->goal - value) / item->duration.tv_sec;
                        }
                        if (item->duration.tv_nsec > 0) {
                            item->goal_correction += (item->goal - value) / item->duration.tv_nsec * NANO_FACTOR;
                        }
                        item->state_ch = RUN;
                    }
                    if (item->stop_kind == STOP_KIND_GOAL) {
                        item->wait_above = (item->goal > value);
                        item->state_sp = BYGOAL;
                    }

                    item->state = RUN;

                    break;
                case WAIT:
                    break;
                case FAILURE:
                    item->state = FAILURE;
                    break;
            }
            break;
        }
        case RUN:
        {
            float goal;
            int done = 0;
            if (item->goal_change_mode == CHANGE_MODE_INSTANT) {
                goal = item->goal;
                done = 1;
            } else {
                switch (item->state_ch) {
                    case RUN:
                    {
                        struct timespec dif = getTimePassed_ts(item->tmr.start);
                        goal = item->value_start + dif.tv_sec * item->goal_correction + dif.tv_nsec * item->goal_correction * NANO_FACTOR;
                        done = 1;
                        break;
                    }
                    case FAILURE:
                        break;
                    case OFF:
                        break;
                    default:
                        item->state_ch = FAILURE;
                        break;
                }
            }

            if (done) {
                slaveSetGoalM(slave, goal);
            }

            //end of step control
            switch (item->state_sp) {
                case BYTIME:
                    if (ton_ts(item->duration, &item->tmr)) {
                        item->state = NSTEP;
                        item->state_sp = OFF;
                    }
                    break;
                case BYGOAL:
                {
                    float value;
                    switch (slaveGetValue(slave, &slave->r3, &value)) {
                        case DONE:
                        {
                            int f = 0;
                            if (item->wait_above) {
                                if (item->goal <= value) {
                                    f = 1;
                                }
                            } else {
                                if (item->goal >= value) {
                                    f = 1;
                                }
                            }
                            if (f) {
                                item->state = NSTEP;
                                item->state_sp = OFF;
                            }
                            break;
                        }
                        case WAIT:
                            break;
                        case FAILURE:
                            item->state_sp = FAILURE;
                            break;
                    }
                    break;
                }
                case FAILURE:
                    break;
                case OFF:
                    break;
                default:
                    item->state_sp = FAILURE;
                    break;
            }

            if (item->state_ch == FAILURE || item->state_sp == FAILURE) {
                item->state = FAILURE;
            }

            break;
        }
        case NSTEP:
            if (lockMutex(db_mutex)) {
                if (getStepByIdFdb(item, item->next_step_id, db_path)) {
                    item->state = INIT;
                } else {
                    item->state = DONE;
                }
                unlockMutex(db_mutex);
            }
            break;
        case DONE:
            break;
        case FAILURE:
            break;
        case OFF:
            break;
        default:
            item->state = FAILURE;
            break;
    }
#ifdef MODE_DEBUG
    putchar('\n');
#endif
    return item->state;
}

int repeatControl(Repeat *item, Slave *slave, Mutex *db_mutex, const char * db_path) {
#ifdef MODE_DEBUG
    char *state = getStateStr(item->state);
    printf("-repeat: id:%d state:%s count:%d c_count:%d \n", item->id, state, item->count, item->c_count);
#endif
    switch (item->state) {
        case INIT:
            item->c_count = 0;
            item->state = CREP;
            break;
        case CREP:
            if (item->c_count < item->count) {
                item->state = FSTEP;
                break;
            }
            item->state = NSTEP;
            break;
        case FSTEP:
            if (lockMutex(db_mutex)) {
                if (getStepByIdFdb(&item->c_step, item->first_step_id, db_path)) {
                    item->c_count++;
                    item->state = RUN;
                } else {
                    item->state = NSTEP;
                }
                unlockMutex(db_mutex);
            }
            break;
        case RUN:
            switch (stepControl(&item->c_step, slave, db_mutex, db_path)) {
                case DONE:
                    item->state = CREP;
                    break;
                case FAILURE:
                    item->state = FAILURE;
                    break;
                default:
                    break;
            }
            break;
        case NSTEP:
            if (lockMutex(db_mutex)) {
                if (getRepeatByIdFdb(item, item->next_repeat_id, db_path)) {
                    item->state = INIT;
                } else {
                    item->state = DONE;
                }
                unlockMutex(db_mutex);
            }
            break;
        case DONE:
            break;
        case FAILURE:
            break;
        case OFF:
            break;
        default:
            item->state = FAILURE;
            break;

    }
    return item->state;
}

void progControl(Prog *item, Mutex *db_mutex, const char * db_path) {
#ifdef MODE_DEBUG
    char *state = getStateStr(item->state);
    printf("prog: id:%d state:%s \n", item->id, state);
#endif
    switch (item->state) {
        case INIT:
            item->slave.r1.crepeat = 0;
            item->slave.r1.state = INIT;
            if (lockMutex(db_mutex)) {
                if (!getRepeatByIdFdb(&item->c_repeat, item->first_repeat_id, db_path)) {
                    item->state = FAILURE;
                }
                item->state = SLAVE_ENABLE;
                unlockMutex(db_mutex);
            }
            break;
        case SLAVE_ENABLE:
            switch (slaveSetState(&item->slave, &item->slave.r1, ENABLED)) {
                case DONE:
                    item->state = RUN;
                    break;
                case WAIT:
                    break;
                case FAILURE:
                    item->state = DISABLE;
                    break;
            }
            break;
        case RUN:
            switch (repeatControl(&item->c_repeat, &item->slave, db_mutex, db_path)) {
                case DONE:
                    item->state = NSTEP;
                    break;
                case FAILURE:
                    item->state = DISABLE;
                    break;
                default:
                    break;
            }
            break;
        case NSTEP:
            if (lockMutex(db_mutex)) {
                if (getRepeatByIdFdb(&item->c_repeat, item->c_repeat.next_repeat_id, db_path)) {
                    item->state = RUN;
                } else {
                    item->state = OFF;
                }
                unlockMutex(db_mutex);
            }
            break;
        case DISABLE:
            switch (slaveSetState(&item->slave, &item->slave.r1, DISABLED)) {
                case DONE:
                case FAILURE:
                    item->state = FAILURE;
                    break;
            }
            break;
        case FAILURE:
            break;
        case OFF:
            break;
        default:
            item->state = OFF;
            break;
    }
}

void cleanup_handler(void *arg) {
    Prog *item = arg;
    printf("cleaning up thread %d\n", item->id);
}

void *threadFunction(void *arg) {
    Prog *item = arg;
#ifdef MODE_DEBUG
    printf("thread for program with id=%d has been started\n", item->id);
#endif
#ifdef MODE_DEBUG
    pthread_cleanup_push(cleanup_handler, item);
#endif
    while (1) {
        struct timespec t1 = getCurrentTime();
        int old_state;
        if (threadCancelDisable(&old_state)) {
            if (lockMutex(&item->mutex)) {
                progControl(item, &db_data_mutex, db_data_path);
                unlockMutex(&item->mutex);
            }
            threadSetCancelState(old_state);
        }
        sleepRest(item->cycle_duration, t1);
    }
#ifdef MODE_DEBUG
    pthread_cleanup_pop(1);
#endif
}

void freeData() {
    stopAllProgThreads(&prog_list);
    secure();
    freeProgList(&prog_list);
    freePeerList(&peer_list);
#ifdef MODE_DEBUG
    puts("freeData: done");
#endif
}

void freeApp() {
    freeData();
    freeSocketFd(&sock_fd);
    freeMutex(&db_data_mutex);
    freeMutex(&progl_mutex);
}

void exit_nicely() {
    freeApp();
#ifdef MODE_DEBUG
    puts("\nBye...");
#endif
    exit(EXIT_SUCCESS);
}

void exit_nicely_e(char *s) {
#ifdef MODE_DEBUG
    fprintf(stderr, "%s", s);
#endif
    freeApp();
    exit(EXIT_FAILURE);
}

int main(int argc, char** argv) {
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    int data_initialized = 0;
    while (1) {
#ifdef MODE_DEBUG
        printf("%s(): %s %d\n", F,getAppState(app_state), data_initialized);
#endif
        switch (app_state) {
            case APP_INIT:
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
                data_initialized = initData();
                app_state = APP_RUN;
                delayUsIdle(1000000);
                break;
            case APP_RUN:
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
                freeApp();
                delayUsIdle(1000000);
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
                exit_nicely();
                break;
            default:
                exit_nicely_e("main: unknown application state");
                break;
        }
    }
    freeApp();
    return (EXIT_SUCCESS);
}