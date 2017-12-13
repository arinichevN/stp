delete from prog;
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,enable,load) VALUES 
(1,'регулятор1_канал1','regsmp_1',1,1,1,1);
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,enable,load) VALUES 
(2,'регулятор1_канал2','regsmp_1',1,2,1,1);
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,enable,load) VALUES 
(3,'регулятор1_канал3','regsmp_1',1,3,1,1);
INSERT INTO prog(id,description,peer_id,first_repeat_id,remote_id,enable,load) VALUES 
(4,'регулятор1_канал4','regsmp_1',1,4,1,1);

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




