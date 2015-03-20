

#ifndef _GNU_SOURCE
#  define _GNU_SOURCE /* ftello(3) */
#endif /* !_GNU_SOURCE */
#ifndef _POSIX_SOURCE
#  define _POSIX_SOURCE /* fileno(3) */
#endif /* _POSIX_SOURCE */


#include <sys/stat.h>

#include <errno.h>
#include <stdint.h> /* SIZE_MAX */
#include <stdio.h>
#include <stdlib.h>

#include "helper.h"


static
char * fread_file(FILE *fstream, size_t *length);


/* ========================================================================= */
static
char * fread_file(FILE *fstream, size_t *length)
/* ========================================================================= */
{
	char *buf = NULL;
	size_t alloc = BUFSIZ;

	/* For a regular file, allocate a buffer that has exactly the right
	   size.  This avoids the need to do dynamic reallocations later.  */
	{
		struct stat st;

		if (fstat(fileno(fstream), &st) >= 0 && S_ISREG(st.st_mode)) {
			off_t pos = ftello(fstream);

			if ((pos >= 0) && (pos < st.st_size)) {
				off_t alloc_off = st.st_size - pos;

				/* '1' below,
				 * accounts for the trailing NUL.  */
				if (((unsigned) (SIZE_MAX - 1)) < alloc_off) {
					errno = ENOMEM;
					return NULL;
				}
 
				alloc = alloc_off + 1;
			}
		}
	}

	buf = malloc(alloc);
	if (buf == NULL) {
		return NULL; /* errno is ENOMEM.  */
	}
    
	{
		size_t size = 0; /* number of bytes read so far */
		int save_errno;

		for (;;) {
			/* This reads 1 more than the size of a regular file
			   so that we get eof immediately.  */
			size_t requested = alloc - size;
			size_t count = fread(buf + size, 1, requested,
			    fstream);
			size += count;

			if (count != requested) {
				save_errno = errno;
				if (ferror(fstream)) {
					break;
				}

				/* Shrink the allocated memory if possible.  */
				if (size < alloc - 1) {
					char *smaller_buf =
					    realloc(buf, size + 1);
					if (smaller_buf != NULL) {
						buf = smaller_buf;
					}
				}

				buf[size] = '\0';
				*length = size;
				return buf;
			}

			{
				char *new_buf;
        
				if (alloc == SIZE_MAX) {
					save_errno = ENOMEM;
					break;
				}

				if (alloc < SIZE_MAX - alloc / 2) {
					alloc = alloc + alloc / 2;
				} else {
					alloc = SIZE_MAX;
				}

				if (!(new_buf = realloc (buf, alloc))) {
					save_errno = errno;
					break;
				}

				buf = new_buf;
			}
		}

		free(buf);
		errno = save_errno;
		return NULL;
	}
}


/* ========================================================================= */
char * internal_read_file(const char *fname, size_t *length, const char *mode)
/* ========================================================================= */
{
	FILE *stream = fopen (fname, mode);
	char *out;
	int save_errno;

	if (NULL == stream) {
		return NULL;
	}

	out = fread_file(stream, length);

	save_errno = errno;

	if (fclose (stream) != 0) {
		if (out) {
			save_errno = errno;
			free (out);
		}
		errno = save_errno;
		return NULL;
	}

	return out;
}
