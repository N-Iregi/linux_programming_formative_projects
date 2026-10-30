#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

/* Buffer size for reading and writing data chunks during file copy operations */
#define BUF_SIZE 1024

/**
 * write_log - writes a numbered entry to the log file and the screen
 * @fd: open file descriptor pointing to the log file
 * @seq: pointer to the current log sequence integer counter
 * @msg: textual message content to record
 */
void write_log(int fd, int *seq, const char *msg)
{
    char line[256];
    int len;

    /* Increment the entry counter via pointer dereference */
    (*seq)++;

    /* Format string into a formatted log line: "[seq] message\n" */
    len = snprintf(line, sizeof(line), "[%d] %s\n", *seq, msg);

    /* Write formatted log entry directly to the file descriptor; print error on failure */
    if (write(fd, line, len) == -1)
        perror("write log");

    /* Echo the message output directly to standard output (terminal screen) */
    printf("%s\n", msg);
}

/**
 * make_backup_dir - creates the backup directory if it does not already exist
 * @dir: relative or absolute path of target directory
 *
 * Return: 0 if directory exists or was created successfully, -1 on error
 */
int make_backup_dir(const char *dir)
{
    /* Attempt to create directory with read/write/execute permissions for owner (7)
       and read/execute permissions for group and others (5) -> 0755 octal */
    if (mkdir(dir, 0755) == 0)
        return (0);

    /* If creation fails because directory already exists, treat as non-fatal success */
    if (errno == EEXIST)
        return (0);

    /* Print descriptive error message to stderr if creation fails for any other reason */
    fprintf(stderr, "mkdir %s: %s\n", dir, strerror(errno));
    return (-1);
}

/**
 * copy_file - copies contents from a source file path to a destination file path
 * @src: path of the source file to read from
 * @dst: path of the destination file to write to
 *
 * Return: 0 on complete file copy success, -1 on error
 */
int copy_file(const char *src, const char *dst)
{
    char buf[BUF_SIZE];
    ssize_t n;
    int in_fd, out_fd, status = 0;

    /* Open source file in Read-Only mode */
    in_fd = open(src, O_RDONLY);
    if (in_fd == -1)
    {
        fprintf(stderr, "open %s: %s\n", src, strerror(errno));
        return (-1);
    }

    /* Open or create destination file in Write-Only mode:
       O_CREAT: Create file if missing
       O_TRUNC: Overwrite existing contents (truncate length to 0)
       0644: Permissions -> Read/Write for owner, Read-Only for group/others */
    out_fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (out_fd == -1)
    {
        fprintf(stderr, "open %s: %s\n", dst, strerror(errno));
        close(in_fd); /* Clean up source file descriptor before returning error */
        return (-1);
    }

    /* Read source file in BUF_SIZE chunks until End-Of-File (returns 0) or error (returns -1) */
    while ((n = read(in_fd, buf, BUF_SIZE)) > 0)
    {
        /* Ensure all read bytes ('n') are successfully written to destination */
        if (write(out_fd, buf, n) != n)
        {
            fprintf(stderr, "write %s: %s\n", dst, strerror(errno));
            status = -1;
            break; /* Abort copy loop on write error */
        }
    }

    /* Check if loop ended due to a read system call error */
    if (n == -1)
    {
        fprintf(stderr, "read %s: %s\n", src, strerror(errno));
        status = -1;
    }

    /* Close file descriptors to free OS file handle resources */
    close(in_fd);
    close(out_fd);
    return (status);
}

/**
 * run_child_copy - spawns a child process to copy the file, parent waits for completion
 * @src: path of the source file
 * @dst: path of the destination file
 *
 * Return: 0 if child executed and copied successfully, -1 otherwise
 */
int run_child_copy(const char *src, const char *dst)
{
    pid_t pid;
    int status;

    /* Flush stdout buffer before fork to prevent duplicated log output in child */
    fflush(stdout);

    /* Fork a new child process: duplicates caller process state */
    pid = fork();
    if (pid == -1)
    {
        perror("fork");
        return (-1);
    }

    /* Code block executed strictly by the CHILD process (fork returns 0 in child) */
    if (pid == 0)
    {
        /* Perform file copy; exit with code 0 on success, exit code 1 on failure */
        if (copy_file(src, dst) == 0)
            exit(0);
        exit(1);
    }

    /* Code block executed strictly by the PARENT process (fork returns child's PID) */
    /* Synchronously wait for child process state change (termination) */
    if (waitpid(pid, &status, 0) == -1)
    {
        perror("waitpid");
        return (-1);
    }

    /* Check if child exited normally (WIFEXITED) and returned exit status 0 (WEXITSTATUS) */
    if (WIFEXITED(status) && WEXITSTATUS(status) == 0)
        return (0);

    return (-1);
}

/**
 * main - parses arguments, sets up directory and log file, runs worker child
 * @argc: argument count passed via command line
 * @argv: argument vector array (argv[1] = source, argv[2] = backup dir)
 *
 * Return: 0 on success, 1 on failure
 */
int main(int argc, char *argv[])
{
    char path[256];
    const char *name;
    int log_fd, seq = 0, ret;

    /* Enforce mandatory 2 arguments: program_name source_path backup_directory */
    if (argc != 3)
    {
        fprintf(stderr, "Usage: %s <source> <backup_dir>\n", argv[0]);
        return (1);
    }

    /* Step 1: Create backup directory if missing */
    if (make_backup_dir(argv[2]) == -1)
        return (1);

    /* Step 2: Construct log file path "<backup_dir>/backup.log" */
    snprintf(path, sizeof(path), "%s/backup.log", argv[2]);

    /* Open log file in write-only append mode (creates file if not present) */
    log_fd = open(path, O_WRONLY | O_CREAT | O_APPEND, 0644);
    if (log_fd == -1)
    {
        fprintf(stderr, "open %s: %s\n", path, strerror(errno));
        return (1);
    }

    /* Log successful directory preparation step */
    write_log(log_fd, &seq, "Backup directory ready");

    /* Step 3: Isolate target filename from full source path string (e.g., "dir/file.txt" -> "file.txt") */
    name = strrchr(argv[1], '/');
    name = name ? name + 1 : argv[1];

    /* Construct target destination file path "<backup_dir>/<filename>" */
    snprintf(path, sizeof(path), "%s/%s", argv[2], name);

    /* Log commencement of background copy operation */
    write_log(log_fd, &seq, "Starting copy in child process");

    /* Step 4: Dispatch process to perform actual file copy operation */
    ret = run_child_copy(argv[1], path);

    /* Log outcome status based on child process exit state */
    write_log(log_fd, &seq, ret == 0 ? "Copy succeeded" : "Copy FAILED");

    /* Close log file descriptor before exit */
    close(log_fd);

    /* Return exit code 0 on success, exit code 1 on copy failure */
    return (ret == 0 ? 0 : 1);
}
