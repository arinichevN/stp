delete from em_mapping;
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (1,'gwu74_1',8,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (2,'gwu74_1',7,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (3,'gwu74_1',6,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (4,'gwu74_1',5,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (5,'gwu74_1',4,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (6,'gwu74_1',3,1000);

INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (7,'gwu74_1',2,1000);
INSERT INTO em_mapping (em_id,peer_id,remote_id,pwm_rsl) VALUES (8,'gwu74_1',1,1000);

delete from sensor_mapping;
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (1,'gwu18_1',1);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (2,'gwu18_1',2);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (3,'gwu18_1',3);
INSERT INTO sensor_mapping (sensor_id,peer_id,remote_id) VALUES (4,'gwu18_1',4);


delete from prog;
INSERT INTO prog(id,description,first_step_r_id,enable,load) VALUES 
(1,'программа1',1,1,1);
INSERT INTO prog(id,description,first_step_r_id,enable,load) VALUES 
(2,'программа2',1,1,1);
INSERT INTO prog(id,description,first_step_r_id,enable,load) VALUES 
(3,'программа3',1,1,1);

delete from step_r;
INSERT INTO step_r(id,description,first_step_id,repeat_count,repeat_infinite,next_step_r_id) VALUES 
(1,'повтор1',1,1,0,-1);

delete from step;
INSERT INTO step(id,description,prog_reg_id,prog_sem_id,goal,duration,goal_change_mode,stop_kind,next_step_id) VALUES 
(1,'держать25',1,1,25,300,'instant','time',2);
INSERT INTO step(id,description,prog_reg_id,prog_sem_id,goal,duration,goal_change_mode,stop_kind,next_step_id) VALUES 
(2,'сделать40',1,1,40,300,'instant','goal',1);

delete from reg;
INSERT INTO reg(id,description,em_mode,mode,delta,kp,ki,kd) VALUES 
(1,'повтор1','heater','pid',0.5,0.2,0.1,0.1);

delete from sem;
INSERT INTO sem(id,description,sensor_id,em_id) VALUES 
(1,'бокс1',1,1);
INSERT INTO sem(id,description,sensor_id,em_id) VALUES 
(2,'бокс2',2,2);
INSERT INTO sem(id,description,sensor_id,em_id) VALUES 
(3,'бокс3',3,3);
INSERT INTO sem(id,description,sensor_id,em_id) VALUES 
(4,'бокс4',4,4);

delete from reg_mapping;
INSERT INTO reg_mapping (prog_id,reg_id,prog_reg_id) VALUES (1,1,1);
INSERT INTO reg_mapping (prog_id,reg_id,prog_reg_id) VALUES (2,1,1);
INSERT INTO reg_mapping (prog_id,reg_id,prog_reg_id) VALUES (3,1,1);

delete from sem_mapping;
INSERT INTO sem_mapping (prog_id,sem_id,prog_sem_id) VALUES (1,1,1);
INSERT INTO sem_mapping (prog_id,sem_id,prog_sem_id) VALUES (2,2,1);
INSERT INTO sem_mapping (prog_id,sem_id,prog_sem_id) VALUES (3,3,1);



