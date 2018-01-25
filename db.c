#include "main.h"

int getProg_callback(void *d, int argc, char **argv, char **azColName) {
    ProgData * data = d;
    Prog *item = data->prog;
    int load = 0, enable = 0;

    int c = 0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("id")) {
            item->id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("peer_id")) {
            Peer *peer = getPeerById(argv[i], data->peer_list);
            if (peer == NULL) {
#ifdef MODE_DEBUG
                fprintf(stderr, "%s(): no peer with id %s found\n", __FUNCTION__, argv[i]);
#endif
                return EXIT_FAILURE;
            }
            item->slave.peer = *peer;
            c++;
        } else if (DB_COLUMN_IS("first_repeat_id")) {
            item->first_repeat_id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("remote_id")) {
            item->slave.remote_id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("check")) {
            item->slave.check = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("retry_count")) {
            item->slave.r1.retry_count = item->slave.r2.retry_count = item->slave.r3.retry_count = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("enable")) {
            enable = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("load")) {
            load = atoi(argv[i]);
            c++;
        } else {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): unknown column\n", __FUNCTION__);
#endif

        }
    }
#define N 8
    if (c != N) {
        fprintf(stderr, "%s(): required %d columns but %d found\n", __FUNCTION__, N, c);
        return EXIT_FAILURE;
    }
#undef N
    if (enable) {
        item->state=INIT;
    } else {
        item->state=OFF;
    }
    if (!load) {
        db_saveTableFieldInt("prog", "load", item->id, 1, data->db_data, NULL);
    }
    return EXIT_SUCCESS;
}

int getProgByIdFDB(int prog_id, Prog *item, PeerList *em_list, sqlite3 *dbl, const char *db_path) {
    if (dbl != NULL && db_path != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): dbl xor db_path expected\n", __FUNCTION__);
#endif
        return 0;
    }
    sqlite3 *db;
    int close = 0;
    if (db_path != NULL) {
        if (!db_open(db_path, &db)) {
            return 0;
        }
        close = 1;
    } else {
        db = dbl;
    }
    char q[LINE_SIZE];
    ProgData data = {.peer_list = em_list, .prog = item, .db_data = db};
    snprintf(q, sizeof q, "select " PROG_FIELDS " from prog where id=%d", prog_id);
    if (!db_exec(db, q, getProg_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): query failed\n", __FUNCTION__);
#endif
        if (close)sqlite3_close(db);
        return 0;
    }
    if (close)sqlite3_close(db);
    return 1;
}

int addProg(Prog *item, ProgList *list) {
    if (list->length >= INT_MAX) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): can not load prog with id=%d - list length exceeded\n", __FUNCTION__, item->id);
#endif
        return 0;
    }
    if (list->top == NULL) {
        lockProgList();
        list->top = item;
        unlockProgList();
    } else {
        lockMutex(&list->last->mutex);
        list->last->next = item;
        unlockMutex(&list->last->mutex);
    }
    list->last = item;
    list->length++;
#ifdef MODE_DEBUG
    printf("%s(): prog with id=%d loaded\n", __FUNCTION__, item->id);
#endif
    return 1;
}

int loadRepeat_callback(void *d, int argc, char **argv, char **azColName) {
    Repeat *item = (Repeat *) d;
    memset(item, 0, sizeof *item);
    int c=0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("id")) {
            item->id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("first_step_id")) {
            item->first_step_id = atof(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("count")) {
            item->count = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("next_repeat_id")) {
            item->next_repeat_id = atoi(argv[i]);
            c++;
        } else {
#ifdef MODE_DEBUG
            fprintf(stderr, "loadRepeat_callback: unknown column: %s\n", azColName[i]);
#endif
            return EXIT_FAILURE;
        }
    }
    #define N 4
    if (c != N) {
        fprintf(stderr, "%s(): required %d columns but %d found\n", __FUNCTION__, N, c);
        return EXIT_FAILURE;
    }
#undef N
    item->state = INIT;
    if (!checkRepeat(item)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int loadStep_callback(void *d, int argc, char **argv, char **azColName) {
    Step *item = (Step *) d;
    memset(item, 0, sizeof *item);
    int c=0;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("id")) {
            item->id = atoi(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("goal")) {
            item->goal = atof(argv[i]);
            c++;
        } else if (DB_COLUMN_IS("duration")) {
            item->duration.tv_sec = atoi(argv[i]);
            item->duration.tv_nsec = 0;
            c++;
        } else if (DB_COLUMN_IS("goal_change_mode")) {
            if (strcmp(CHANGE_MODE_EVEN_STR, argv[i]) == 0) {
                item->goal_change_mode = CHANGE_MODE_EVEN;
            } else if (strcmp(CHANGE_MODE_INSTANT_STR, argv[i]) == 0) {
                item->goal_change_mode = CHANGE_MODE_INSTANT;
            } else {
                item->goal_change_mode = UNKNOWN;
            }
            c++;
        } else if (DB_COLUMN_IS("stop_kind")) {
            if (strcmp(STOP_KIND_TIME_STR, argv[i]) == 0) {
                item->stop_kind = STOP_KIND_TIME;
            } else if (strcmp(STOP_KIND_GOAL_STR, argv[i]) == 0) {
                item->stop_kind = STOP_KIND_GOAL;
            } else {
                item->stop_kind = UNKNOWN;
            }
            c++;
        } else if (DB_COLUMN_IS("next_step_id")) {
            item->next_step_id = atoi(argv[i]);
            c++;
        } else {
#ifdef MODE_DEBUG
            fprintf(stderr, "loadStep_callback: unknown column: %s\n", azColName[i]);
#endif
            return EXIT_FAILURE;
        }
    }
    #define N 6
    if (c != N) {
        fprintf(stderr, "%s(): required %d columns but %d found\n", __FUNCTION__, N, c);
        return EXIT_FAILURE;
    }
#undef N
    item->state = INIT;
    if (!checkStep(item)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int addProgById(int prog_id, ProgList *list, PeerList *peer_list, sqlite3 *db_data, const char *db_data_path) {
    extern struct timespec cycle_duration;
    Prog *rprog = getProgById(prog_id, list);
    if (rprog != NULL) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): program with id = %d is being controlled by program\n", __FUNCTION__, rprog->id);
#endif
        return 0;
    }

    Prog *item = malloc(sizeof *(item));
    if (item == NULL) {
        fprintf(stderr, "%s(): failed to allocate memory\n", __FUNCTION__);
        return 0;
    }
    memset(item, 0, sizeof *item);
    item->id = prog_id;
    item->next = NULL;
    item->cycle_duration = cycle_duration;
    if (!initMutex(&item->mutex)) {
        free(item);
        return 0;
    }
    if (!initClient(&item->sock_fd, WAIT_RESP_TIMEOUT)) {
        freeMutex(&item->mutex);
        free(item);
        return 0;
    }
    if (!getProgByIdFDB(item->id, item, peer_list, db_data, db_data_path)) {
        freeSocketFd(&item->sock_fd);
        freeMutex(&item->mutex);
        free(item);
        return 0;
    }
    item->slave.peer.fd = &item->sock_fd;
    if (!checkProg(item)) {
        freeSocketFd(&item->sock_fd);
        freeMutex(&item->mutex);
        free(item);
        return 0;
    }
    if (!addProg(item, list)) {
        freeSocketFd(&item->sock_fd);
        freeMutex(&item->mutex);
        free(item);
        return 0;
    }
    if (!createMThread(&item->thread, &threadFunction, item)) {
        freeSocketFd(&item->sock_fd);
        freeMutex(&item->mutex);
        free(item);
        return 0;
    }
    return 1;
}

int deleteProgById(int id, ProgList *list, const char* db_path) {
#ifdef MODE_DEBUG
    printf("prog to delete: %d\n", id);
#endif
    Prog *prev = NULL, *curr;
    int done = 0;
    curr = list->top;
    while (curr != NULL) {
        if (curr->id == id) {
            if (prev != NULL) {
                lockMutex(&prev->mutex);
                prev->next = curr->next;
                unlockMutex(&prev->mutex);
            } else {//curr=top
                lockProgList();
                list->top = curr->next;
                unlockProgList();
            }
            if (curr == list->last) {
                list->last = prev;
            }
            list->length--;
            stopProgThread(curr);
            db_saveTableFieldInt("prog", "load", curr->id, 0, NULL, db_data_path);
            freeProg(curr);
#ifdef MODE_DEBUG
            printf("prog with id: %d deleted from prog_list\n", id);
#endif
            done = 1;
            break;
        }
        prev = curr;
        curr = curr->next;
    }

    return done;
}

int loadActiveProg_callback(void *d, int argc, char **argv, char **azColName) {
    ProgData *data = d;
    for (int i = 0; i < argc; i++) {
        if (DB_COLUMN_IS("id")) {
            int id = atoi(argv[i]);
            printf("%s: %d\n", __FUNCTION__, id);
            addProgById(id, data->prog_list, data->peer_list, data->db_data, NULL);
        } else {
#ifdef MODE_DEBUG
            fprintf(stderr, "%s(): unknown column\n", __FUNCTION__);
#endif
        }
    }
    return EXIT_SUCCESS;
}

int loadActiveProg(ProgList *list, PeerList *peer_list, char *db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    ProgData data = {.prog_list = list, .peer_list = peer_list, .db_data = db};
    char *q = "select id from prog where load=1";
    if (!db_exec(db, q, loadActiveProg_callback, &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "%s(): query failed\n", __FUNCTION__);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int getStepByIdFdb(Step *item, int id, const char *db_path) {
    Step tmp;
    tmp.state = UNKNOWN;
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select id,goal,duration,goal_change_mode,stop_kind,next_step_id from step where id=%d limit 1", id);
    if (!db_exec(db, q, loadStep_callback, (void*) &tmp)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "getStepByIdFdb: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    if (tmp.state != INIT) {
        return 0;
    }
    *item = tmp;
    return 1;
}

int getRepeatByIdFdb(Repeat *item, int id, const char *db_path) {
    Repeat tmp;
    tmp.state = UNKNOWN;
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select id,first_step_id,count,next_repeat_id from repeat where id=%d limit 1", id);
    if (!db_exec(db, q, loadRepeat_callback, (void*) &tmp)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "getRepeatByIdFdb: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    if (tmp.state != INIT) {
        return 0;
    }
    *item = tmp;
    return 1;
}
