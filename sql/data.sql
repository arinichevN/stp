CREATE TABLE "prog"
(
  "id" INTEGER PRIMARY KEY,
  "description" TEXT,
  "first_repeat_id" INTEGER NOT NULL,
  "peer_id" TEXT NOT NULL,
  "remote_id" INTEGER NOT NULL,
  "enable" INTEGER NOT NULL,
  "load" INTEGER NOT NULL
);

CREATE TABLE "repeat"
(
  "id" INTEGER PRIMARY KEY,
  "first_step_id" INTEGER NOT NULL,
  "count" INTEGER NOT NULL,
  "next_repeat_id" INTEGER NOT NULL
);

CREATE TABLE "step"
(
  "id" INTEGER PRIMARY KEY,
  "goal" REAL NOT NULL,
  "duration" INTEGER NOT NULL,
  "goal_change_mode" INTEGER NOT NULL, --even or instant
  "stop_kind" TEXT NOT NULL, --time or goal
  "next_step_id" INTEGER NOT NULL
);

