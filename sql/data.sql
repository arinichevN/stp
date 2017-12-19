CREATE TABLE "prog"
(
  "id" INTEGER PRIMARY KEY,
  "description" TEXT,
  "first_repeat_id" INTEGER NOT NULL,--link to repeat table row
  "peer_id" TEXT NOT NULL,--slave data
  "remote_id" INTEGER NOT NULL,--slave data
  "check" INTEGER NOT NULL,-- 1 - send command to slave and then check result; 0 - just send command to slave
  "retry_count" INTEGER NOT NULL,-- if check enabled, then retry for certain times if check failed
  "enable" INTEGER NOT NULL,-- enable this program when loaded
  "load" INTEGER NOT NULL-- load this program into RAM
);

CREATE TABLE "repeat"
(
  "id" INTEGER PRIMARY KEY,
  "first_step_id" INTEGER NOT NULL,--link to step table row
  "count" INTEGER NOT NULL,
  "next_repeat_id" INTEGER NOT NULL--link to repeat table row
);

CREATE TABLE "step"
(
  "id" INTEGER PRIMARY KEY,
  "goal" REAL NOT NULL,
  "duration" INTEGER NOT NULL,--seconds
  "goal_change_mode" INTEGER NOT NULL, --even or instant
  "stop_kind" TEXT NOT NULL, --time or goal
  "next_step_id" INTEGER NOT NULL--link to step table row
);

