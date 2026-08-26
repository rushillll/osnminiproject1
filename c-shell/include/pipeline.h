#ifndef PIPELINE_H
#define PIPELINE_H

#include "command.h"

void run_pipeline(Pipeline* pipeline, const char* home, char** previous_cwd);

#endif