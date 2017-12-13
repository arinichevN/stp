#include "main.h"

int checkProg(const Prog *item, const ProgList *list) {
    if (item->slave.peer == NULL) {
        fprintf(stderr, "checkProg: no peer attached to prog with id = %d\n", item->id);
        return 0;
    }
    //unique id
    if (getProgById(item->id, list) != NULL) {
        fprintf(stderr, "checkProg: prog with id = %d is already running\n", item->id);
        return 0;
    }
    return 1;
}

int checkStep(const Step *item) {
    if (item->duration.tv_sec < 0 || item->duration.tv_nsec < 0) {
        fprintf(stderr, "checkStep: bad duration where id = %d\n", item->id);
        return 0;
    }
    if (item->goal_change_mode == UNKNOWN) {
        fprintf(stderr, "checkStep: bad goal_change_mode where id = %d\n", item->id);
        return 0;
    }
    if (item->stop_kind == UNKNOWN) {
        fprintf(stderr, "checkStep: bad stop_kind where id = %d\n", item->id);
        return 0;
    }
    return 1;
}

int checkRepeat(const Repeat *item) {
    if (item->count < 0) {
        fprintf(stderr, "checkRepeat: bad count where id = %d\n", item->id);
        return 0;
    }
    return 1;
}

int addProg(Prog *item, ProgList *list) {
    if (list->length >= INT_MAX) {
#ifdef MODE_DEBUG
        fprintf(stderr, "addProg: ERROR: can not load prog with id=%d - list length exceeded\n", item->id);
#endif
        return 0;
    }
    if (list->top == NULL) {
        lockProgList();
        list->top = item;
        unlockProgList();
    } else {
        lockProg(list->last);
        list->last->next = item;
        unlockProg(list->last);
    }
    list->last = item;
    list->length++;
#ifdef MODE_DEBUG
    printf("addProg: prog with id=%d loaded\n", item->id);
#endif
    return 1;
}

void saveProgLoad(int id, int v, sqlite3 *db) {
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set load=%d where id=%d", v, id);
    if (!db_exec(db, q, 0, 0)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "saveProgLoad: query failed: %s\n", q);
#endif
    }
}

void saveProgEnable(int id, int v, const char* db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        printfe("saveProgEnable: failed to open db: %s\n", db_path);
        return;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set enable=%d where id=%d", v, id);
    if (!db_exec(db, q, 0, 0)) {
        printfe("saveProgEnable: query failed: %s\n", q);
    }
}

void saveProgFieldFloat(int id, float value, const char* db_path, const char *field) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        printfe("saveProgFieldFloat: failed to open db where id=%d\n", id);
        return;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set %s=%f where id=%d", field, value, id);
    if (!db_exec(db, q, 0, 0)) {
        printfe("saveProgFieldFloat: query failed: %s\n", q);
    }
    sqlite3_close(db);
}

void saveProgFieldInt(int id, int value, const char* db_path, const char *field) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        printfe("saveProgFieldInt: failed to open db where id=%d\n", id);
        return;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set %s=%d where id=%d", field, value, id);
    if (!db_exec(db, q, 0, 0)) {
        printfe("saveProgFieldInt: query failed: %s\n", q);
    }
    sqlite3_close(db);
}

void saveProgFieldText(int id, const char * value, const char* db_path, const char *field) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        printfe("saveProgFieldText: failed to open db where id=%d\n", id);
        return;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "update prog set %s='%s' where id=%d", field, value, id);
    if (!db_exec(db, q, 0, 0)) {
        printfe("saveProgFieldText: query failed: %s\n", q);
    }
    sqlite3_close(db);
}

int loadProg_callback(void *d, int argc, char **argv, char **azColName) {
    ProgData *data = d;
    Prog *item = (Prog *) malloc(sizeof *(item));
    if (item == NULL) {
        fputs("loadProg_callback: failed to allocate memory\n", stderr);
        return 0;
    }
    memset(item, 0, sizeof *item);
    int load = 0, enable = 0;
    for (int i = 0; i < argc; i++) {
        if (strcmp("id", azColName[i]) == 0) {
            item->id = atoi(argv[i]);
        } else if (strcmp("first_repeat_id", azColName[i]) == 0) {
            item->first_repeat_id = atoi(argv[i]);
        } else if (strcmp("peer_id", azColName[i]) == 0) {
            item->slave.peer=getPeerById(argv[i],data->peer_list);          
        } else if (strcmp("remote_id", azColName[i]) == 0) {
            item->slave.remote_id = atoi(argv[i]);
        } else if (strcmp("enable", azColName[i]) == 0) {
            enable = atoi(argv[i]);
        } else if (strcmp("load", azColName[i]) == 0) {
            load = atoi(argv[i]);
        } else {
            putse("loadProg_callback: unknown column: %s\n");
        }
    }

    if (enable) {
        item->state = INIT;
    } else {
       item->state = DISABLE;
    }

    item->next = NULL;

    if (!initMutex(&item->mutex)) {
        free(item);
        return EXIT_FAILURE;
    }
    if (!checkProg(item, data->prog_list)) {
        free(item);
        return EXIT_FAILURE;
    }
    if (!addProg(item, data->prog_list)) {
        free(item);
        return EXIT_FAILURE;
    }
    if (!load) {
        saveProgLoad(item->id, 1, data->db);
    }
    return EXIT_SUCCESS;
}

int loadRepeat_callback(void *d, int argc, char **argv, char **azColName) {
    Repeat *item = (Repeat *) d;
    memset(item, 0, sizeof *item);
    for (int i = 0; i < argc; i++) {
        if (strcmp("id", azColName[i]) == 0) {
            item->id = atoi(argv[i]);
        } else if (strcmp("first_step_id", azColName[i]) == 0) {
            item->first_step_id = atof(argv[i]);
        } else if (strcmp("count", azColName[i]) == 0) {
            item->count = atof(argv[i]);
        } else if (strcmp("next_repeat_id", azColName[i]) == 0) {
            item->next_repeat_id = atoi(argv[i]);
        } else {
#ifdef MODE_DEBUG
            fprintf(stderr, "loadRepeat_callback: unknown column: %s\n", azColName[i]);
#endif
            return EXIT_FAILURE;
        }
    }
    item->state = INIT;
    if (!checkRepeat(item)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int loadStep_callback(void *d, int argc, char **argv, char **azColName) {
    Step *item = (Step *) d;
    memset(item, 0, sizeof *item);
    for (int i = 0; i < argc; i++) {
        if (strcmp("id", azColName[i]) == 0) {
            item->id = atoi(argv[i]);
        } else if (strcmp("goal", azColName[i]) == 0) {
            item->goal = atof(argv[i]);
        } else if (strcmp("duration", azColName[i]) == 0) {
            item->duration.tv_sec = atoi(argv[i]);
            item->duration.tv_nsec = 0;
        } else if (strcmp("goal_change_mode", azColName[i]) == 0) {
            if (strcmp(CHANGE_MODE_EVEN_STR, argv[i]) == 0) {
                item->goal_change_mode = CHANGE_MODE_EVEN;
            } else if (strcmp(CHANGE_MODE_INSTANT_STR, argv[i]) == 0) {
                item->goal_change_mode = CHANGE_MODE_INSTANT;
            } else {
                item->goal_change_mode = UNKNOWN;
            }
        } else if (strcmp("stop_kind", azColName[i]) == 0) {
            if (strcmp(STOP_KIND_TIME_STR, argv[i]) == 0) {
                item->stop_kind = STOP_KIND_TIME;
            } else if (strcmp(STOP_KIND_GOAL_STR, argv[i]) == 0) {
                item->stop_kind = STOP_KIND_GOAL;
            } else {
                item->stop_kind = UNKNOWN;
            }
        } else if (strcmp("next_step_id", azColName[i]) == 0) {
            item->next_step_id = atoi(argv[i]);
        } else {
#ifdef MODE_DEBUG
            fprintf(stderr, "loadStep_callback: unknown column: %s\n", azColName[i]);
#endif
            return EXIT_FAILURE;
        }
    }
    item->state = INIT;
    if (!checkStep(item)) {
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

int addProgById(int prog_id, ProgList *list, PeerList *pl, const char *db_path) {
    Prog *rprog = getProgById(prog_id, list);
    if (rprog != NULL) {//program is already running
#ifdef MODE_DEBUG
        fprintf(stderr, "WARNING: addProgById: program with id = %d is being controlled by program\n", rprog->id);
#endif
        return 0;
    }
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    ProgData data = {db, pl, list};
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select " PROG_FIELDS " from prog where id=%d", prog_id);
    if (!db_exec(db, q, loadProg_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "addProgById: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
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
            sqlite3 *db;
            if (db_open(db_path, &db)) {
                saveProgLoad(curr->id, 0, db);
                sqlite3_close(db);
            }
            if (prev != NULL) {
                lockProg(prev);
                prev->next = curr->next;
                unlockProg(prev);
            } else {//curr=top
                lockProgList();
                list->top = curr->next;
                unlockProgList();
            }
            if (curr == list->last) {
                list->last = prev;
            }
            list->length--;
            //we will wait other threads to finish curr program and then we will free it
            lockProg(curr);
            unlockProg(curr);
            free(curr);
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

int loadActiveProg(ProgList *list, PeerList *pl, char *db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    ProgData data = {db, pl, list};
    char *q = "select " PROG_FIELDS " from prog where load=1";
    if (!db_exec(db, q, loadProg_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "loadActiveProg: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int loadAllProg(ProgList *list, PeerList *pl, char *db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    ProgData data = {db, pl, list};
    char *q = "select " PROG_FIELDS " from prog";
    if (!db_exec(db, q, loadProg_callback, (void*) &data)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "loadAllProg: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int reloadProgById(int id, ProgList *list, PeerList *pl, const char* db_path) {
    if (deleteProgById(id, list, db_path)) {
        return 1;
    }
    return addProgById(id, list, pl, db_path);
}


int getStepByIdFdb(Step *item, int id, const char *db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select id,goal,duration,goal_change_mode,stop_kind,next_step_id from step where id=%d limit 1", id);
    if (!db_exec(db, q, loadStep_callback, (void*) item)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "getStepByIdFdb: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}

int getRepeatByIdFdb(Repeat *item, int id, const char *db_path) {
    sqlite3 *db;
    if (!db_open(db_path, &db)) {
        return 0;
    }
    char q[LINE_SIZE];
    snprintf(q, sizeof q, "select id,first_step_id,count,next_repeat_id from repeat where id=%d limit 1", id);
    if (!db_exec(db, q, loadRepeat_callback, (void*) item)) {
#ifdef MODE_DEBUG
        fprintf(stderr, "getRepeatByIdFdb: query failed: %s\n", q);
#endif
        sqlite3_close(db);
        return 0;
    }
    sqlite3_close(db);
    return 1;
}
