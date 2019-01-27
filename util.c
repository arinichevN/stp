
#include "main.h"
int nextStep ( Step **step, StepList *list ) {
    Step *nstep;
    LIST_GETBYID ( nstep, list, ( *step )->next_step_id )
    if ( nstep==NULL ) return 0;
    *step=nstep;
    return 1;
}

char * getStateStr ( char state ) {
    switch ( state ) {
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
    case REACH:
        return "REACH";
    case HOLD:
        return "HOLD";
    case STOP:
        return "STOP";
    case WAIT_GREEN_LIGHT:
        return "WAIT_GREEN_LIGHT";
    case UNKNOWN:
        return "UNKNOWN";
    case FAILURE:
        return "FAILURE";
    case SLAVE_ENABLE:
        return "SLAVE_ENABLE";
    case SET_GOAL:
        return "SET_GOAL";
    case GET_VALUE:
        return "GET_VALUE";
    }
    return "?";
}

int channelSetState ( Slave *item, int state ) {
    Peer *peer=&item->remote_channel.peer;
    int remote_channel_id=item->remote_channel.channel_id;
    I1List data = {.item = &remote_channel_id, .length = 1, .max_length=1};
    switch ( state ) {
    case ENABLED:
        if ( !acp_requestSendUnrequitedI1List ( ACP_CMD_CHANNEL_ENABLE, &data, peer ) ) {
            return 0;
        }
        break;
    case DISABLED:
        if ( !acp_requestSendUnrequitedI1List ( ACP_CMD_CHANNEL_DISABLE, &data, peer ) ) {
            return 0;
        }
        break;
    }
    return 1;
}

int channelCheckState ( Slave *item, int state ) {
    Peer *peer=&item->remote_channel.peer;
    int remote_channel_id=item->remote_channel.channel_id;
    ACPRequest request;
    I1List data = {.item = &remote_channel_id, .length = 1, .max_length=1};
    if ( !acp_requestSendI1List ( ACP_CMD_CHANNEL_GET_ENABLED, &data, &request, peer ) ) {
        printde ( "failed to send request where peer.id = %s and remote_id=%d\n", peer->id, remote_channel_id );
        return 0;
    }
    I2 td1;
    I2List tl1 = {&td1, 0, 1};
    if ( !acp_responseReadI2List ( &tl1, &request, peer ) ) {
        printde ( "failed to read response where peer.id = %s and remote_id=%d\n", peer->id, remote_channel_id );
        return 0;
    }
    if ( tl1.item[0].p0 != remote_channel_id ) {
        printde ( "peer returned id=%d but requested one was %d\n", tl1.item[0].p0, remote_channel_id );
        return 0;
    }
    if ( tl1.length != 1 ) {
        printde ( "number of items = %d but 1 expected\n", tl1.length );
        return 0;
    }
    switch ( state ) {
    case ENABLED:
        if ( tl1.item[0].p1 == 1 ) {
            return 1;
        }
        break;
    case DISABLED:
        if ( tl1.item[0].p1 == 0 ) {
            return 1;
        }
        break;
    }
    return 0;
}

int channelSetStateW ( Slave *item, int state ) {
    if ( !channelSetState ( item,  state ) ) {
        return WAIT;
    }
    if ( !channelCheckState ( item,  state ) ) {
        return WAIT;
    }
    return DONE;
}

int slaveSetGoal ( Slave *item, double goal ) {
    I1F1 i1f1 = {.p0 = item->remote_channel.channel_id, .p1 = goal};
    I1F1List data = {.item = &i1f1, .length = 1, .max_length=1};
    return acp_requestSendUnrequitedI1F1List ( ACP_CMD_REG_PROG_SET_GOAL, &data, &item->remote_channel.peer )  ;
}

int sensorRead ( Sensor *item ) {
    for ( int i=0; i<item->retry_num; i++ ) {
        int r= acp_getFTS ( &item->input, &item->remote_channel.peer, item->remote_channel.channel_id );
        if ( r ) return item->input.state;
    }
    return 0;
}

void progEnable ( Prog *item ) {
    item->state=INIT;
}
void progDisable ( Prog *item ) {
    item->state=DISABLE;
}
int progStop ( Prog *item ) {
    return channelSetStateW ( &item->slave, DISABLED ) ;
}

#define SINPUT sensor->input.value
int stepControl ( Step *item, Slave *slave, Sensor *sensor ) {
#ifdef MODE_DEBUG
    char *state = getStateStr ( item->state );
    struct timespec tm_rest = tonTimeRest(&item->tmr );
    printf ( "\tstep_ini: id:%d goal:%.3f reach_duration:%ld hold_duration:%ld\n", item->id, item->goal, item->reach_duration.tv_sec,item->hold_duration.tv_sec );
    printf ( "\tstep_run: state:%s value_start:%.3f goal_corr:%.3f goal_out:%.3f input:%.3f tm_rest:%ld", state, item->value_start, item->goal_correction,item->goal_out, SINPUT, tm_rest.tv_sec );

#endif
    switch ( item->state ) {
    case INIT:
        tonReset(&item->tmr);
        if ( item->reach_duration.tv_sec > 0 || item->reach_duration.tv_nsec > 0 ) {
            if ( !sensorRead ( sensor ) ) {
                item->state=FAILURE;
                printde ( "reading from sensor failed where step_id=%d\n", item->id );
                break;
            }
            item->goal_correction = 0.0;
            item->value_start = SINPUT;
            if ( item->reach_duration.tv_sec > 0 ) {
                item->goal_correction = ( item->goal - SINPUT ) / item->reach_duration.tv_sec;
            }
            if ( item->reach_duration.tv_nsec > 0 ) {
                item->goal_correction += ( item->goal - SINPUT ) / item->reach_duration.tv_nsec * NANO_FACTOR;
            }
            tonSetInterval(item->reach_duration, &item->tmr);
            item->state = REACH;
            break;
        } else if ( item->hold_duration.tv_sec > 0 || item->hold_duration.tv_nsec > 0 ) {
            tonSetInterval(item->hold_duration, &item->tmr);
            item->state = HOLD;
            break;
        }
        item->state=DONE;
        break;
    case REACH:
        if ( ton ( &item->tmr ) ) {
            if ( item->hold_duration.tv_sec > 0 || item->hold_duration.tv_nsec > 0 ) {
                tonSetInterval(item->hold_duration, &item->tmr);
                tonReset(&item->tmr);
                item->state = HOLD;
            } else {
                item->state = DONE;
            }
            break;
        }
        struct timespec dif = tonTimePassed ( &item->tmr );
        item->goal_out = item->value_start + dif.tv_sec * item->goal_correction + dif.tv_nsec * item->goal_correction * NANO_FACTOR;
        slaveSetGoal ( slave, item->goal_out );
        break;
    case HOLD:
        if ( ton ( &item->tmr ) ) {
            item->state = DONE;
            break;
        }
        item->goal_out=item->goal;
        slaveSetGoal ( slave, item->goal_out );
        break;
    case DONE:
        break;
    case FAILURE:
        break;
    case OFF:
        break;
    default:
        item->state = FAILURE;
        printde ( "unknown state where step_id=%d\n", item->id );
        break;
    }
#ifdef MODE_DEBUG
    putchar ( '\n' );
#endif
    return item->state;
}

int progControl ( Prog *item ) {
    switch ( item->state ) {
    case INIT:
        if ( item->step_list.length>0 ) {
            item->step=&item->step_list.item[0];
            item->step->state=INIT;
            item->state=WAIT_GREEN_LIGHT;
        } else {
            item->state=OFF;
        }
        break;
    case WAIT_GREEN_LIGHT:
        if ( greenLight_isGreen ( &item->green_light ) ) {
            item->state=SLAVE_ENABLE;
        }
        break;
    case SLAVE_ENABLE:
        switch ( channelSetStateW ( &item->slave, ENABLED ) ) {
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
        switch ( stepControl ( item->step, &item->slave, &item->sensor ) ) {
        case DONE:
            item->state = NSTEP;
            break;
        case FAILURE:
            item->state = DISABLE;
            break;
        default :
            break;
        }
        break;
    case NSTEP:
        if ( !nextStep ( &item->step, &item->step_list ) ) {
            item->state=DISABLE;
            break;
        }
        item->step->state=INIT;
        item->state=RUN;
        break;
    case DISABLE:
        switch ( progStop ( item ) ) {
        case DONE:
            item->state = OFF;
            break;
        case WAIT:
            break;
        case FAILURE:
            item->state = OFF;
            break;
        }
        break;
    case OFF:
        break;
    default:
        break;
    }
    return item->state;
}

void freeChannel ( Channel*item ) {
    freeSocketFd ( &item->sock_fd );
    freeMutex ( &item->mutex );
    free ( item );
}

void freeChannelList ( ChannelLList *list ) {
    Channel *item = list->top;
    while ( item != NULL ) {
        Channel *temp = item;
        item = item->next;
        freeChannel ( temp );
    }
    list->top = NULL;
    list->last = NULL;
    list->length = 0;
}

void secure ( ChannelLList *list ) {
    FOREACH_LLIST ( item,list, Channel ) {
        for ( int i=0; i<SECURE_TRY_NUM; i++ ) channelSetState ( &item->prog.slave, DISABLED );
    }
}
int checkStep ( const Step *item ) {
    int success=1;
    if ( item->reach_duration.tv_sec < 0 || item->reach_duration.tv_nsec < 0 ) {
        fprintf ( stderr, "%s(): bad reach_duration where id = %d\n", F, item->id );
        success= 0;
    }
    if ( item->hold_duration.tv_sec < 0 || item->hold_duration.tv_nsec < 0 ) {
        fprintf ( stderr, "%s(): bad hold_duration where id = %d\n", F, item->id );
        success= 0;
    }
    return success;
}

int checkChannelStepList ( const StepList *list ) {
    int success=1;
    FORLi {
        if ( !checkStep ( &LIi ) ) success=0;
    }
    //step list items link to each other
    FORLISTP ( list, i ) {
        if ( i>=list->length-1 ) break;
        if ( list->item[i].next_step_id!=list->item[i+1].id ) {
            printde ( "step list is not linked step_id=%d\n",list->item[i].id );
            success=0;
        }
    }

    return success;
}

int checkChannel ( const Channel *item ) {
    int success=1;
    if ( item->cycle_duration.tv_sec < 0 ) {
        fprintf ( stderr, "%s(): bad cycle_duration_sec where id = %d\n", F, item->id );
        success= 0;
    }
    if ( item->cycle_duration.tv_nsec < 0 ) {
        fprintf ( stderr, "%s(): bad cycle_duration_nsec where id = %d\n", F, item->id );
        success= 0;
    }
    if ( !checkChannelStepList ( &item->prog.step_list ) ) {
        success=0;
    }
    return success;
}

int bufCatProgStepInfo ( Channel *item, ACPResponse *response ) {
    if ( lockMutex ( &item->mutex ) ) {
        char q[LINE_SIZE];
        if ( item->prog.step==NULL ) {
            char *state = "UNDEF";
            struct timespec tm_rest = {0L, 0L};
            snprintf ( q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_ROW_STR,
                       item->id,
                       0,
                       0.0,
                       0L,
                       0L,
                       0L,
                       0L,
                       0,
                       
                       state,
                       0.0,
                       0.0,
                       tm_rest.tv_sec,
                       tm_rest.tv_nsec
                     );
        } else {
            char *state = getStateStr ( item->prog.step->state );
            struct timespec tm_rest = tonTimeRest ( &item->prog.step->tmr );
            snprintf ( q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR FLOAT_NUM ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_COLUMN_STR "%ld" ACP_DELIMITER_ROW_STR,
                       item->id,
                       item->prog.step->id,
                       item->prog.step->goal,
                       item->prog.step->reach_duration.tv_sec,
                       item->prog.step->reach_duration.tv_nsec,
                       item->prog.step->hold_duration.tv_sec,
                       item->prog.step->hold_duration.tv_nsec,
                       item->prog.step->next_step_id,
                       
                       state,
                       item->prog.step->value_start,
                       item->prog.step->goal_correction,
                       tm_rest.tv_sec,
                       tm_rest.tv_nsec
                     );
        }
        unlockMutex ( &item->mutex );
        return acp_responseStrCat ( response, q );
    }
    return 0;
}

int bufCatProgInfo ( Channel *item, ACPResponse *response ) {
    if ( lockMutex ( &item->mutex ) ) {
        char q[LINE_SIZE];
            char *state = getStateStr ( item->prog.state );
            snprintf ( q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%s" ACP_DELIMITER_ROW_STR,
                       item->id,
                       state
                     );
        unlockMutex ( &item->mutex );
        return acp_responseStrCat ( response, q );
    }
    return 0;
}

int bufCatProgError ( Channel *item, ACPResponse *response ) {
    char q[LINE_SIZE];
    snprintf ( q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%u" ACP_DELIMITER_ROW_STR, item->id, item->error_code );
    return acp_responseStrCat ( response, q );
}

int bufCatProgFTS ( Channel *item, ACPResponse *response ) {
    if ( lockMutex ( &item->mutex ) ) {
        int r = acp_responseFTSCat ( item->id, item->prog.sensor.input.value, item->prog.sensor.input.tm, item->prog.sensor.input.state, response );
        unlockMutex ( &item->mutex );
        return r;
    }
    return 0;
}

int bufCatProgEnabled ( Channel *item, ACPResponse *response ) {
    if ( lockMutex ( &item->mutex ) ) {
        char q[LINE_SIZE];
        int enabled;
        switch ( item->prog.state ) {
        case OFF:
            enabled=0;
            break;
        default:
            enabled=1;
            break;
        }
        snprintf ( q, sizeof q, "%d" ACP_DELIMITER_COLUMN_STR "%d" ACP_DELIMITER_ROW_STR,
                   item->id,
                   enabled
                 );
        unlockMutex ( &item->mutex );
        return acp_responseStrCat ( response, q );
    }
    return 0;
}

void printData ( ACPResponse *response ) {
    char q[LINE_SIZE];
    snprintf ( q, sizeof q, "CONFIG_FILE: %s\n", CONFIG_FILE );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "port: %d\n", sock_port );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "db_path: %s\n", db_path );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "app_state: %s\n", getAppState ( app_state ) );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "PID: %d\n", getpid() );
    SEND_STR ( q )
    SEND_STR ( "+-----------------------------------------------+\n" )
    SEND_STR ( "|                     channel                   |\n" )
    SEND_STR ( "+-----------+-----------------------------------+\n" )
    SEND_STR ( "|           |               slave               |\n" )
    SEND_STR ( "|           +-----------+-----------+-----------+\n" )
    SEND_STR ( "|     id    |rchannel_id| remote_id |  peer_id  |\n" )
    SEND_STR ( "+-----------+-----------+-----------+-----------+\n" )
    FOREACH_CHANNEL {
        snprintf ( q, sizeof q, "|%11d|%11d|%11d|%11s|\n",
        item->id,
        item->prog.slave.remote_channel.id,
        item->prog.slave.remote_channel.channel_id,
        item->prog.slave.remote_channel.peer.id
                 );
        SEND_STR ( q )
    }
    SEND_STR ( "+-----------+-----------+-----------+-----------+\n" )

    SEND_STR ( "+-----------------------------------------------------------------------------------------------+\n" )
    SEND_STR ( "|                                         channel                                               |\n" )
    SEND_STR ( "+-----------+-----------------------------------------------------------------------------------+\n" )
    SEND_STR ( "|           |                                      sensor                                       |\n" )
    SEND_STR ( "|           +-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n" )
    SEND_STR ( "|     id    |rchannel_id| remote_id |  peer_id  |   value   |    sec    |    nsec   |   state   |\n" )
    SEND_STR ( "+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n" )
    FOREACH_CHANNEL {
        snprintf ( q, sizeof q, "|%11d|%11d|%11d|%11s|%11.3f|%11ld|%11ld|%11d|\n",
        item->id,
        item->prog.sensor.remote_channel.id,
        item->prog.sensor.remote_channel.channel_id,
        item->prog.sensor.remote_channel.peer.id,
        item->prog.sensor.input.value,
        item->prog.sensor.input.tm.tv_sec,
        item->prog.sensor.input.tm.tv_nsec,
        item->prog.sensor.input.state
                 );
        SEND_STR ( q )
    }
    SEND_STR ( "+-----------+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n" )

    SEND_STR ( "+-----------------------------------------------------------------------+\n" )
    SEND_STR ( "|                               channel                                 |\n" )
    SEND_STR ( "+-----------+-----------------------------------------------------------+\n" )
    SEND_STR ( "|           |                         step                              |\n" )
    SEND_STR ( "|           +-----------+-----------+-----------+-----------+-----------+\n" )
    SEND_STR ( "|     id    |     id    |   goal    |reach_durat|hold_durati|  nstep_id |\n" )
    SEND_STR ( "+-----------+-----------+-----------+-----------+-----------+-----------+\n" )
    FOREACH_CHANNEL {
        FORLISTN ( item->prog.step_list, j ) {
            snprintf ( q, sizeof q, "|%11d|%11d|%11.3f|%11ld|%11ld|%11d|\n",
            item->id,
            item->prog.step_list.item[j].id,
            item->prog.step_list.item[j].goal,
            item->prog.step_list.item[j].reach_duration.tv_sec,
            item->prog.step_list.item[j].hold_duration.tv_sec,
            item->prog.step_list.item[j].next_step_id
                     );
            SEND_STR ( q )
        }
    }
    SEND_STR ( "+-----------+-----------+-----------+-----------+-----------+-----------+\n" )

    SEND_STR ( "+-----------------------------------------------------------------------------------+\n" )
    SEND_STR ( "|                                   channel                                         |\n" )
    SEND_STR ( "+-----------+-----------------------------------------------------------------------+\n" )
    SEND_STR ( "|           |                          active step                                  |\n" )
    SEND_STR ( "|           +-----------+-----------+-----------+-----------+-----------+-----------+\n" )
    SEND_STR ( "|     id    |     id    |   state   |goal_correc|value_start|  goal_out | time_rest |\n" )
    SEND_STR ( "+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n" )
    FOREACH_CHANNEL {
        if ( item->prog.step==NULL ) {
            SEND_STR ( "|           |           |           |           |           |           |           |           |\n" )
        } else{
            char *state=getStateStr ( item->prog.step->state );
            struct timespec tm_rest= tonTimeRest ( &item->prog.step->tmr );
            snprintf ( q, sizeof q, "|%11d|%11d|%11s|%11.3f|%11.3f|%11.3f|%11ld|\n",
            item->id,
            item->prog.step->id,
            state,
            item->prog.step->goal_correction,
            item->prog.step->value_start,
            item->prog.step->goal_out,
            tm_rest.tv_sec
                     );

            SEND_STR ( q )
        }
    }
    SEND_STR ( "+-----------+-----------+-----------+-----------+-----------+-----------+-----------+\n" )

    SEND_STR ( "+-----------------------------------------------+\n" )
    SEND_STR ( "|                     channel                   |\n" )
    SEND_STR ( "+-----------+-----------------------------------+\n" )
    SEND_STR ( "|           |            green light            |\n" )
    SEND_STR ( "|           +-----------+-----------+-----------+\n" )
    SEND_STR ( "|     id    |     id    |   value   |   active  |\n" )
    SEND_STR ( "+-----------+-----------+-----------+-----------+\n" )
    FOREACH_CHANNEL {
        snprintf ( q, sizeof q, "|%11d|%11d|%11.3f|%11d|\n",
        item->id,
        item->prog.green_light.id,
        item->prog.green_light.value,
        item->prog.green_light.active
                 );
        SEND_STR ( q )
    }
    SEND_STR_L ( "+-----------+-----------+-----------+-----------+\n" )


}

void printHelp ( ACPResponse *response ) {
    char q[LINE_SIZE];
    SEND_STR ( "COMMAND LIST\n" )
    snprintf ( q, sizeof q, "%s\tput process into active mode; process will read configuration\n", ACP_CMD_APP_START );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tput process into standby mode; all running programs will be stopped\n", ACP_CMD_APP_STOP );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tfirst stop and then start process\n", ACP_CMD_APP_RESET );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tterminate process\n", ACP_CMD_APP_EXIT );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget state of process; response: B - process is in active mode, I - process is in standby mode\n", ACP_CMD_APP_PING );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget some variable's values; response will be packed into multiple packets\n", ACP_CMD_APP_PRINT );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget this help; response will be packed into multiple packets\n", ACP_CMD_APP_HELP );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tload channel into RAM and start its execution; channel id expected\n", ACP_CMD_CHANNEL_START );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tunload channel from RAM; channel id expected\n", ACP_CMD_CHANNEL_STOP );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tunload channel from RAM, after that load it; channel id expected\n", ACP_CMD_CHANNEL_RESET );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tenable running channel; channel id expected\n", ACP_CMD_CHANNEL_ENABLE );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tdisable running channel; channel id expected\n", ACP_CMD_CHANNEL_DISABLE );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget channel state (1-enabled, 0-disabled); channel id expected\n", ACP_CMD_CHANNEL_GET_ENABLED );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tsave data to database or not; channel id and value (1 || 0) expected\n", ACP_CMD_CHANNEL_SET_SAVE );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget channel error code; channel id expected\n", ACP_CMD_CHANNEL_GET_ERROR );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget channel info; channel id expected\n", ACP_CMD_CHANNEL_GET_INFO );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget channel sensor value; channel id expected\n", ACP_CMD_GET_FTS );
    SEND_STR ( q )
    snprintf ( q, sizeof q, "%s\tget channel active step info; channel id expected\n", ACP_CMD_STEPPED_SETTER_ACTIVE_STEP_GET_INFO );
    SEND_STR_L ( q )
}
