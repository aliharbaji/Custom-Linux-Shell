#include <iostream>
#include <signal.h>
#include "signals.h"
#include "Commands.h"

using namespace std;

using namespace std;

void ctrlCHandler(int sig_num) {
    SmallShell& smash = SmallShell::getInstance();
    pid_t foreground_pid = smash.getForegroundPid();

    std::cout << "smash: got ctrl-C" << std::endl;

    if (foreground_pid > 0) {
        if (kill(foreground_pid, SIGINT) == 0) {
            std::cout << "smash: process " << foreground_pid << " was killed" << std::endl;
        } else {
            perror("smash error: kill failed");
        }
        smash.setForegroundPid(-1);
        smash.setForegroundCommand(nullptr);
    }
}