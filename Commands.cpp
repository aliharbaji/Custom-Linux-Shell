#include <unistd.h>
#include <iostream>
#include <vector>
#include <sstream>
#include <regex>
#include <iomanip>
#include "Commands.h"
#include <cstring>
#include <fcntl.h>
#include <algorithm>
#include <unistd.h>
#include <sys/syscall.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <dirent.h>
#include <pwd.h>
#include <grp.h>

using namespace std;

const std::string WHITESPACE = " \n\r\t\f\v";

#define ONE 1

#if 0
#define FUNC_ENTRY()  \
  cout << PRETTY_FUNCTION << " --> " << endl;

#define FUNC_EXIT()  \
  cout << PRETTY_FUNCTION << " <-- " << endl;
#else
#define FUNC_ENTRY()
#define FUNC_EXIT()
#endif

string _ltrim(const std::string &s) {
    size_t start = s.find_first_not_of(WHITESPACE);
    return (start == std::string::npos) ? "" : s.substr(start);
}

string _rtrim(const std::string &s) {
    size_t end = s.find_last_not_of(WHITESPACE);
    return (end == std::string::npos) ? "" : s.substr(0, end + 1);
}

string _trim(const std::string &s) {
    return _rtrim(_ltrim(s));
}

int _parseCommandLine(const char *cmd_line, char **args) {
    FUNC_ENTRY()
    int i = 0;
    std::istringstream iss(_trim(string(cmd_line)).c_str());
    for (std::string s; iss >> s;) {
        args[i] = (char *) malloc(s.length() + 1);
        memset(args[i], 0, s.length() + 1);
        strcpy(args[i], s.c_str());
        args[++i] = nullptr;
    }
    return i;

    FUNC_EXIT()
}

bool _isBackgroundComamnd(const char *cmd_line) {
    const string str(cmd_line);
    return str[str.find_last_not_of(WHITESPACE)] == '&';
}

// might want to delete
bool endsWithAmpersand(const char *cmd_line) {
    if (cmd_line == nullptr) {
        return false;
    }

    int length = strlen(cmd_line);
    if (length == 0) {
        return false;
    }

    // Start from the end of the string and move backwards
    int i = length - 1;

    // Skip any trailing whitespace characters
    while (i >= 0 && std::isspace(cmd_line[i])) {
        i--;
    }

    // If we find an ampersand, return true, otherwise false
    return (i >= 0 && cmd_line[i] == '&');
}

void _removeBackgroundSign(char *cmd_line) {
    const string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == string::npos) {
        return;
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return;
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    cmd_line[idx] = ' ';
    // truncate the command line string up to the last non-space character
    cmd_line[str.find_last_not_of(WHITESPACE, idx) + 1] = 0;
}

const char* removeBackgroundSignChar(const char* cmd_line) {
    std::string str(cmd_line);
    // find last character other than spaces
    unsigned int idx = str.find_last_not_of(WHITESPACE);
    // if all characters are spaces then return
    if (idx == std::string::npos) {
        return strdup(cmd_line);
    }
    // if the command line does not end with & then return
    if (cmd_line[idx] != '&') {
        return strdup(cmd_line);
    }
    // replace the & (background sign) with space and then remove all tailing spaces.
    str[idx] = ' ';
    // truncate the command line string up to the last non-space character
    str = str.substr(0, str.find_last_not_of(WHITESPACE) + 1);

    return strdup(str.c_str());
}


// TODO: Add your implementation for classes in Commands.h
const char* Command::getCmdName() const {
    // get the first word of the command line
    string cmd_s = _trim(string(cmd_line));
    return cmd_s.substr(0, cmd_s.find_first_of(WHITESPACE)).c_str();
}


Command::Command(const char *cmd_line, bool should_free) :
        cmd_line(cmd_line),
        expanded_cmd_line(""),
        isBackground(_isBackgroundComamnd(cmd_line)),
        isForeground(!isBackground),
        isExpanded(false),
        should_free_original(should_free),
        original_cmd_line(cmd_line)
{}



const char* Command::getExpandedCmdLine() {
    if (!isExpanded) {
        expanded_cmd_line = SmallShell::getInstance().getAliasManager().expandAlias(string(cmd_line));
        isExpanded = true;
    }
    return expanded_cmd_line.c_str();
}
BuiltInCommand::BuiltInCommand(const char *cmd_line) : Command(removeBackgroundSignChar(cmd_line), true) {}


// TODO: COMMANDS ADDED BY ALI

void freeArgs(char** args){
    for(int i = 0; args[i] != nullptr; i++){
        if(args[i] == nullptr){
            continue;
        }
        free(args[i]);
        args[i] = nullptr;
    }
}


// start of chprompt
chpromptCommand::chpromptCommand(const char *cmd_line) : BuiltInCommand(cmd_line), prompt(""){

    char* args[COMMAND_MAX_ARGS];
    // initialize args
    for(auto & arg : args){
        arg = nullptr;
    }
    // parse the command line into args
    _parseCommandLine(cmd_line, args);
    if(!args[1]){
        // set to default name 'smash'
        setPrompt("smash");
    }else{
        // set the new prompt
        setPrompt(args[1]);
    }

    // free args except for the first one
    freeArgs(args);
}

void chpromptCommand::execute() {
    SmallShell::getInstance().setName(prompt);
}
// end of chprompt

// start of showpid
ShowPidCommand::ShowPidCommand(const char* cmd_line): BuiltInCommand(cmd_line) {}
void ShowPidCommand::execute() {
    cout << "smash pid is " << getpid() << endl;
}
// end of showpid

// start of cd
cdCommand::cdCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {

}

void cdCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    // initialize args
    for(auto & arg : args) {
        arg = nullptr;
    }
    // parse the command line into args
    _parseCommandLine(this->getExpandedCmdLine(), args);

    // check if extra arguments were given
    if(args[2] != nullptr) {
        fprintf(stderr, "smash error: cd: too many arguments\n");
        freeArgs(args);
        return;
    }

    char cwd[MAX_PATH_LEN]; // might need to change
    getcwd(cwd, sizeof(cwd));

    // check if args[1] is "-"
    if(args[1] != nullptr && strcmp(args[1], "-") == 0) {
        if(SmallShell::getInstance().getLastPwd() == "") {
            fprintf(stderr, "smash error: cd: OLDPWD not set\n");
        } else {
            auto returnVal = chdir(SmallShell::getInstance().getLastPwd().c_str());
            if(returnVal == -1) {
                fprintf(stderr, "smash error: cd: ");
                perror("");  // This will print the specific error from errno
            } else {
                SmallShell::getInstance().setLastPwd(cwd);
            }
        }
    } else if(!args[1]) {
        // do nothing
    } else {
        auto returnVal = chdir(args[1]);

        // if the return value is 0, then the directory was changed successfully
        if(returnVal == 0) {
            // set lastPwd to current directory
            SmallShell::getInstance().setLastPwd(cwd);
        } else {
            fprintf(stderr, "smash error: chdir failed: ");
            perror("");  // This will print the specific error from errno
        }
    }
    freeArgs(args);
}
// end of cd


void GetCurrDirCommand::execute() {
    char cwd[MAX_PATH_LEN]; // TODO: might want to change
    if (getcwd(cwd, sizeof(cwd)) != nullptr) {
        cout << cwd << endl;
    }
//    else {
//        perror("smash error: getcwd failed"); // TODO check if this is necessary
//    }
}

// start of smallShell
SmallShell::SmallShell() {
// TODO: add your implementation
    // set name of the shell to default name 'smash'
    shellName = "smash";
    // set lastPwd to empty string
    lastPwd = "";
    // create a new jobs list
    jobs = new JobsList();


}

SmallShell::~SmallShell() {
// TODO: add your implementation
    // delete jobs list
    if(jobs) delete jobs;
    jobs = nullptr;
    delete foregroundCmd;
    foregroundCmd = nullptr;
}
// end of smallShell

/**
* Creates and returns a pointer to Command class which matches the given command line (cmd_line)
*/
Command* SmallShell::CreateCommand(const char *cmd_line) {
    string expanded_cmd = _trim(aliasManager.expandAlias(cmd_line));
    string firstWord = expanded_cmd.substr(0, expanded_cmd.find_first_of(WHITESPACE));
    const char* a_cmd = expanded_cmd.c_str();

    char* args[COMMAND_MAX_ARGS];
    for(auto & arg : args){
        arg = nullptr;
    }

    // parse the command line into args
    _parseCommandLine(a_cmd, args);

    // No command inserted
    if(args[0] == nullptr) {
        freeArgs(args);
        return nullptr;
    }

    Command* cmnd = nullptr;

    // Check for alias command first
    if (firstWord.compare("alias") == 0 || firstWord.compare("alias&") == 0 ) {
        cmnd = new AliasCommand(cmd_line, aliasManager);
    }


        // Then check for redirection
    else if (expanded_cmd.find('>') != std::string::npos) {
        cmnd = new RedirectionCommand(cmd_line);
    }

        // Then check for pipe
    else if (expanded_cmd.find('|') != std::string::npos) {
        cmnd = new PipeCommand(cmd_line);
    }

    else if (firstWord.compare("watch") == 0) {
        cmnd = new WatchCommand(cmd_line);
    }

        // Then check for the rest of the commands
    else if (firstWord.compare("pwd") == 0 || firstWord.compare("pwd&") == 0) {
        cmnd = new GetCurrDirCommand(cmd_line);
    } else if (firstWord.compare("chprompt") == 0 || firstWord.compare("chprompt&") == 0) {
        cmnd = new chpromptCommand(cmd_line);
    } else if (firstWord.compare("showpid") == 0 || firstWord.compare("showpid&") == 0) {
        cmnd = new ShowPidCommand(cmd_line);
    } else if (firstWord.compare("cd") == 0 || firstWord.compare("cd&") == 0) {
        cmnd = new cdCommand(cmd_line);
    } else if (firstWord.compare("jobs") == 0 || firstWord.compare("jobs&") == 0) {
        cmnd = new JobsCommand(cmd_line, jobs);
    } else if (firstWord.compare("fg") == 0 || firstWord.compare("fg&") == 0) {
        cmnd = new ForegroundCommand(a_cmd, jobs);
    } else if (firstWord.compare("quit") == 0 || firstWord.compare("quit&") == 0) {
        cmnd = new QuitCommand(cmd_line, jobs);
    } else if (firstWord.compare("kill") == 0) {
        cmnd = new KillCommand(cmd_line, jobs);
    } else if (firstWord.compare("listdir") == 0 || firstWord.compare("listdir&") == 0) {
        cmnd = new ListdirCommand(cmd_line);
    } else if (firstWord.compare("getuser") == 0 || firstWord.compare("getuser&") == 0) {
        cmnd = new GetUserCommand(cmd_line);
    }  else if (firstWord.compare("unalias") == 0 || firstWord.compare("unalias&") == 0) {
        cmnd = new UnaliasCommand(cmd_line, aliasManager);
    } else {
        cmnd = new ExternalCommand(cmd_line);
    }

    if (cmnd && _isBackgroundComamnd(cmd_line) && !dynamic_cast<BuiltInCommand*>(cmnd)) {
        cmnd->setBackground();
    }

    freeArgs(args);
    return cmnd;
}

/* void SmallShell::executeCommand(const char *cmd_line) {
    // TODO: Add your implementation here
    // for example:
    // Command* cmd = CreateCommand(cmd_line);
    // cmd->execute();
    // Please note that you must fork smash process for some commands (e.g., external commands....)

    // copy cmd_line to a new string
    const char* cmd_line_s = strdup(cmd_line);
    Command* cmd = CreateCommand(cmd_line_s);
    // free the memory allocated for cmd_line_s
    if(cmd_line_s) free((void*)cmd_line_s); // TODO: might be problematic
    cmd_line_s = nullptr;

    if(cmd != nullptr){
        cmd->execute();
//        cout << "COMMAND NAME: " << cmd->getCmdName() << endl;
//        cout << "[" << 69 << "] " << cmd->getCmdLine() << endl;
    }

    if(cmd && !_isBackgroundComamnd(cmd->getCmdLine())) {
        delete cmd; // this does double free to fix we
        cmd = nullptr;
    }
//    if(cmd && !endsWithAmpersand(cmd->getCmdLine())) {
//        delete cmd; // this does double free to fix we
//    }

//    delete cmd;

} */

void SmallShell::executeCommand(const char* cmd_line) {
    jobs->removeFinishedJobs();  // Remove finished jobs before executing new command

    string expanded_cmd = _trim(aliasManager.expandAlias(cmd_line));
    string firstWord = expanded_cmd.substr(0, expanded_cmd.find_first_of(WHITESPACE));
    const char* a_cmd = strdup(expanded_cmd.c_str());

    Command* cmd = CreateCommand(a_cmd);

    if (cmd != nullptr) {

        ExternalCommand* exCmd = dynamic_cast<ExternalCommand*>(cmd);
        bool isCmplx = false;
        if (exCmd){
            isCmplx = exCmd->isComplexCommand();
        }
        // Check if it's a built-in command
        BuiltInCommand* builtInCmd = dynamic_cast<BuiltInCommand*>(cmd);
        if (builtInCmd) {
            // Execute built-in commands in the main process
            builtInCmd->execute();
        } else if (!cmd->getIsBackground() || isCmplx) {
            setForegroundCommand(cmd);
            pid_t pid = fork();
            if (pid == 0) { // Child process
                setpgrp();
                cmd->execute();
                exit(0);
            } else if (pid > 0) { // Parent process
                setForegroundPid(pid);
                int status;
                waitpid(pid, &status, WUNTRACED);
                setForegroundPid(-1);
                setForegroundCommand(nullptr);
            } else {
                perror("smash error: fork failed");
            }
        } else {
            // Background command
            pid_t pid = fork();
            if (pid == 0) { // Child process
                setpgrp();
                cmd->execute();
                exit(0);
            } else if (pid > 0) { // Parent process
                jobs->addJob(pid, cmd_line);
            } else {
                perror("smash error: fork failed");
            }
        }
    }

    // Clean up
    if (cmd) {
        delete cmd;
    }

    jobs->removeFinishedJobs();  // Remove finished jobs after executing the command

    // Always print the prompt after executing a command
}

// start of JobsList
/*
void JobsList::JobEntry::printJob() const {
    cout << "[" << this->jobId << "] " << cmd->getOriginalCmdLine() << endl;
}


// ctor
JobsList::JobsList() : maxJobId(ONE) {}
// dtor
JobsList::~JobsList() {
    for(auto & job : jobs){
        delete job;
    }
    jobs.clear(); // clear the vector, TODO: check with Omar if this is necessary,
                                    // TODO: from what I know vectors are destructed when they go out of scope
}

void JobsList::updateMaxId(){
    maxJobId = ONE;
    for(unsigned int i = 0; i < jobs.size(); i++){
        if (jobs[i] && jobs[i]->getJobId() >= maxJobId){
            maxJobId = jobs[i]->getJobId() + 1;
        }
    }
}

void JobsList::addJob(Command *cmd, bool isStopped){
    // I read somewhere that there shouldn't be any allocation failures in this assignment, but I'm not sure
    JobEntry* newJob = new JobEntry(cmd, maxJobId);
    jobs.push_back(newJob);

    // update maxJobId // TODO: seems like a bad way to do this, will make it more efficient later
    updateMaxId();

    // also according to Piazza @71 isStopped is unused for this semester's assignment
}

void JobsList::printJobsList(){
//    cout << "Size of jobs list: " << jobs.size() << endl;
    for (unsigned int i = 0; i < jobs.size(); i++){
        // print job
        jobs[i]->printJob();
    }
}

void JobsList::killAllJobs() {
    for (unsigned int i = 0; i < jobs.size(); i++) {
        if(!jobs[i]) continue; // null check
        // kill job
        // TODO: implement
        cout << "KILL JOB: ";
        jobs[i]->printJob();
//      kill(jobs[i]->getJobPid(), SIGKILL); // not sure if this is the correct way to kill a job
        if(jobs[i]) delete jobs[i];
        jobs[i] = nullptr;
        cout << "-----" << endl;
    }
    updateMaxId();
}

void JobsList::removeFinishedJobs() {
    for(unsigned int i = 0; i < jobs.size(); i++) {
        if(jobs[i]->isFinished()){
            delete jobs[i];
            jobs.erase(jobs.begin() + i); // syntax for removing element from vector
        }
    }
    updateMaxId();
}

JobsList::JobEntry* JobsList::getJobById(int jobId) {
    for(unsigned int i = 0; i < jobs.size(); i++) {
        if(jobs[i]->getJobId() == jobId){
            return jobs[i];
        }
    }
    return nullptr;
}

void JobsList::removeJobById(int jobId) {
    for(unsigned int i = 0; i < jobs.size(); i++) {
        if(jobs[i]->getJobId() == jobId){
            delete jobs[i];
            jobs.erase(jobs.begin() + i); // syntax for removing element from vector
        }
    }
    updateMaxId();
}

JobsList::JobEntry* JobsList::getLastJob(int* lastJobId) {
    if(jobs.empty()){
        return nullptr;
    }
    *lastJobId = jobs.back()->getJobId();
    return jobs.back();
}

// JobsList::~JobsList() {
//     for(auto & job : jobs){
//         delete job;
//     }
//     jobs.clear(); // clear the vector, TODO: check with Omar if this is necessary,
// }
// end of JobsList

JobsList::JobEntry* JobsList::getLastStoppedJob(int* jobId) {
    // this function is not used in this semester's assignment according to Piazza @71
    return nullptr;
}

 */

// start of JobsCommand
JobsCommand::JobsCommand(const char *cmd_line, JobsList *jobs) : BuiltInCommand(cmd_line), jobs(jobs) {
    // check if jobs is nullptr
    if(jobs == nullptr){
        // create a new jobs list
        this->jobs = new JobsList();
        ::perror("smash error: JobsCommand: jobs list is nullptr"); // TODO delete later
    }
//    int jobsSize = this->jobs->getJobsListSize();
//    if(jobsSize == 0){
//        cout << "No jobs to list" << endl;
//    }
}
void JobsCommand::execute() {
    jobs->printJobsList();
}
// end of JobsCommand

// start of ForegroundCommand
void ForegroundCommand::execute() {
    SmallShell& shell = SmallShell::getInstance();
    JobsList* jobs = shell.getJobs();

    char *args[COMMAND_MAX_ARGS];
    for (auto & arg : args) {
        arg = nullptr;
    }
    _parseCommandLine(getExpandedCmdLine(), args);

    int jobId = -1;
    JobEntry* job = nullptr;

    if (!args[1]) {
        job = jobs->getLastJob(&jobId);
        if (!job) {
            std::cerr << "smash error: fg: jobs list is empty" << std::endl;
            freeArgs(args);
            return;
        }
    } else {
        jobId = atoi(args[1]);

        if (args[2] != nullptr) {
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            freeArgs(args);
            return;
        }

        if (jobId <= 0) {
            std::cerr << "smash error: fg: invalid arguments" << std::endl;
            freeArgs(args);
            return;
        }
        job = jobs->getJobById(jobId);
        if (!job) {
            std::cerr << "smash error: fg: job-id " << jobId << " does not exist" << std::endl;
            freeArgs(args);
            return;
        }

    }

    freeArgs(args);

    pid_t pid = job->getPid();
    std::cout << job->getCommandLine() << " " << pid << std::endl;

    shell.setForegroundPid(pid);
    shell.setForegroundCommand(this);

    if (job->getIsStopped()) {
        if (kill(pid, SIGCONT) < 0) {
            perror("smash error: kill failed");
            return;
        }
    }

    int status;
    pid_t wait_result = waitpid(pid, &status, WUNTRACED);

    if (wait_result == -1) {
        perror("smash error: waitpid failed");
    } else {
        if (WIFSTOPPED(status)) {
            job->setIsStopped(true);
        } else {
            jobs->removeJobById(jobId);
        }
    }

    shell.setForegroundPid(-1);
    shell.setForegroundCommand(nullptr);
}
// end of ForegroundCommand


// start of QuitCommand
void QuitCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    for(auto & arg : args) {
        arg = nullptr;
    }
    int num_args = _parseCommandLine(getExpandedCmdLine(), args);

    if (num_args >= 2 && strcmp(args[1], "kill") == 0) {
        std::cout << "smash: sending SIGKILL signal to " << jobs->size() << " jobs:" << std::endl;

        const auto& jobsToKill = jobs->getJobs();
        for (const auto& job : jobsToKill) {
            std::cout << job.getPid() << ": " << job.getCommandLine() << std::endl;
        }

        jobs->killAllJobs();
    }

    // Free allocated memory
    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }

    // Exit regardless of whether "kill" was present
    exit(0);
}

// end of QuitCommand


// start of KillCommand
void KillCommand::execute() {
    SmallShell* smash = &SmallShell::getInstance();
    char *args[COMMAND_MAX_ARGS];
    for(auto & arg : args){
        arg = nullptr;
    }
    _parseCommandLine(KillCommand::getExpandedCmdLine(), args);

    // Check for correct number of arguments and basic format
    if(args[1] == nullptr || args[2] == nullptr || args[3] != nullptr
       || args[1][0] != '-' || !isdigit(args[1][1]) || !isdigit(args[2][0])){
        fprintf(stderr, "smash error: kill: invalid arguments\n");
        freeArgs(args);
        return;
    }

    int jobId = atoi(args[2]);
    JobEntry* job = smash->getJobs()->getJobById(jobId);
    if(job == nullptr){
        fprintf(stderr, "smash error: kill: job-id %d does not exist\n", jobId);
        freeArgs(args);
        return;
    }

    int signum = atoi(args[1] + 1);
    pid_t pid = job->getPid();

    // Print the message regardless of signal validity
    cout << "signal number " << signum << " was sent to pid " << pid << endl;

    // Attempt to send the signal
    int retVal = kill(pid, signum);
    if(retVal < 0){
        // Only print an error if the kill system call fails
        perror("smash error: kill failed");
    }

    freeArgs(args);
}

ExternalCommand::ExternalCommand(const char* cmd_line) : Command(cmd_line) {}

void ExternalCommand::execute() {
    string cmd_without_amp = getExpandedCmdLine();
    cmd_without_amp = _trim(cmd_without_amp);
    if (cmd_without_amp.back() == '&') {
        cmd_without_amp.pop_back();
    }

    if (isComplexCommand()) {
        string bash_command = string("/bin/bash -c \"") + cmd_without_amp + string("\"");
        execlp("/bin/bash", "bash", "-c", bash_command.c_str(), nullptr);
    } else {
        char *args[COMMAND_MAX_ARGS];
        _parseCommandLine(cmd_without_amp.c_str(), args);
        execvp(args[0], args);
    }

    // If we get here, exec failed
    perror("smash error: execvp failed");
    exit(1);
}

void AliasCommand::execute() {
    std::string full_cmd = _trim(std::string(getOriginalCmdLine()));
    size_t cmd_start = full_cmd.find_first_not_of(WHITESPACE, 5); // 5 is the length of "alias"

    if (cmd_start == std::string::npos) {
        aliasManager.printAliases();
        return;
    }

    full_cmd = full_cmd.substr(cmd_start);
    size_t eq_pos = full_cmd.find('=');

    if (eq_pos == std::string::npos || eq_pos == 0 || eq_pos == full_cmd.length() - 1) {
        std::cerr << "smash error: alias: invalid arguments" << std::endl;
        return;
    }

    std::string alias_name = _trim(full_cmd.substr(0, eq_pos));
    std::string alias_command = _trim(full_cmd.substr(eq_pos + 1));

    if (alias_command.front() == '\'' && alias_command.back() == '\'') {
        alias_command = alias_command.substr(1, alias_command.length() - 2);
    }

    if (!std::regex_match(alias_name, std::regex("^[a-zA-Z0-9_]+$"))) {
        std::cerr << "smash error: alias: invalid alias format" << std::endl;
        return;
    }

    if (!aliasManager.addAlias(alias_name, alias_command)) {
        std::cerr << "smash error: alias: " << alias_name << " already exists or is a reserved command" << std::endl;
        return;
    }
}


bool ExternalCommand::isComplexCommand() const {
    // Check if the command line contains '*' or '?'
    return (cmd_line.find('*') != std::string::npos) || (cmd_line.find('?') != std::string::npos);
}

/*
bool ExternalCommand::isComplexCommand() const {
    return (strchr(cmd_line, '*') != nullptr) || (strchr(cmd_line, '?') != nullptr);
} */

RedirectionCommand::RedirectionCommand(const char* cmd_line) : Command(cmd_line) {
    const char* temp = removeBackgroundSignChar(cmd_line);
    std::string cmd_s(temp);
    size_t pos = cmd_s.find(">");

    command = cmd_s.substr(0, pos);
    command = _trim(command);

    append_mode = false;
    if (cmd_s[pos + 1] == '>') {
        append_mode = true;
        pos++;
    }

    output_file = cmd_s.substr(pos + 1);
    output_file = _trim(output_file);

}

void RedirectionCommand::execute() {
    int stdout_copy = dup(STDOUT_FILENO);
    if (stdout_copy == -1) {
        perror("smash error: dup failed");
        return;
    }

    int file_fd = open(output_file.c_str(),
                       O_WRONLY | O_CREAT | (append_mode ? O_APPEND : O_TRUNC),
                       0644);
    if (file_fd == -1) {
        perror("smash error: open failed");
        close(stdout_copy);
        return;
    }

    if (dup2(file_fd, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
        close(file_fd);
        close(stdout_copy);
        return;
    }

    close(file_fd);

    // Create and execute the command
    Command* cmd = SmallShell::getInstance().CreateCommand(command.c_str());
    if (cmd) {
        cmd->execute();
        delete cmd;
    }

    // Restore stdout
    if (dup2(stdout_copy, STDOUT_FILENO) == -1) {
        perror("smash error: dup2 failed");
    }
    close(stdout_copy);
}

PipeCommand::PipeCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    std::string cmd_s(cmd_line);
    size_t pipe_pos = cmd_s.find("|");

    if (pipe_pos == std::string::npos) {
        std::cerr << "smash error: PipeCommand: No pipe character found" << std::endl;
        return;
    }

    is_stderr_pipe = (cmd_s[pipe_pos + 1] == '&');

    if (is_stderr_pipe) {
        command1 = _trim(cmd_s.substr(0, pipe_pos));
        command2 = _trim(cmd_s.substr(pipe_pos + 2));
    } else {
        command1 = _trim(cmd_s.substr(0, pipe_pos));
        command2 = _trim(cmd_s.substr(pipe_pos + 1));
    }
}

void PipeCommand::execute() {
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        perror("smash error: pipe failed");
        return;
    }

    pid_t pid1 = fork();
    if (pid1 < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (pid1 == 0) {  // First child (writer)
        setpgrp();
        close(pipefd[0]);  // Close unused read end

        // Redirect stdout (or stderr) to the write end of the pipe
        dup2(pipefd[1], is_stderr_pipe ? STDERR_FILENO : STDOUT_FILENO);
        close(pipefd[1]);

        // Create and execute the first command
        Command* cmd = SmallShell::getInstance().CreateCommand(command1.c_str());
        if (cmd) {
            cmd->execute();
            delete cmd;
        }
        exit(0);
    }

    pid_t pid2 = fork();
    if (pid2 < 0) {
        perror("smash error: fork failed");
        return;
    }

    if (pid2 == 0) {  // Second child (reader)
        setpgrp();
        close(pipefd[1]);  // Close unused write end

        // Redirect stdin to the read end of the pipe
        dup2(pipefd[0], STDIN_FILENO);
        close(pipefd[0]);

        // Create and execute the second command
        Command* cmd = SmallShell::getInstance().CreateCommand(command2.c_str());
        if (cmd) {
            cmd->execute();
            delete cmd;
        }
        exit(0);
    }

    // Parent process
    close(pipefd[0]);
    close(pipefd[1]);

    // Wait for both children to finish
    waitpid(pid1, nullptr, 0);
    waitpid(pid2, nullptr, 0);
}

struct my_linux_dirent64 {
    ino64_t        d_ino;
    off64_t        d_off;
    unsigned short d_reclen;
    unsigned char  d_type;
    char           d_name[];
};

void ListdirCommand::listDirectory() {
    int fd = open(dir_path.c_str(), O_RDONLY | O_DIRECTORY);
    if (fd == -1) {
        perror("smash error: open failed");
        return;
    }

    char buf[4096];
    int nread;
    struct my_linux_dirent64 *d;
    int bpos;

    while ((nread = syscall(SYS_getdents64, fd, buf, sizeof(buf))) > 0) {
        for (bpos = 0; bpos < nread;) {
            d = (struct my_linux_dirent64 *) (buf + bpos);
            string name = d->d_name;
            if (name[0] != '.') { // Skip hidden files
                string full_path = dir_path + "/" + name;
                struct stat st;
                if (stat(full_path.c_str(), &st) != -1) {
                    if (S_ISREG(st.st_mode)) {
                        files.push_back("file: " + name);
                    } else if (S_ISDIR(st.st_mode)) {
                        directories.push_back("directory: " + name);
                    }
                }
            }
            bpos += d->d_reclen;
        }
    }

    if (nread == -1) {
        perror("smash error: getdents64 failed");
    }

    close(fd);
}

ListdirCommand::ListdirCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    std::string cmd_str(removeBackgroundSignChar(getExpandedCmdLine()));
    int num_args = _parseCommandLine(cmd_str.c_str(), args);

    if (num_args > 2) {
        std::cerr << "smash error: listdir: too many arguments" << std::endl;
        return;
    }

    dir_path = (num_args == 2) ? args[1] : ".";

    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}

void ListdirCommand::sortAndPrint() {
    std::sort(files.begin(), files.end());
    std::sort(directories.begin(), directories.end());

    for (const auto& file : files) {
        std::cout << file << std::endl;
    }
    for (const auto& dir : directories) {
        std::cout << dir << std::endl;
    }
}

void ListdirCommand::execute() {
    listDirectory();
    sortAndPrint();
}


GetUserCommand::GetUserCommand(const char* cmd_line) : BuiltInCommand(cmd_line) {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(cmd_line, args);

    if (num_args == 1) {
        target_pid = -1;
    }

    if (num_args >= 2) {
        std::cerr << "smash error: getuser: too many arguments" << std::endl;
        target_pid = -1;
    } else {
        target_pid = std::stoi(args[1]);
    }

    for (int i = 0; i < num_args; i++) {
        free(args[i]);
    }
}


#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <pwd.h>
#include <grp.h>

void GetUserCommand::execute() {
    if (target_pid == -1) {
        return;  // Error already printed in constructor
    }

    char proc_path[256];
    snprintf(proc_path, sizeof(proc_path), "/proc/%d/status", target_pid);

    int fd = open(proc_path, O_RDONLY);
    if (fd == -1) {
        std::cerr << "smash error: getuser: process " << target_pid << " does not exist" << std::endl;
        return;
    }

    char buffer[1024];
    ssize_t bytes_read = read(fd, buffer, sizeof(buffer) - 1);
    close(fd);

    if (bytes_read <= 0) {
        std::cerr << "smash error: getuser: failed to read process information" << std::endl;
        return;
    }

    buffer[bytes_read] = '\0';

    uid_t uid = 0;
    gid_t gid = 0;
    char* line = strtok(buffer, "\n");
    while (line != nullptr) {
        if (strncmp(line, "Uid:", 4) == 0) {
            sscanf(line, "Uid:\t%d", &uid);
        } else if (strncmp(line, "Gid:", 4) == 0) {
            sscanf(line, "Gid:\t%d", &gid);
            break;  // We have both UID and GID, no need to continue
        }
        line = strtok(nullptr, "\n");
    }

    struct passwd* pw = getpwuid(uid);
    struct group* gr = getgrgid(gid);

    if (pw && gr) {
        std::cout << "User: " << pw->pw_name << std::endl;
        std::cout << "Group: " << gr->gr_name << std::endl;
    } else {
        std::cerr << "smash error: getuser: failed to retrieve user or group information" << std::endl;
    }
}

WatchCommand::WatchCommand(const char* cmd_line) : BuiltInCommand(cmd_line), interval(2), should_stop(false), child_pid(-1) {
    std::string cmd_s(cmd_line);
    size_t pos = cmd_s.find("watch");
    if (pos != std::string::npos) {
        cmd_s = cmd_s.substr(pos + 5);  // Remove "watch"
    }
    cmd_s = _trim(cmd_s);

    // Check if it's a background command
    if (cmd_s.back() == '&') {
        this->setBackground();
        cmd_s.pop_back();
        cmd_s = _trim(cmd_s);
    }

    // Parse interval
    if (cmd_s[0] == '-') {
        size_t space_pos = cmd_s.find(' ');
        if (space_pos != std::string::npos) {
            interval = std::abs(std::stoi(cmd_s.substr(1, space_pos - 1)));
            command = _trim(cmd_s.substr(space_pos + 1));
        }
    } else {
        command = cmd_s;
    }
}


void WatchCommand::execute() {
    if (command.empty()) {
        std::cerr << "smash error: watch: command not specified" << std::endl;
        return;
    }

    if (interval <= 0) {
        std::cerr << "smash error: watch: invalid interval" << std::endl;
        return;
    }

    if (isBackground){

    }
    child_pid = fork();
    if (child_pid == -1) {
        perror("smash error: fork failed");
        return;
    }

    if (child_pid == 0) {  // Child process
        setpgrp();
        signal(SIGINT, SIG_DFL); // Reset signal handling for child
        while (!should_stop) {
            // Clear screen
            std::cout << "\033[2J\033[1;1H";  // ANSI escape codes to clear screen and move cursor to top-left

            Command* cmd = SmallShell::getInstance().CreateCommand(command.c_str());
            if (dynamic_cast<BuiltInCommand*>(cmd)) {
                cmd->execute();
            } else {
                pid_t exec_pid = fork();
                if (exec_pid == 0) { // Grandchild
                    cmd->execute();
                    exit(0);
                } else if (exec_pid > 0) {
                    waitpid(exec_pid, nullptr, 0);
                } else {
                    perror("smash error: fork failed");
                }
            }
            delete cmd;
            sleep(interval);
        }
        exit(0);
    } else {  // Parent process
        if (!this->getIsBackground()) {
            SmallShell::getInstance().setForegroundPid(child_pid);
            SmallShell::getInstance().setForegroundCommand(this);
            int status;
            waitpid(child_pid, &status, WUNTRACED);
            SmallShell::getInstance().setForegroundPid(-1);
            SmallShell::getInstance().setForegroundCommand(nullptr);
        } else {
            // Add to jobs list if it's a background command
            SmallShell::getInstance().getJobs()->addJob(child_pid, getOriginalCmdLine());
        }
    }
}

void WatchCommand::stop() {
    should_stop = true;
    if (child_pid != -1) {
        kill(child_pid, SIGTERM);
    }
}

UnaliasCommand::UnaliasCommand(const char *cmd_line, AliasManager &aliasManager)
        : BuiltInCommand(cmd_line), aliasManager(aliasManager) {}

void UnaliasCommand::execute() {
    char* args[COMMAND_MAX_ARGS];
    int num_args = _parseCommandLine(getExpandedCmdLine(), args);

    if (num_args < 2) {
        std::cerr << "smash error: unalias: not enough arguments" << std::endl;
        freeArgs(args);
        return;
    }

    for (int i = 1; i < num_args; i++) {
        if (!aliasManager.removeAlias(args[i])) {
            std::cerr << "smash error: unalias: " << args[i] << " alias does not exist" << std::endl;
            freeArgs(args);
            return;
        }
    }

    freeArgs(args);
}