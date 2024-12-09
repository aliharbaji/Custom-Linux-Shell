#ifndef SMASH_COMMAND_H_
#define SMASH_COMMAND_H_

#include <vector>

#include <signal.h>

#include <string.h>
#include <map>
#include <set>
#include <vector>
#include "Commands.h"
#include <sstream>
#include <sys/wait.h>


#define COMMAND_MAX_LENGTH (200)
#define COMMAND_MAX_ARGS (20)
#define MAX_PATH_LEN (260)

using namespace std;




class AliasManager {
private:
    std::map<std::string, std::string> aliases;
    std::vector<std::pair<std::string, std::string>> aliasList;
    const std::set<std::string> reservedKeywords = {
            "chprompt", "showpid", "pwd", "cd", "jobs", "fg", "quit", "kill", "alias", "unalias",
            "listdir", "getuser", ">", "<", "|", ">>", "<<", "watch", "|&",
            "chprompt&", "showpid&", "pwd&", "cd&", "jobs&", "fg&", "quit&", "kill&", "alias&", "unalias&",
            "listdir&", "getuser&"

    };

public:
    bool addAlias(const std::string alias, const std::string command) {
        if (reservedKeywords.find(alias) != reservedKeywords.end()) {
            return false;
        }
        if (aliases.find(alias) != aliases.end()) {
            return false;
        }
        aliases[alias] = command;
        aliasList.emplace_back(alias, command);
        return true;
    }

    std::string getCommand(const std::string& alias, bool& isFound) const {
        auto it = aliases.find(alias);
        if (it != aliases.end()) {
            isFound = true;
            return it->second;
        }
        isFound = false;
        return alias;
    }

    bool removeAlias(const std::string& alias) {
        auto i = aliases.find(alias);
        if (i == aliases.end()) {
            return false;
        }
        aliases.erase(i);

        for (auto it = aliasList.begin(); it != aliasList.end(); ++it) {
            if (it->first == alias) {
                aliasList.erase(it);
                return true;
            }
        }
        return true;
    }

    void printAliases() const {
        if (aliasList.empty()) {
            return;
        }
        for (const auto& pair : aliasList) {
            std::cout << pair.first << "='" << pair.second << "'" << std::endl;
        }
    }

    std::string expandAlias(const std::string& command) {
        std::istringstream iss(command);
        std::string firstWord;
        iss >> firstWord;

        auto it = aliases.find(firstWord);
        if (it != aliases.end()) {
            // Replace the first word with its alias
            std::string remaining;
            std::getline(iss, remaining);
            return it->second + remaining;
        }
        return command;
    }






}
;

class Command {
// TODO: Add your data members
protected:

    const string cmd_line;
    string expanded_cmd_line;
    bool should_free_original;
    const char* original_cmd_line;

    // Ali's data members
    bool isBackground;
    bool isForeground;
    bool isExpanded;


public:
    Command(const char *cmd_line, bool should_free = false);

    virtual void execute() = 0;
    //virtual void prepare();
    //virtual void cleanup();
    // TODO: Add your extra methods if needed

    // Ali's methods
    const char* getCmdName() const;
    const char* getExpandedCmdLine();
    const char* getOriginalCmdLine() const { return cmd_line.c_str(); }

    bool getIsBackground() { return isBackground; }
    bool getIsForeground() { return isForeground; }

    void setBackground() { isBackground = true; isForeground = false; }
    void setForeground() { isForeground = true; isBackground = false; }

};

class BuiltInCommand : public Command {
//    const char* cmd_line;
public:
    BuiltInCommand(const char *cmd_line);

    virtual ~BuiltInCommand() {}
};

class ExternalCommand : public Command {
private:

    const char *aliased_cmd;
    bool isAliased;

public:
    ExternalCommand(const char *cmd_line);

    bool isComplexCommand() const;

    virtual ~ExternalCommand() {}

    void execute() override;
};


class ChangeDirCommand : public BuiltInCommand {
// TODO: Add your data members public:
    ChangeDirCommand(const char *cmd_line, char **plastPwd);

    virtual ~ChangeDirCommand() {}

    void execute() override;
};

class GetCurrDirCommand : public BuiltInCommand {
public:
    GetCurrDirCommand(const char *cmd_line): BuiltInCommand(cmd_line) {}

    virtual ~GetCurrDirCommand() = default;

    void execute() override;
};

class ShowPidCommand : public BuiltInCommand {
public:
    ShowPidCommand(const char *cmd_line);

    virtual ~ShowPidCommand() = default;

    void execute() override;
};

class JobsList;

class QuitCommand : public BuiltInCommand {
// TODO: Add your data members
private:
    JobsList* jobs;
public:
    QuitCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~QuitCommand() {
        // if(jobs) delete jobs;
        // jobs = nullptr;
    }

    void execute() override;
};

/*
class JobsList {
public:
    class JobEntry {
        // TODO: Add your data members
    public:
        JobEntry(Command* cmd, int jobId) : cmd(cmd), isZombie(false), isBackground(false), isForeground(true), jobId(jobId), jobPid(0) // TODO: check if time(nullptr) is correct
        {
            // jobPid = dynamic_cast<ExternalCommand*>(cmd)->getPid(); ? dynamic_cast<ExternalCommand*>(cmd)->getPid() : 0; // will use later
            // job pid will be set when the command is executed
        }

        ~JobEntry() {
            if(cmd) delete cmd;
            cmd = nullptr;
        }

        void printJob() const;

        Command* getCmd() const { return cmd; }

        bool isFinished() const { return isZombie; }

        void setFinished() { isZombie = true; }

        int getJobId() const { return jobId; }

        int getJobPid() const {
            // how to get the pid of the command?
            return 0;
        }

        bool isItBackground() const { return isBackground; }
        bool isItForeground() const { return isForeground; }



    private:
        Command* cmd;
        bool isZombie; // or hasFinished
        bool isBackground;
        bool isForeground;
        int jobId;
        int jobPid; // I am having a hard time figuring out how to maintain the pid of the command
//        double timeStarted; // TODO: check if time(nullptr) is correct, maybe this is not needed
    };
    // TODO: Add your data members
private:
    vector<JobEntry*> jobs;
    int maxJobId;

    void updateMaxId();
public:
    JobsList();

    ~JobsList();

    void addJob(Command *cmd, bool isStopped = false);

    void printJobsList();

    void killAllJobs();

    void removeFinishedJobs();

    JobEntry *getJobById(int jobId);

    void removeJobById(int jobId);

    JobEntry *getLastJob(int *lastJobId);

    JobEntry *getLastStoppedJob(int *jobId);
    // TODO: Add extra methods or modify exisitng ones as needed

    int getMaxJobId() const { return maxJobId; }

    int getJobsListSize() const { return jobs.size(); }

    vector<JobEntry*> getJobs() const { return jobs; }

    void clearJobs() { jobs.clear();}
}; */

class JobsCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *jobs;
public:
    JobsCommand(const char *cmd_line, JobsList *jobs);

    virtual ~JobsCommand() {}

    void execute() override;

};

class KillCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *jobs;
public:
    KillCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~KillCommand() {}

    void execute() override;
};

class ForegroundCommand : public BuiltInCommand {
    // TODO: Add your data members
    JobsList *jobs;
public:
    ForegroundCommand(const char *cmd_line, JobsList *jobs): BuiltInCommand(cmd_line), jobs(jobs) {}

    virtual ~ForegroundCommand() {
        // TODO: implement if needed

    }

    void execute() override;

    JobsList *getJobs() { return jobs; }
};


class aliasCommand : public BuiltInCommand {
public:
    aliasCommand(const char *cmd_line);

    virtual ~aliasCommand() {}

    void execute() override;
};

class unaliasCommand : public BuiltInCommand {
public:
    unaliasCommand(const char *cmd_line);

    virtual ~unaliasCommand() {}

    void execute() override;
};

// TODO: COMMANDS ADDED BY ALI
class chpromptCommand : public BuiltInCommand {
private:
    std::string prompt;
public:
    chpromptCommand(const char *cmd_line);

    void execute() override;

    void setPrompt(const char* newPrompt) {
        prompt = newPrompt;
    }
};

class cdCommand : public BuiltInCommand {
public:
    cdCommand(const char* cmd_line);

    virtual ~cdCommand() = default;

    void execute() override;
};


class SmallShell {
private:
    // TODO: Add your data members
    SmallShell();
    AliasManager aliasManager;
    string shellName;
    string lastPwd;
    pid_t foreground_pid;
    Command* foreground_cmd;

    JobsList *jobs;

    Command* foregroundCmd;
public:
    Command *CreateCommand(const char *cmd_line);

    SmallShell(SmallShell const &) = delete; // disable copy ctor
    void operator=(SmallShell const &) = delete; // disable = operator
    static SmallShell &getInstance() // make SmallShell singleton
    {
        static SmallShell instance; // Guaranteed to be destroyed.
        // Instantiated on first use.
        return instance;
    }

    pid_t getForegroundPid() const { return foreground_pid; }
    void setForegroundPid(pid_t pid) { foreground_pid = pid; }
    Command* getForegroundCommand() const { return foreground_cmd; }
    void setForegroundCommand(Command* cmd) { foreground_cmd = cmd; }

    ~SmallShell();

    void executeCommand(const char *cmd_line);

    // TODO: add extra methods as needed
    void setName(const std::string new_name) { shellName = new_name; }

    const string getName() { return shellName; }

    const string getLastPwd() { return lastPwd; }

    void setLastPwd(const string newPwd) { lastPwd = newPwd; }

    AliasManager getAliasManager() { return aliasManager; }

    JobsList *getJobs() { return jobs; }

//    int getJobsListSize() { return jobs->getJobsListSize(); }
};

class AliasCommand : public BuiltInCommand {
private:
    AliasManager &aliasManager;

public:
    AliasCommand(const char *cmd_line, AliasManager &aliasManager)
            : BuiltInCommand(cmd_line), aliasManager(aliasManager) {}
    void execute() override;

};

class RedirectionCommand : public Command {
private:
    std::string command;
    std::string output_file;
    bool append_mode;

public:
    RedirectionCommand(const char* cmd_line);
    void execute() override;
};

class PipeCommand : public BuiltInCommand {
private:
    std::string command1;
    std::string command2;
    bool is_stderr_pipe;

public:
    PipeCommand(const char* cmd_line);
    void execute() override;
};

class ListdirCommand : public BuiltInCommand {
private:
    std::string dir_path;
    std::vector<std::string> files;
    std::vector<std::string> directories;

public:
    ListdirCommand(const char* cmd_line);
    void listDirectory();
    void execute() override;
    void sortAndPrint();
};

class GetUserCommand : public BuiltInCommand {
private:
    pid_t target_pid;

public:
    GetUserCommand(const char* cmd_line);
    void execute() override;
};

class WatchCommand : public BuiltInCommand {
private:
    int interval;
    std::string command;
    bool should_stop;
    pid_t child_pid;

public:
    WatchCommand(const char* cmd_line);
    virtual ~WatchCommand() {}
    void execute() override;
    void stop();
};


class UnaliasCommand : public BuiltInCommand {
private:
    AliasManager &aliasManager;
public:
    UnaliasCommand(const char *cmd_line, AliasManager &aliasManager);
    virtual ~UnaliasCommand() {}
    void execute() override;
};


class JobEntry {
private:
    int jobId;
    pid_t pid;
    string commandLine;
    bool isStopped;

public: //TODO: check strdup
    JobEntry(int jobId, pid_t pid, const string commandLine, bool isStopped = false)
            : jobId(jobId), pid(pid), commandLine(commandLine), isStopped(isStopped) {}

    int getJobId() const { return jobId; }
    pid_t getPid() const { return pid; }
    const char* getCommandLine() const { return commandLine.c_str(); }
    bool getIsStopped() const { return isStopped; }
    void setIsStopped(bool stopped) { isStopped = stopped; }


    //assignment, copy ct'or and destructor


};

class JobsList {
private:
    std::vector<JobEntry> jobs;
    int maxJobId;

    // Helper function to check if a job is finished
    static bool isJobFinished(const JobEntry& job) {
        int status;
        pid_t result = waitpid(job.getPid(), &status, WNOHANG);
        if (result == 0) {
            // Child is still running
            return false;
        } else if (result > 0) {
            // Child has finished, check how it terminated
            return WIFEXITED(status) || WIFSIGNALED(status);
        } else {
            // Error occurred (e.g., no such process)
            // We might want to consider this job as 'finished' to remove it
            return true;
        }
    }

    // Helper function to find job by ID
    static bool jobIdEquals(const JobEntry& job, int jobId) {
        return job.getJobId() == jobId;
    }

    // Helper function to check if a job is stopped
    static bool isJobStopped(const JobEntry& job) {
        return job.getIsStopped();
    }

public:
    JobsList() : maxJobId(1) {}

    void addJob(pid_t pid, const string commandLine, bool isStopped = false) {
        removeFinishedJobs();
        jobs.push_back(JobEntry(maxJobId++, pid, commandLine, isStopped));
        updateMaxJobId();
    }

    void removeFinishedJobs() {
        std::vector<JobEntry>::iterator it = jobs.begin();
        while (it != jobs.end()) {
            if (isJobFinished(*it)) {
                it = jobs.erase(it);
            } else {
                ++it;
            }
        }
        updateMaxJobId(); // Update after cleaning up
    }

    void removeJobById(int jobId) {
        for (auto it = jobs.begin(); it != jobs.end(); ++it) {
            if (it->getJobId() == jobId) {
                jobs.erase(it);
                updateMaxJobId(); // Recalculate maxJobId after removal
                break;
            }
        }
    }



    JobEntry* getJobById(int jobId) {
        for (auto& job : jobs) {
            if (jobIdEquals(job, jobId)) {
                return &job;
            }
        }
        return nullptr;
    }

    JobEntry* getLastJob(int* lastJobId) {
        if (jobs.empty()) return nullptr;
        *lastJobId = jobs.back().getJobId();
        return &jobs.back();
    }


    JobEntry* getLastStoppedJob(int* jobId) {
        for (auto it = jobs.rbegin(); it != jobs.rend(); ++it) {
            if (isJobStopped(*it)) {
                *jobId = it->getJobId();
                return &(*it);
            }
        }
        return nullptr;
    }

    void printJobsList() {
      //  removeFinishedJobs();
        for (const auto& job : jobs) {
            std::cout << "[" << job.getJobId() << "] " << job.getCommandLine();
            if (job.getIsStopped()) std::cout << " (stopped)";
            std::cout << std::endl;
        }
    }

    void killAllJobs() {
        for (const auto& job : jobs) {
            kill(job.getPid(), SIGKILL);
        }
        jobs.clear();
        updateMaxJobId();
    }

    int size() const { return jobs.size(); }

    const std::vector<JobEntry>& getJobs() const {
        return jobs;
    }

    void updateMaxJobId() {
        int newMaxId = 0;
        for (const auto& job : jobs) {
            if (job.getJobId() > newMaxId) {
                newMaxId = job.getJobId();
            }
        }
        maxJobId = newMaxId + 1; // Next available ID
    }
};





#endif //SMASH_COMMAND_H_
