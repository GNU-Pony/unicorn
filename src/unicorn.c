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
#ifndef MAX_PATH
#define MAX_PATH  4096
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
  char unicorn_path_env[5 + MAX_PATH];
  char* unicorn_path = unicorn_path_env + 5;
  
  /* Get the largest possible PATH */
  #define _bin(DIR)			\
    ":" DIR "/bin"   ":" DIR "/xbin"	\
    ":" DIR "/sbin"  ":" DIR "/sxbin"
  
  snprintf(unicorn_path_env, 5 + MAX_PATH,
           "PATH=%s"
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
	      memcpy(p_uniq, p, (size_t)(p_end - p) * sizeof(char));
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
  
  /* Autocomplete command name */
  if ((argc == 3) && (argv[1][0] == '-') && (argv[1][1] == 0))
    {
      /* The command name to complete */
      char* command = argv[2];
      /* The end of our PATH */
      char* end = unicorn_path + strlen(unicorn_path) + 1;
      /* The end of the current directory */
      char* p_end;
      /* The current directory */
      char* p;
      
      /* The pathname of the found file */
      char pathname[MAX_PATH];
      
      /* Look for commands in each directory in our PATH */
      for (p = unicorn_path; p != end; p = p_end + 1)
	{
	  DIR* dir;
	  struct dirent* file;
	  
	  /* Find the character delimiting the current
	     directory and the next directory */
	  p_end = strchrnul(p, ':');
	  /* NUL-terminate the current directory */
	  *p_end = '\0';
	  
	  /* Opem the directory, skip to next on error */
	  if ((dir = opendir(p)) == NULL)
	    continue;
	  
	  /* Compare all files in the directory agains the partial command name  */
	  while ((file = readdir(dir)) != NULL)
	    /* Check that the file name starts with the partial name of the command */
	    if (strstr(file->d_name, command) == file->d_name)
	      /* Check that the found command does not match those so
	         ridiculously included . and .. */
	      if (strcmp(file->d_name, ".") && strcmp(file->d_name, ".."))
		{
		  struct stat attr;
		  mode_t mode;
		  
		  /* Concatenate the directory and the file */
		  sprintf(pathname, "%s/%s", p, file->d_name);
		  
		  /* Get file stat, ignore on failure (removed or broken link)  */
		  if (stat(pathname, &attr))
		    continue;
		  
		  /* Check that the file (symlinks followed) an
		     executable (by anypony) regulare file (keep
		     in mind, directories are normally executable) */
		  mode = attr.st_mode;
		  if (S_ISREG(mode) && ((S_IXUSR | S_IXGRP | S_IXOTH) & mode))
		    /* Print the found commend */
		    printf("%s\n", file->d_name);
		}
	  
	  /* Close the directory */
	  closedir(dir);
	}
    }
  else
    {
      int preserve_env = 0;
      char* command_file = NULL;
      char** command_args;
      size_t command_argc;
      char* command;
      int i;
      
      /* Parse command line options */
      for (i = 1; i < argc; i++)
	{
	  char* arg = argv[i];
	  
	  if (*arg != '-')
	    break;
	  else if (!strcmp("-p", arg) || !strcmp("--preserve-env", arg))
	    preserve_env = 1;
	  else if (!strcmp("-h", arg) || !strcmp("--help", arg))
	    {
	      printf("%s [-p] [--] command...\n\n%s\n",
		     *argv,
		     "    -p (--preserve-env)    Do not update PATH in the environment\n"
		     "\n"
		     "    -h (--heelp)           Print this help\n"
		     "    -c (--copyright)       Print copyright information\n"
		     "    -w (--warranty)        Print warranty disclaimer");
	      return 0;
	    }
	  else if (!strcmp("-c", arg) || !strcmp("--copying", arg) || !strcmp("--copyright", arg))
	    {
	      printf("%s\n",
		     "unicorn – PATH unification utility\n"
		     "\n"
		     "Copyright © 2014  Mattias Andrée (maandree@member.fsf.org)\n"
		     "\n"
		     "This program is free software: you can redistribute it and/or modify\n"
		     "it under the terms of the GNU General Public License as published by\n"
		     "the Free Software Foundation, either version 3 of the License, or\n"
		     "(at your option) any later version.\n"
		     "\n"
		     "This program is distributed in the hope that it will be useful,\n"
		     "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		     "GNU General Public License for more details.\n"
		     "\n"
		     "You should have received a copy of the GNU General Public License\n"
		     "along with this program.  If not, see <http://www.gnu.org/licenses/>.");
	      return 0;
	    }
	  else if (!strcmp("-w", arg) || !strcmp("--warranty", arg))
	    {
	      printf("%s\n",
		     "This program is distributed in the hope that it will be useful,\n"
		     "but WITHOUT ANY WARRANTY; without even the implied warranty of\n"
		     "MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the\n"
		     "GNU General Public License for more details.");
	      return 0;
	    }
	  else if (!strcmp("--", arg))
	    {
	      i++;
	      break;
	    }
	  else
	    fprintf(stderr, "%s: warning: option was not recognised: %s\n", *argv, arg);
	}
      if (i == argc)
	{
	  fprintf(stderr, "%s: error: no command specified\n", *argv);
	  return 1;
	}
      command = *(command_args = argv + i);
      command_argc = (size_t)(argc - i);
      
      /* Use the path of the command if it was specified */
      if (strchr(command, '/'))
	command_file = command;
      
      /* Look for the command in our PATH if we did not get its path */
      if (command_file == NULL)
	{
	  /* The end of our PATH */
	  char* end = unicorn_path + strlen(unicorn_path) + 1;
	  /* The end of the current directory */
	  char* p_end;
	  /* The current directory */
	  char* p;
	  
	  /* The pathname of the found file */
	  char pathname[MAX_PATH];
	  
	  for (p = unicorn_path; (p != end) && (command_file == NULL); p = p_end + 1)
	    {
	      DIR* dir;
	      struct dirent* file;
	      
	      /* Remove NUL-termination from last directory */
	      if (p != unicorn_path)
		*(p - 1) = ':';
	      
	      /* Find the character delimiting the current
		 directory and the next directory */
	      p_end = strchrnul(p, ':');
	      /* NUL-terminate the current directory */
	      *p_end = '\0';
	      
	      /* Opem the directory, skip to next on error */
	      if ((dir = opendir(p)) == NULL)
		continue;
	      
	      /* Compare all files in the directory agains the partial command name  */
	      while ((file = readdir(dir)) != NULL)
		/* Check that the file name starts with the partial name of the command */
		if (!strcmp(file->d_name, command))
		  /* Check that the found command does not match those so
		     ridiculously included . and .. */
		  if (strcmp(file->d_name, ".") && strcmp(file->d_name, ".."))
		    {
		      struct stat attr;
		      mode_t mode;
		      
		      /* Concatenate the directory and the file */
		      sprintf(pathname, "%s/%s", p, file->d_name);
		      
		      /* Get file stat, ignore on failure (removed or broken link)  */
		      if (stat(pathname, &attr))
			continue;
		      
		      /* Check that the file (symlinks followed) an
			 executable (by anypony) regulare file (keep
			 in mind, directories are normally executable) */
		      mode = attr.st_mode;
		      if (S_ISREG(mode) && ((S_IXUSR | S_IXGRP | S_IXOTH) & mode))
			{
			  /* We found it! */
			  if ((command_file = strdup(pathname)) == NULL)
			    {
			      perror(*argv);
			      abort();
			    }
			  printf("%s\n", file->d_name);
			  break;
			}
		    }
	      
	      /* Close the directory */
	      closedir(dir);
	    }
	  /* Remove NUL-termination from last directory */
	  if (p != unicorn_path)
	    *(p - 1) = ':';
	}
      if (command_file == NULL)
	{
	  fprintf(stderr, "%s: error: command not found: %s\n", *argv, command);
	  return 1;
	}
      
      /* Modify the environment if -p was not used */
      if (preserve_env == 0)
	if (putenv(unicorn_path_env))
	  perror(*argv);
      
      /* Execute command */
      {
	/* NULL-terminate argument list */
	char** command_argv = malloc((command_argc + 1) * sizeof(char*));
	memcpy(command_argv, command_args, command_argc * sizeof(char*));
	*(command_argv + command_argc) = NULL;
	/* Exec */
	execv(command_file, command_args);
	perror(*argv);
	abort();
      }
      return 2;
    }
  
  return 0;
}

