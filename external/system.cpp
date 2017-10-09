/**
* Copyright (C) 2014-2015 All rights reserved.
* @file system.cpp 
* @brief system api function definition, this file derived from elf 
* @author Weidong Huang (huangweidong01@baidu.com)
* @version 1.0
* @date 2015-10-09 23:28
*/
#include "system.h"
#include <mutex>
#include <signal.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include "memory_handle.h"

namespace bdl {
namespace luna {

static const int KMALLOC_UNIT = 1 << 20;
static std::mutex s_popen_mutex;

int system_exec(const std::string& cmd, int retry_num, bool alloc_memory, char*& buf,
                const size_t in_buf_len, size_t& out_buf_len, int& cmd_ret_val) {
    FILE* fp = nullptr;
    MemoryHandleUniquePtr tmp_mem = nullptr;
    auto old_child_handler = signal(SIGCHLD, SIG_DFL);
    CHECK(SIG_ERR != old_child_handler) << "Failed to set SIGCHLD handler";

    if (alloc_memory) {
        CHECK(nullptr == buf) << "input paramter buf must be nullptr";
        CHECK(0 == in_buf_len) << "input paramter buf length must be zero";
        tmp_mem.reset(new MemoryHandle(KMALLOC_UNIT));
        CHECK(tmp_mem) << "new MemoryHandle with size: " << KMALLOC_UNIT << " failed";
        buf = reinterpret_cast<char*>(malloc(KMALLOC_UNIT + 1));
        CHECK(buf) << "malloc memory with size: " << KMALLOC_UNIT << " failed";
        out_buf_len = KMALLOC_UNIT;
    }

    for (int retry = 0; retry < retry_num; ++retry) {
        VLOG(3) << "Start to exec: [" << cmd << "]"
                << "retry:" << retry
                << "retry_num:" << retry_num;

        if (nullptr == (fp = popen_safe(cmd.c_str(), "r"))) {
            LOG(WARNING) << "[" << retry << "]th Failed to exec: [" << cmd << "]";
            continue;
        }

        if (alloc_memory) {
            size_t acc_size = 0;

            while (!feof(fp)) {
                size_t count = fread(tmp_mem->get_buf(), 1, KMALLOC_UNIT, fp);

                if (count != KMALLOC_UNIT && !feof(fp)) {
                    LOG(ERROR) << "Failed to read files";
                    pclose_safe(fp);
                    CHECK(SIG_ERR != signal(SIGCHLD, old_child_handler))
                            << "failed to set SIGCHLD signal handler";
                    return -1;
                }

                if (acc_size + count > out_buf_len) {
                    buf = reinterpret_cast<char*>(realloc(buf, out_buf_len * 2 + 1));
                    CHECK(buf) << "realloc memory failed.";
                    out_buf_len *= 2;
                }

                memcpy(buf + acc_size, tmp_mem->get_buf(), count);
                acc_size += count;
            }

            out_buf_len = acc_size;
            buf[acc_size] = 0;
        } else if (buf) {
            size_t count = fread(buf, 1, in_buf_len, fp);

            if (count != in_buf_len && !feof(fp)) {
                LOG(ERROR) << "Failed to read files";
                pclose_safe(fp);
                CHECK(SIG_ERR != signal(SIGCHLD, old_child_handler))
                        << "failed to set SIGCHLD signal handler";
                return -1;
            }

            if (EOF != fgetc(fp)) {
                LOG(WARNING) << "Provided buffer is not enough!";
            }

            out_buf_len = count;
        }

        int rc = pclose_safe(fp);
        cmd_ret_val = WEXITSTATUS(rc);
        CHECK(SIG_ERR != signal(SIGCHLD, old_child_handler))
                << "failed to set SIGCHLD signal handler";
        return 0;
    }

    LOG(ERROR) << "Exec: [" << cmd << "]" << "Failed after try " << retry_num << " times";
    return -1;
}

int system_exec_no_output(const std::string& cmd, int retry_num, int& cmd_ret_val) {
    char* buf = nullptr;
    size_t in_buf_len = 0;
    size_t out_buf_len = 0;
    int exec_ret = system_exec(cmd, retry_num, true, buf, in_buf_len, out_buf_len, cmd_ret_val);

    if (buf) {
        free(buf);
    }

    return exec_ret;
}

FILE* popen_safe(const char* command, const char* type) {
    std::unique_lock<std::mutex> lock(s_popen_mutex);
    int retry_num = 10;
    Configure::singleton().get_by_name<int>("common_systemCommandRetry", retry_num);
    FILE* ret = nullptr;

    for (int i = 1; i <= retry_num; ++i) {
        ret = popen(command, type);

        if (ret != nullptr) {
            return ret;
        }

        PLOG(WARNING) << "popen failed " << retry_num << " times with command: " << command;
        sleep(i);
    }

    return ret;
}

int pclose_safe(FILE* stream) {
    //int retry_num = 10;
    int retry_num = 1;
    Configure::singleton().get_by_name<int>("common_systemCommandRetry", retry_num);

    int ret = -1;

    for (int i = 1; i <= retry_num; ++i) {
        ret = pclose(stream);

        if (ret != -1) {
            return ret;
        }

        PLOG(WARNING) << "pclose failed " << retry_num << " times";
        sleep(i);
    }

    return ret;
}

void popen_bidirectional(const char* command, FILE*& fp_read, FILE*& fp_write, int& pid) {
    std::unique_lock<std::mutex> lock(s_popen_mutex);
    int fd_read[2];
    int fd_write[2];
    CHECK(command) << "input paramter command must not be nullptr";
    PCHECK(pipe(fd_read) == 0) << "create read pipe failed";
    PCHECK(pipe(fd_write) == 0) << "create write pipe failed";
    pid = fork();
    PCHECK(pid >= 0) << "fork failed.";

    if (pid == 0) {
        if (STDOUT_FILENO != fd_read[1]) {
            CHECK(-1 != dup2(fd_read[1], STDOUT_FILENO)) << "dup2 failed";
            close(fd_read[1]);
        }

        close(fd_read[0]);

        if (STDIN_FILENO != fd_write[0]) {
            CHECK(-1 != dup2(fd_write[0], STDIN_FILENO)) << "dup2 failed";
            close(fd_write[0]);
        }

        close(fd_write[1]);
        execl("/bin/sh", "sh", "-c", command, NULL);
        exit(errno);
    } else {
        close(fd_read[1]);
        close(fd_write[0]);
        fp_read = fdopen(fd_read[0], "r");
        CHECK(fp_read) << "fdopen failed.";
        fp_write = fdopen(fd_write[1], "w");
        CHECK(fp_write) << "fdopen failed.";
    }
}

void pclose_bidirectional(int pid, int& result) {
    int ret_state = -1;

    do {
        CHECK(waitpid(pid, &ret_state, 0) >= 0) << "waitpid failed.";

        if (WIFEXITED(ret_state)) {
            result = WEXITSTATUS(ret_state);
            VLOG(3) << "popen_bidirectional exited with: " << result << ". " << strerror(result);
        } else if (WIFSIGNALED(ret_state)) {
            LOG(WARNING) << "popen_bidirectional signaled by: " << strsignal(WTERMSIG(ret_state));
            result = WTERMSIG(ret_state);
            exit(WTERMSIG(ret_state));
        }
    } while (!WIFEXITED(ret_state) && !WIFSIGNALED(ret_state));
}

} // namespace luna
} // namespace bdl
