delete from "peer";
INSERT INTO "peer" VALUES('gwu18_1',49161,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu22_1',49162,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu18_2',49161,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu22_2',49162,'127.0.0.1');
INSERT INTO "peer" VALUES('regonf_1',49191,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu74_1',49163,'127.0.0.1');
INSERT INTO "peer" VALUES('lck_1',49175,'127.0.0.1');
INSERT INTO "peer" VALUES('lgr_1',49172,'127.0.0.1');
INSERT INTO "peer" VALUES('gwu59_1',49164,'127.0.0.1');
INSERT INTO "peer" VALUES('alp_1',49171,'127.0.0.1');
INSERT INTO "peer" VALUES('alr_1',49174,'127.0.0.1');
INSERT INTO "peer" VALUES('regsmp_1',49192,'127.0.0.1');
INSERT INTO "peer" VALUES('stp_1',49179,'127.0.0.1');
INSERT INTO "peer" VALUES('obj_1',49178,'127.0.0.1');
INSERT INTO "peer" VALUES('swr_1',49183,'127.0.0.1');
INSERT INTO "peer" VALUES('swf_1',49182,'127.0.0.1');

delete from prog;
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,'check',retry_count,enable,load) VALUES 
(1,'регулятор1_канал1','regsmp_1',1,1,1,1,1,1);
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,'check',retry_count,enable,load) VALUES 
(2,'регулятор1_канал2','regsmp_1',1,2,1,1,1,1);
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,'check',retry_count,enable,load) VALUES 
(3,'регулятор1_канал3','regsmp_1',1,3,1,1,1,1);
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,'check',retry_count,enable,load) VALUES 
(4,'регулятор1_канал4','regsmp_1',1,4,1,1,1,1);

delete from 'repeat';
INSERT INTO 'repeat'(id,first_step_id,count,next_repeat_id) VALUES 
(1,1,1,2);

delete from step;
INSERT INTO step(id,goal,duration,goal_change_mode, stop_kind, next_step_id) VALUES 
(1,30.0,10,'instant','time', 2);
INSERT INTO step(id,goal,duration,goal_change_mode, stop_kind, next_step_id) VALUES 
(2,40.0,10,'instant','goal', 3);
INSERT INTO step(id,goal,duration,goal_change_mode, stop_kind, next_step_id) VALUES 
(3,60.0,10,'instant','time', 4);
INSERT INTO step(id,goal,duration,goal_change_mode, stop_kind, next_step_id) VALUES 
(4,-10.0,120,'even','time', 5);




