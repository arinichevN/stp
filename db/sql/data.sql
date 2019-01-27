CREATE TABLE "peer" (
    "id" TEXT NOT NULL,
    "port" INTEGER NOT NULL,
    "ip_addr" TEXT NOT NULL
);
CREATE TABLE "remote_channel"
(
  "id" INTEGER PRIMARY KEY,
  "peer_id" TEXT NOT NULL,
  "channel_id" INTEGER NOT NULL
);
CREATE TABLE "green_light"
(
  "id" INTEGER PRIMARY KEY,
  "remote_channel_id" INTEGER NOT NULL,--link to remote_channel table row
  "value" REAL NOT NULL
);

CREATE TABLE "step"
(
  "id" INTEGER PRIMARY KEY,
  "goal" REAL NOT NULL,
  "reach_duration_sec" INTEGER NOT NULL,
  "hold_duration_sec" INTEGER NOT NULL,
  "next_step_id" INTEGER NOT NULL --link to step table row
);

CREATE TABLE "channel"
(
  "id" INTEGER PRIMARY KEY,
  "description" TEXT,
  "first_step_id" INTEGER NOT NULL,--link to step table row
  "slave_remote_channel_id" INTEGER NOT NULL,--we will control this channel
  "sensor_remote_channel_id" INTEGER NOT NULL,--we will read value from this channel and compare it with goal when particular step is running 
  "green_light_id" INTEGER NOT NULL,--link to green_light table row
  "retry_num" INTEGER NOT NULL,--number of request repetitions to peers if previous request failed
  "cycle_duration_sec" INTEGER NOT NULL,
  "cycle_duration_nsec" INTEGER NOT NULL,
  "save" INTEGER NOT NULL,
  "enable" INTEGER NOT NULL,-- enable this channel when loaded
  "load" INTEGER NOT NULL-- load this channel into RAM
);




