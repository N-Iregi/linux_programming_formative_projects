#include <dirent.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#define LOG_FILE "sync.log"

/**
 * write_log - appends a timestamped entry to the log file
 * @reason: why the file needs syncing
 * @name: the file name
 */
void write_log(const char *reason, const char *name)
{
	FILE *log;
	char stamp[32];
	time_t now;

	log = fopen(LOG_FILE, "a");
	if (log == NULL)
		return;
	now = time(NULL);
	strftime(stamp, sizeof(stamp), "%Y-%m-%d %H:%M:%S", localtime(&now));
	fprintf(log, "[%s] %s: %s\n", stamp, reason, name);
	fclose(log);
}

/**
 * checksum - calculates a simple checksum of a file's bytes
 * @path: path of the file
 *
 * Return: the checksum, or 0 if the file cannot be opened
 */
unsigned long checksum(const char *path)
{
	FILE *fp;
	unsigned char buf[512];
	unsigned long sum = 0;
	size_t n, i;

	fp = fopen(path, "rb");
	if (fp == NULL)
		return (0);
	while ((n = fread(buf, 1, sizeof(buf), fp)) > 0)
	{
		for (i = 0; i < n; i++)
			sum = sum * 31 + buf[i];
	}
	fclose(fp);
	return (sum);
}

/**
 * needs_sync - decides whether a source file must be synchronized
 * @src_path: full path of the source file
 * @dst_path: full path of the destination file
 *
 * Return: 0 if no sync is needed, 1 if missing, 2 if the size differs,
 * 3 if the checksum differs
 */
int needs_sync(const char *src_path, const char *dst_path)
{
	struct stat src_st, dst_st;

	if (stat(src_path, &src_st) == -1 || !S_ISREG(src_st.st_mode))
		return (0);
	if (stat(dst_path, &dst_st) == -1)
		return (1);
	if (src_st.st_size != dst_st.st_size)
		return (2);
	if (checksum(src_path) != checksum(dst_path))
		return (3);
	return (0);
}

/**
 * scan_directory - reports files in src that need syncing to dst
 * @src: source directory
 * @dst: destination directory
 *
 * Return: number of files needing sync, or -1 on error
 */
int scan_directory(const char *src, const char *dst)
{
	const char *reasons[] = {"", "missing", "size differs", "checksum differs"};
	char src_path[512], dst_path[512];
	struct dirent *entry;
	DIR *dp;
	int count = 0, why;

	dp = opendir(src);
	if (dp == NULL)
	{
		perror("opendir source");
		return (-1);
	}
	while ((entry = readdir(dp)) != NULL)
	{
		if (strcmp(entry->d_name, ".") == 0 ||
		    strcmp(entry->d_name, "..") == 0)
			continue;
		snprintf(src_path, sizeof(src_path), "%s/%s", src, entry->d_name);
		snprintf(dst_path, sizeof(dst_path), "%s/%s", dst, entry->d_name);
		why = needs_sync(src_path, dst_path);
		if (why != 0)
		{
			printf("NEEDS SYNC: %s (%s)\n", entry->d_name, reasons[why]);
			write_log(reasons[why], entry->d_name);
			count++;
		}
	}
	closedir(dp);
	return (count);
}

/**
 * main - compares two directories using size and checksum checks
 * @argc: argument count
 * @argv: argument vector (source directory, destination directory)
 *
 * Return: 0 on success, 1 on error
 */
int main(int argc, char *argv[])
{
	int count;

	if (argc != 3)
	{
		fprintf(stderr, "Usage: %s <source_dir> <dest_dir>\n", argv[0]);
		return (1);
	}
	count = scan_directory(argv[1], argv[2]);
	if (count == -1)
		return (1);
	printf("%d file(s) need synchronization\n", count);
	return (0);
}
