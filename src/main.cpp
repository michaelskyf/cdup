#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <ftw.h>
#include <stdlib.h>
#include <openssl/sha.h>
#include <sys/stat.h>

/* In the future I might replace it with my own implementation */
#include <map>
#include <utility>

const char *program_name;
unsigned int fnopen = 10; /* Max number of fileno opened at the same time by ftw */

struct fdata
{
	const char *fpath;
	struct fdata *next, *prev;
};

struct hdata
{
	struct fdata *f;
	struct hdata *next;
	struct hdata *prev;
};

std::map<size_t, struct fdata*> map;

struct hdata *last_entry(struct hdata *first)
{
	struct hdata *d = first;
	struct hdata *pd = first;
	while(d)
	{
		pd = d;
		d = d->next;
	}

	return pd;
}

void compare_files(struct fdata *first_data)
{
	size_t count = 1; /* Number of elements */
	struct fdata *d = first_data;
	while(d->next)
	{
		d = d->next;
		count++;
	}

	unsigned char hashes[count][SHA256_DIGEST_LENGTH];
	unsigned char file_buffer[1024 * 1024]; /* 1MB */

	SHA256_CTX c;
	size_t read_bytes;

	/* Calculate hashes */
	d = first_data;
	for(size_t i = 0; d; i++)
	{
		FILE *fp = fopen(d->fpath, "r");
		if(fp == NULL)
		{
			fprintf(stderr, "%s: failed to open '%s': %s\n", program_name, d->fpath, strerror(errno));
			return;
		}

		SHA256_Init(&c);

		while((read_bytes = fread(file_buffer, 1, sizeof(file_buffer), fp)))
		{
			SHA256_Update(&c, file_buffer, read_bytes);
		}

		fclose(fp);
		SHA256_Final(hashes[i], &c);

		d = d->next;
	}

	/* Compare hashes */
	d = first_data;
	struct hdata buckets[count];
	for(size_t i = 0; i < count; i++)
	{
		buckets[i].next = NULL;
		buckets[i].prev = NULL;
		buckets[i].f = d;
		d = d->next;
	}

	for(size_t i = 0; i < count; i++)
	{
		if(buckets[i].next || buckets[i].prev)
			continue;

		for(size_t j = 0; j < count; j++)
		{
			if(buckets[j].prev || buckets[j].next || i == j)
				continue;

			if(memcmp(hashes[i], hashes[j], sizeof(*hashes)) == 0)
			{
				struct hdata *l = last_entry(&buckets[i]);
				l->next = &buckets[j];
				buckets[j].prev = l;
			}
		}
	}

	for(size_t i = 0; i < count; i++)
	{
		if(buckets[i].prev == NULL && buckets[i].next)
		{
			struct hdata *h = &buckets[i];
			struct stat sb;
			stat(h->f->fpath, &sb);
			float size = sb.st_size;
			const char *size_type;

			if(size < 1024)
			{
				size_type = "B";
			}
			else if(size < 1024*1024)
			{
				size_type = "KB";
				size /= 1024;
			}
			else
			{
				size_type = "MB";
				size /= 1024 * 1024;
			}

			printf("\nFound duplicate files of size %.02f %s:\n", size, size_type);
			while(h)
			{
				printf("+%s\n", h->f->fpath);

				h = h->next;
			}
		}
	}
}

int find_files(const char *fpath, const struct stat *sb, int typeflag)
{
	if(typeflag != FTW_F || sb->st_size == 0)
		return 0;

	struct fdata *d = (struct fdata*)malloc(sizeof(struct fdata));

	d->fpath = strdup(fpath);
	d->next = NULL;
	d->prev = NULL;

	/* If there's already a file (or files) with the same size */
	if(map.insert(std::make_pair(sb->st_size, d)).second == false)
	{
		struct fdata *dp = map[sb->st_size];
		while(dp->next)
			dp = dp->next;

		dp->next = d;
		d->prev = dp;
	}

	return 0;
}

void destroy_list(struct fdata *d)
{

	/* Find last element */
	while(d->next)
	{
		d = d->next;
	}

	struct fdata *pd;
	while(d)
	{
		pd = d->prev;

		free((void*)d->fpath);
		free(d);

		d = pd;
	}
}


int main(int argc, char **argv)
{
	/*  Usage:
	 *  ./cdup file1.txt file2.c directory directory2
	 *  or
	 *  ./cdup (current directory)
	*/

	/* Notes:
	 * If file has 0 bytes, skip it
	 *
	*/

	/* Steps:
	 *  1. Find all files and their sizes and add them to std::map
	 *  2. If there's 2 or more files under one entry, compare their hashes
	 *  3. If hashes are identical, print file names
	*/

	program_name = argv[0];

	if(argc != 1)
	{
		struct stat sb;
		for(int i = 1; i < argc; i++)
		{
			if(stat(argv[i], &sb))
			{
				fprintf(stderr, "%s: failed to stat '%s': %s\n", program_name, argv[i], strerror(errno));
				continue;
			}

			if((sb.st_mode & S_IFMT) == S_IFDIR)
			{
				ftw(argv[i], find_files, fnopen);
			}
			else if((sb.st_mode & S_IFMT) == S_IFREG)
			{
				find_files(argv[i], &sb, FTW_F);
			}
			else
			{
				fprintf(stderr, "%s: file type not supported '%s'\n", program_name, argv[i]);
			}
		}
	}
	else
	{
		ftw(".", find_files, fnopen);
	}

	for(auto i = map.begin(); i != map.end(); i++)
	{
		if(i->second->next)
			compare_files(i->second);
		destroy_list(i->second);
	}

	return 0;
}
