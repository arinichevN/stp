INSERT OR REPLACE INTO "peer" VALUES
('gwu18_1',49161,'127.0.0.1'),
('gwu22_1',49162,'127.0.0.1'),
('gwu18_2',49161,'127.0.0.1'),
('gwu22_2',49162,'127.0.0.1'),
('regonf_1',49191,'127.0.0.1'),
('gwu74_1',49163,'127.0.0.1'),
('lck_1',49175,'127.0.0.1'),
('lgr_1',49172,'127.0.0.1'),
('gwu59_1',49164,'127.0.0.1'),
('alp_1',49171,'127.0.0.1'),
('alr_1',49174,'127.0.0.1'),
('regsmp_1',49192,'127.0.0.1'),
('stp_1',49179,'127.0.0.1'),
('obj_1',49178,'127.0.0.1'),
('swr_1',49183,'127.0.0.1'),
('swf_1',49182,'127.0.0.1');

INSERT OR REPLACE INTO "remote_channel" (id,peer_id,channel_id) VALUES
(1,'regsmp_1',1),
(2,'regsmp_1',2),
(3,'regsmp_1',3),
(4,'regsmp_1',4);

INSERT OR REPLACE INTO "step" (id,goal,reach_duration_sec,hold_duration_sec,next_step_id) VALUES
(1,25.0,60,60,2),
(2,30.0,60,60,3),
(3,50.0,90,60,0),
(4,25.0,60,60,0),
(5,25.0,60,60,0),
(6,25.0,60,60,0),
(7,25.0,60,60,0),
(8,25.0,60,60,0);

INSERT OR REPLACE INTO "channel" (
  id,
  description,
  first_step_id,
  slave_remote_channel_id,
  sensor_remote_channel_id,
  green_light_id,
  retry_num,
  cycle_duration_sec,
  cycle_duration_nsec,
  save,
  enable,
  load) VALUES
(1,"канал1",1,1,1,1,3, 1,0, 1,1,1),
(2,"канал1",1,2,2,1,3, 1,0, 1,1,1),
(3,"канал1",1,3,3,1,3, 1,0, 1,1,1),
(4,"канал1",1,4,4,1,3, 1,0, 1,1,1);




