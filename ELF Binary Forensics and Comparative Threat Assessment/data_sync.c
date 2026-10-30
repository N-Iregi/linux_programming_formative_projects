#include <dirent.h>
#include <stdio.h>
#include <string.h>

/**
 * name_in_dir - checks whether a file name exists in a directory
 * @dir: directory to search
 * @name: file name to look for
 *
 * Return: 1 if found, 0 if not found, -1 if the directory cannot be opened
 */
int name_in_dir(const char *dir, const char *name)
{
	DIR *dp;
	struct dirent *entry;
	int found = 0;

	dp = opendir(dir);
	if (dp == NULL)
		return (-1);
	while ((entry = readdir(dp)) != NULL)
	{
		if (strcmp(entry->d_name, name) == 0)
		{
			found = 1;
			break;
		}
	}
	closedir(dp);
	return (found);
}

/**
 * scan_directory - reports files in src whose names are missing from dst
 * @src: source directory
 * @dst: destination directory
 *
 * Return: number of files needing sync, or -1 on error
 */
int scan_directory(const char *src, const char *dst)
{
	DIR *dp;
	struct dirent *entry;
	int count = 0, found;

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
		found = name_in_dir(dst, entry->d_name);
		if (found == -1)
		{
			perror("opendir destination");
			closedir(dp);
			return (-1);
		}
		if (found == 0)
		{
			printf("NEEDS SYNC: %s\n", entry->d_name);
			count++;
		}
	}
	closedir(dp);
	return (count);
}

/**
 * main - compares two directories by file name
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
