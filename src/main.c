#include <stdio.h>
#include <unistd.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <ftw.h>
#include <openssl/sha.h>
#include <sys/stat.h>

const char *program_name;
unsigned int fnopen = 10; /* Max number of fileno opened at the same time by ftw */

int generate_hash(const char *fpath, const struct stat *sb, int typeflag)
{
	size_t read_bytes = 0;
	SHA256_CTX c;
	unsigned char hash_buffer[SHA256_DIGEST_LENGTH];
	unsigned char file_buffer[1024 * 1024]; /* 1MB */

	if(typeflag != FTW_F)
		return 0;

	FILE *fp = fopen(fpath, "r");
	if(fp == NULL)
	{
		fprintf(stderr, "%s: failed to open '%s': %s\n", program_name, fpath, strerror(errno));
		return 0;
	}

	SHA256_Init(&c);

	while((read_bytes = fread(file_buffer, 1, sizeof(file_buffer), fp)))
	{
		SHA256_Update(&c, file_buffer, read_bytes);
	}

	SHA256_Final(hash_buffer, &c);
	printf("%s\n", fpath);

	fclose(fp);
	return 0;
}


int main(int argc, char **argv)
{
	/*  Usage:
	 *  ./cdup file1.txt file2.c directory directory2
	 *  or
	 *  ./cdup (current directory)
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
				ftw(argv[i], generate_hash, fnopen);
			}
			else if((sb.st_mode & S_IFMT) == S_IFREG)
			{
				generate_hash(argv[i], &sb, FTW_F);
			}
			else
			{
				fprintf(stderr, "%s: file type not supported '%s'\n", program_name, argv[i]);
			}
		}
	}
	else
	{
		ftw(".", generate_hash, fnopen);
	}

	return 0;
}
