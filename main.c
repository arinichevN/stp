
#include "main.h"

char pid_path[LINE_SIZE];
int app_state = APP_INIT;

char db_data_path[LINE_SIZE];
char db_public_path[LINE_SIZE];

int pid_file = -1;
int proc_id;
unsigned int retry_count = 0;
int sock_port = -1;
int sock_fd = -1;
int sock_fd_tf = -1;
Peer peer_client = {.fd = &sock_fd, .addr_size = sizeof peer_client.addr};
struct timespec cycle_duration = {0, 0};
DEF_THREAD
Mutex progl_mutex = {.created = 0, .attr_initialized = 0};
I1List i1l = {NULL, 0};
PeerList peer_list = {NULL, 0};
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
    n = fscanf(stream, "%d\t%255s\t%u\t%ld\t%ld\t%255s\t%255s\n",
            &sock_port,
            pid_path,
            &retry_count,
            &cycle_duration.tv_sec,
            &cycle_duration.tv_nsec,
            db_data_path,
            db_public_path
            );
    if (n != 7) {
        fclose(stream);
#ifdef MODE_DEBUG
        fputs("ERROR: readSettings: bad format\n", stderr);
#endif
        return 0;
    }
    fclose(stream);
#ifdef MODE_DEBUG
    printf("readSettings: \n\tsock_port: %d, \n\tpid_path: %s, \n\tretry_count: %u, \n\tcycle_duration: %ld sec %ld nsec, \n\tdb_data_path: %s, \n\tdb_public_path: %s\n", sock_port, pid_path,retry_count, cycle_duration.tv_sec, cycle_duration.tv_nsec, db_data_path, db_public_path);
#endif
    return 1;
}

void initApp() {
    if (!readSettings()) {
        exit_nicely_e("initApp: failed to read settings\n");
    }
    if (!initPid(&pid_file, &proc_id, pid_path)) {
        exit_nicely_e("initApp: failed to initialize pid\n");
    }
    if (!initMutex(&progl_mutex)) {
        exit_nicely_e("initApp: failed to initialize prog mutex\n");
    }

    if (!initServer(&sock_fd, sock_port)) {
        exit_nicely_e("initApp: failed to initialize udp server\n");
    }

    if (!initClient(&sock_fd_tf, WAIT_RESP_TIMEOUT)) {
        exit_nicely_e("initApp: failed to initialize udp client\n");
    }
}

int initData() {
    if (!config_getPeerList(&peer_list, &sock_fd_tf, db_public_path)) {
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!loadActiveProg(&prog_list, &peer_list, db_data_path)) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to load active programs\n", stderr);
#endif
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!initI1List(&i1l, ACP_BUFFER_MAX_SIZE)) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to allocate memory for i1l\n", stderr);
#endif
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    if (!THREAD_CREATE) {
#ifdef MODE_DEBUG
        fputs("initData: ERROR: failed to create thread\n", stderr);
#endif
        FREE_LIST(&i1l);
        freeProg(&prog_list);
        FREE_LIST(&peer_list);
        return 0;
    }
    return 1;
}

#define PARSE_I1LIST acp_requestDataToI1List(&request, &i1l, prog_list.length);if (i1l.length <= 0) {return;}

void serverRun(int *state, int init_state) {
    SERVER_HEADER
    SERVER_APP_ACTIONS

    if (ACP_CMD_IS(ACP_CMD_PROG_STOP)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                 slaveDisable(&curr->slave);
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_START)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_RESET)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                 slaveDisable(&curr->slave);
                deleteProgById(i1l.item[i], &prog_list, db_data_path);
            }
        }
        for (int i = 0; i < i1l.length; i++) {
            addProgById(i1l.item[i], &prog_list, &peer_list, db_data_path);
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_ENABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    progEnable(curr);
                    saveProgEnable(curr->id, 1, db_data_path);
                    unlockProg(curr);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_DISABLE)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (lockProg(curr)) {
                    progDisable(curr);
                    saveProgEnable(curr->id, 0, db_data_path);
                    unlockProg(curr);
                }
            }
        }
        return;
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_RUNTIME)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgRuntime(curr, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_DATA_INIT)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgInit(curr, &response)) {
                    return;
                }
            }
        }
    } else if (ACP_CMD_IS(ACP_CMD_PROG_GET_ENABLED)) {
        PARSE_I1LIST
        for (int i = 0; i < i1l.length; i++) {
            Prog *curr = getProgById(i1l.item[i], &prog_list);
            if (curr != NULL) {
                if (!bufCatProgEnabled(curr, &response)) {
                    return;
                }
            }
        }

    }
    acp_responseSend(&response, &peer_client);
}

int stepControl(Step *item, Slave *slave, const char * db_path) {
#ifdef MODE_DEBUG
    char *state = getStateStr(item->state);
    char *state_ch = getStateStr(item->state);
    char *state_sp = getStateStr(item->state);
    struct timespec tm_rest = getTimeRestTmr(item->duration, item->tmr);
    printf("\t\tstep: id: %d state: %s state_ch: %s value_start: %f goal_corr: %f state_sp: %s wait_above: %d tm_rest: %ld \n", item->id, state, state_ch,item->value_start, item->goal_correction, state_sp, item->wait_above, tm_rest.tv_sec);
#endif
    switch (item->state) {
        case INIT:
            item->tmr.ready = 0;
            item->state_ch = INIT;
            item->state_sp = INIT;
            item->state = RUN;
            break;
        case RUN:

            //goal correction
            switch (item->state_ch) {
                case INIT:
                    if (item->goal_change_mode == CHANGE_MODE_EVEN && item->stop_kind == STOP_KIND_TIME) {
                        float value;
                        if (slaveGetValue(slave, &value)) {
                            item->goal_correction = 0;
                            item->value_start = value;
                            if (item->duration.tv_sec > 0) {
                                item->goal_correction = (item->goal - value) / item->duration.tv_sec;
                            }
                            if (item->duration.tv_nsec > 0) {
                                item->goal_correction += (item->goal - value) / item->duration.tv_nsec * NANO_FACTOR;
                            }
                            item->state_ch = RUN;
                        } else {
                            item->state_ch = FAILURE;
                        }
                    } else {
                        item->state_ch = OFF;
                    }
                    break;
                case RUN:
                {
                    struct timespec dif = getTimePassed_ts(item->tmr.start);
                    float goal = item->value_start + dif.tv_sec * item->goal_correction + dif.tv_nsec * item->goal_correction * NANO_FACTOR;
                    if (!slaveSetGoal(slave, goal)) {
                        item->state_ch = FAILURE;
                    }
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

            //end of step control
            switch (item->state_sp) {
                case INIT:
                    if (item->stop_kind == STOP_KIND_TIME) {
                        item->state_sp = BYTIME;
                    } else if (item->stop_kind == STOP_KIND_GOAL) {
                        float value;
                        if (slaveGetValue(slave, &value)) {
                            item->wait_above = (item->goal > value);
                            item->state_sp = BYGOAL;
                        } else {
                            item->state_sp = FAILURE;
                        }
                    }
                    break;
                case BYTIME:
                    if (ton_ts(item->duration, &item->tmr)) {
                        item->state = NSTEP;
                    }
                    break;
                case BYGOAL:
                {
                    float value;
                    if (slaveGetValue(slave, &value)) {
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
                        }
                    } else {
                        item->state_sp = FAILURE;
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
        case NSTEP:
            if (getStepByIdFdb(item, item->next_step_id, db_path)) {
                item->state = INIT;
            } else {
                item->state = DONE;
            }
            break;
            item->state = DONE;
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
int repeatControl(Repeat *item, Slave *slave, const char * db_path) {
#ifdef MODE_DEBUG
    char *state = getStateStr(item->state);
    printf("\trepeat: id: %d state: %s c_count: %d \n", item->id, state, item->c_count);
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
            if (getStepByIdFdb(&item->c_step, item->first_step_id, db_path)) {
                item->c_count++;
                item->state = RUN;
            } else {
                item->state = NSTEP;
            }
            break;
        case RUN:
            switch (stepControl(&item->c_step, slave, db_path)) {
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
            if (getRepeatByIdFdb(item, item->next_repeat_id, db_path)) {
                item->state = INIT;
            } else {
                item->state = DONE;
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

void progControl(Prog *item, const char * db_path) {
#ifdef MODE_DEBUG
    char *state = getStateStr(item->state);
    printf("prog: id: %d state: %s \n", item->id, state);
#endif
    switch (item->state) {
        case INIT:
            if (getRepeatByIdFdb(&item->c_repeat, item->first_repeat_id, db_path)) {
                if (slaveEnable(&item->slave)) {
                    item->state = RUN;
                    break;
                }
            }
            item->state = OFF;
            break;
        case RUN:
            switch (repeatControl(&item->c_repeat, &item->slave, db_path)) {
                case DONE:
                    item->state = NSTEP;
                    break;
                case FAILURE:
                    item->state = FAILURE;
                    break;
                default:
                    break;
            }
            break;
        case NSTEP:
            if (getRepeatByIdFdb(&item->c_repeat, item->c_repeat.next_repeat_id, db_path)) {
                item->state = RUN;
            } else {
                item->state = OFF;
            }
            break;
        case DISABLE:
            slaveDisable(&item->slave);
            item->state = OFF;
            break;
        case FAILURE:
            break;
        case OFF:
            break;
        default:
            item->state = OFF;
            break;
    }
#ifdef MODE_DEBUG
    putchar('\n');
#endif
}

void *threadFunction(void *arg) {
    THREAD_DEF_CMD
#ifdef MODE_DEBUG
            puts("threadFunction: running...");
#endif
    while (1) {
        struct timespec t1 = getCurrentTime();

        lockProgList();
        Prog *curr = prog_list.top;
        unlockProgList();
        while (1) {
            if (curr == NULL) {
                break;
            }
            if (tryLockProg(curr)) {
                progControl(curr, db_data_path);
                Prog *temp = curr;
                curr = curr->next;
                unlockProg(temp);
            }
            THREAD_EXIT_ON_CMD
        }
        THREAD_EXIT_ON_CMD
        sleepRest(cycle_duration, t1);
    }
}

void freeProg(ProgList * list) {
    Prog *curr = list->top, *temp;
    while (curr != NULL) {
        temp = curr;
        curr = curr->next;
        free(temp);
    }
    list->top = NULL;
    list->last = NULL;
    list->length = 0;
}

void freeData() {
    THREAD_STOP
    secure();
    freeProg(&prog_list);
    FREE_LIST(&i1l);
    FREE_LIST(&peer_list);
#ifdef MODE_DEBUG
    puts("freeData: done");
#endif
}

void freeApp() {
    freeData();
    freeSocketFd(&sock_fd);
    freeSocketFd(&sock_fd_tf);
    freeMutex(&progl_mutex);
    freePid(&pid_file, &proc_id, pid_path);
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
    if (geteuid() != 0) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s: root user expected\n", APP_NAME_STR);
#endif
        return (EXIT_FAILURE);
    }
#ifndef MODE_DEBUG
    daemon(0, 0);
#endif
    conSig(&exit_nicely);
    if (mlockall(MCL_CURRENT | MCL_FUTURE) == -1) {
        perror("main: memory locking failed");
    }
    int data_initialized = 0;
    while (1) {
        switch (app_state) {
            case APP_INIT:
#ifdef MODE_DEBUG
                puts("MAIN: init");
#endif
                initApp();
                app_state = APP_INIT_DATA;
                break;
            case APP_INIT_DATA:
#ifdef MODE_DEBUG
                puts("MAIN: init data");
#endif
                data_initialized = initData();
                app_state = APP_RUN;
                delayUsIdle(1000000);
                break;
            case APP_RUN:
#ifdef MODE_DEBUG
                puts("MAIN: run");
#endif
                serverRun(&app_state, data_initialized);
                break;
            case APP_STOP:
#ifdef MODE_DEBUG
                puts("MAIN: stop");
#endif
                freeData();
                data_initialized = 0;
                app_state = APP_RUN;
                break;
            case APP_RESET:
#ifdef MODE_DEBUG
                puts("MAIN: reset");
#endif
                freeApp();
                delayUsIdle(1000000);
                data_initialized = 0;
                app_state = APP_INIT;
                break;
            case APP_EXIT:
#ifdef MODE_DEBUG
                puts("MAIN: exit");
#endif
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