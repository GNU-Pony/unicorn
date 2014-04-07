/**
 * unicorn – PATH unification utility
 * 
 * Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>


/**
 * The maximum length of PATH
 */
#ifndef MAXPATH
#define MAXPATH  4096
#endif

/**
 * The number of buckets to use
 * when deduplicating
 */
#ifndef BUCKETS
#define BUCKETS  32
#endif

/**
 * The size of the buckets to
 * use when deduplicating
 */
#ifndef BUCKET_SIZE
#define BUCKET_SIZE  128
#endif


/**
 * This is the main entry point of the program
 * 
 * @param   argc  The number of elements in `argv`
 * @param   argv  Command line arguments
 * @return        Zero on success
 */
int main(int argc, char** argv)
{
  /* Get current PATH and user home  */
  char* path = getenv("PATH");
  char* fake_home = getenv("HOME");
  char* real_home = getpwuid(getuid()) ? getpwuid(getuid())->pw_dir : NULL;
  
  /* Unified PATH */
  char unicorn_path[MAXPATH];
  
  /* Get the largest possible PATH */
  #define _bin(DIR)			\
    ":" DIR "/bin"   ":" DIR "/xbin"	\
    ":" DIR "/sbin"  ":" DIR "/sxbin"
  
  snprintf(unicorn_path, MAXPATH,
           "%s"
           _bin("/usr/local")  _bin("/usr/local/games")
           _bin("/usr")        _bin("/usr/games")
           _bin("")            _bin("/games")
           _bin("%s/.local")   _bin("%s/.local/games")
           _bin("%s/.local")   _bin("%s/.local/games"),
	   path,
	   fake_home, fake_home, fake_home, fake_home,
	   fake_home, fake_home, fake_home, fake_home,
	   real_home, real_home, real_home, real_home,
	   real_home, real_home, real_home, real_home);
  
  #undef _bin
  
  /* Remove non-existant and duplicates (by inode) */
  {
    /* The location to write the next unique path in */
    char* p_uniq = unicorn_path;
    /* The end PATH */
    char* end = unicorn_path + strlen(unicorn_path) + 1;
    /* The end of the current directory in PATH */
    char* p_end;
    /* The current directory in PATH */
    char* p;
    
    /* Buckets used or identifying duplicates */
    ino_t ino_buckets[BUCKETS][BUCKET_SIZE];
    dev_t dev_buckets[BUCKETS][BUCKET_SIZE];
    int bucket_ptrs[BUCKETS] = {0};
    
    /* Start deduplication */
    for (p = unicorn_path; p != end; p = p_end + 1)
      {
	struct stat attr;
	
	/* Find the character delimiting the current
	   directory and the next directory */
	p_end = strchrnul(p, ':');
	/* NUL-terminate the current directory */
	*p_end = '\0';
	/* Do nothing for this directory if it
	   is just an empty string and not an
	   actual directory */
	if (*p == '\0')
	  continue;
	
	/* Include the directory only if it exists,
	   while getting identification of the directory */
	if (stat(p, &attr) == 0)
	  {
	    /* Base what bucket we use on the least
	       significant (highest verity) digits of
	       the directory's inode number */
	    int bucket = attr.st_ino % BUCKETS;
	    /* Get how many items we have in the bucket */
	    int bucket_ptr = bucket_ptrs[bucket];
	    /* Inode number hemibucket */
	    ino_t* ino_bucket = ino_buckets[bucket];
	    /* Device number hemibucket */
	    dev_t* dev_bucket = dev_buckets[bucket];
	    int i;
	    
	    /* Check if the directory is a duplicate */
	    for (i = 0; i < bucket_ptr; i++)
	      if (ino_bucket[i] == attr.st_ino)
		if (dev_bucket[i] == attr.st_dev)
		  break;
	    
	    /* Ignore the directory if it is a duplicate */
	    if (i < bucket_ptr)
	      continue;
	    
	    /* List the directory */
	    if (p != p_uniq)
	      /* Write it only if there no overlap,
	         a partial overlap is impossible */
	      memcpy(p_uniq, p, (p_end - p) * sizeof(char));
	    p_uniq += p_end - p;
	    /* Colon-terminate */
	    *p_uniq++ = ':';
	    
	    /* Do not add the directory to the bucket if
	       the bucket is full */
	    if (bucket_ptr == BUCKET_SIZE)
	      continue;
	    
	    /* Otherwise, add the directory to the bucket */
	    bucket_ptrs[bucket] = i + 1;
	    ino_bucket[i] = attr.st_ino;
	    dev_bucket[i] = attr.st_dev;
	  }
      }
    
    /* NUL-terminate our new PATH */
    *--p_uniq = '\0';
  }
  
  if ((argc == 3) && (argv[1][0] == '-') && (argv[1][1] == 0))
    {
      char* command = argv[2];
      char* end = unicorn_path + strlen(unicorn_path) + 1;
      char* p_end;
      char* p;
      
      for (p = unicorn_path; p != end; p = p_end + 1)
	{
	  DIR* dir;
	  struct dirent* file;
	  
	  p_end = strchrnul(p, ':');
	  *p_end = '\0';
	  
	  if ((dir = opendir(p)) == NULL)
	    continue;
	  
	  while ((file = readdir(dir)) != NULL)
	    if (strstr(file->d_name, command) == file->d_name)
	      if (strcmp(file->d_name, ".") && strcmp(file->d_name, ".."))
		printf("%s\n", file->d_name);
	  
	  closedir(dir);
	}
      
      return 0;
    }
  
  return 0;
}

