#include <ctype.h>
#include <unistd.h>
#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <errno.h>
#include <pwd.h>
#include <grp.h>
#include <time.h>
#include <math.h>
#include <sys/types.h>
#include <termios.h>
#include <sys/ioctl.h>

int COLS;

// should have created a struct, will do but for now...
char file_type;
char permissions[10] = {0};
int link_count;
char owner[50] = {0}, group[50] = {0}, time_string[50] = {0}, file_name[100] = {0};
long file_size;

int cmpfunc (const void * a, const void * b);
void update_windows_dimension();
void do_ls(char*, int, int);
void set_stat_info(char*);
char getFileType(long);
void getPermissions(long mode, char* str);
void getOwnerName(int uid, char* owner);
void getGroupName(int gid, char* group);
void show_stat_info(char*);
void color_print(char*, char, int);

int main (int argc, char **argv)
{
  int lflag = 0;
  int rflag = 0;
  int c;
  int index;
  update_windows_dimension();

  opterr = 0;

  while ((c = getopt (argc, argv, ":Rl")) != -1)
  {
    switch (c)
    {
    case 'l':
      lflag = 1;
      break;
    case 'R':
      rflag = 1;
      break;
    case '?':
      if (isprint (optopt))
        fprintf (stderr, "Unknown option `-%c'.\nAsk Ahmad For Help\n", optopt);

      else
        fprintf (stderr,
                 "Unknown option character `\\x%x'.\nAsk Ahmad For Help\n",
                 optopt);
      return 1;
      break;
    default:
      abort ();
    }
  }

  // printf("%d %d\n", lflag, rflag);
  if (optind == argc)
  {
    puts("-----------------------------");
    printf("Directory listing of pwd:\n");
    puts("-----------------------------");
    do_ls(".", lflag, rflag);
  }
  else
    for (index = optind; index < argc; index++)
    {
      // printf ("Non-option argument %s\n", argv[index]);
      puts("-----------------------------");
      printf("Directory listing of %s:\n", argv[index]);
      puts("-----------------------------");
      do_ls(argv[index], lflag, rflag);
    }

  return 0;
}

void do_ls(char * dir, int lflag, int rflag)
{
  struct dirent * entry;
  DIR * dp = opendir(dir);
  if (dp == NULL) {
    fprintf(stderr, "Cannot open directory:%s\n", dir);
    return;
  }
  chdir(dir);
  errno = 0;
  char* names[1000] = {};
  char* dirs_array[1000] = {};
  int idx = 0;
  int max_name_length = -1;
  while ((entry = readdir(dp)) != NULL) {
    if (entry == NULL && errno != 0) {
      perror("readdir failed");
      exit(1);
    } else {
      if (entry->d_name[0] != '.') {
        names[idx++] = entry->d_name;
        if ((int)strlen(entry->d_name) > max_name_length)
          max_name_length = (int)strlen(entry->d_name);
      }
    }
  }


  qsort(names, idx, sizeof(names[0]), cmpfunc);

  int cols = COLS / (max_name_length + 2);
  int offset = ceil((double)idx / cols);
  // printf("%d %d %d\n", idx, cols, offset);

  int dir_count = 0;

  if (lflag)
    for (int i = 0; i < idx; i++)
    {
      set_stat_info(names[i]);
      if (names[i][0] != '.' && file_type == 'd')
        dirs_array[dir_count++] = names[i];
      show_stat_info(names[i]);
    }
  else
  {
    // printf("%d\n", COLS / (max_name_length + 2));
    for (int i = 0; i < offset; i++) {
      for (int j = i; j < idx; j += offset) {
        // printf("%d   ", j);
        set_stat_info(names[i]);

        if (names[i][0] != '.' && file_type == 'd')
          dirs_array[dir_count++] = names[i];

        // printf("%-*s", (max_name_length + 2) , names[j]);
        color_print(names[j], file_type, max_name_length + 2);
      }
      puts("");
    }
  }

  if (rflag)
    for (int i = 0; i < dir_count; i++)
    {
      puts("-----------------------------");
      printf("Directory listing of %s:\n", dirs_array[i]);
      puts("-----------------------------");
      do_ls(dirs_array[i], lflag, rflag);
    }

  chdir("..");
  closedir(dp);
}

void set_stat_info(char *file_name) {
  struct stat info;
  int rv = lstat(file_name, &info);
  if (rv == -1) {
    perror("stat failed");
    exit(1);
  }

  strcpy(file_name, file_name);
  file_type = getFileType(info.st_mode);
  link_count = info.st_nlink;
  file_size = info.st_size;
  getPermissions(info.st_mode, permissions);
  getOwnerName(info.st_uid, owner);
  getGroupName(info.st_gid, group);

  //set formatted time in time_string
  struct tm * time_info;
  time_t current_time = info.st_mtime;
  time_info = localtime(&current_time);
  strftime(time_string, sizeof(time_string), "%H:%M %d %B", time_info);

}

char getFileType(long mode) {
  if ((mode &  0170000) == 0010000)
    return 'p';
  else if ((mode &  0170000) == 0020000)
    return 'c';
  else if ((mode &  0170000) == 0040000)
    return 'd';
  else if ((mode &  0170000) == 0060000)
    return 'b';
  else if ((mode &  0170000) == 0100000)
    return '-';
  else if ((mode &  0170000) == 0120000)
    return 'l';
  else if ((mode &  0170000) == 0140000)
    return 's';
}

void getPermissions(long mode, char* str) {
  strcpy(str, "---------");
//owner  permissions
  if ((mode & 0000400) == 0000400) str[0] = 'r';
  if ((mode & 0000200) == 0000200) str[1] = 'w';
  if ((mode & 0000100) == 0000100) str[2] = 'x';
//group permissions
  if ((mode & 0000040) == 0000040) str[3] = 'r';
  if ((mode & 0000020) == 0000020) str[4] = 'w';
  if ((mode & 0000010) == 0000010) str[5] = 'x';
//others  permissions
  if ((mode & 0000004) == 0000004) str[6] = 'r';
  if ((mode & 0000002) == 0000002) str[7] = 'w';
  if ((mode & 0000001) == 0000001) str[8] = 'x';
//special  permissions
  if ((mode & 0004000) == 0004000) str[2] = 's';
  if ((mode & 0002000) == 0002000) str[5] = 's';
  if ((mode & 0001000) == 0001000) str[8] = 't';
}

void getOwnerName(int uid, char* owner) {
  errno = 0;
  struct passwd * pwd = getpwuid(uid);

  if (pwd == NULL) {
    if (errno == 0)
      printf("Record not found in passwd file.\n");
    else
      perror("getpwuid failed");
  }
  else
    strcpy(owner, pwd->pw_name);
}

void getGroupName(int gid, char* group) {
  struct group * grp = getgrgid(gid);

  errno = 0;
  if (grp == NULL) {
    if (errno == 0)
      printf("Record not found in /etc/group file.\n");
    else
      perror("getgrgid failed");
  } else
    strcpy(group, grp->gr_name);
}

void show_stat_info(char * file_name) {
  set_stat_info(file_name);
  printf("%c%s\t%d\t%s\t%s\t%ld\t%s\t\t",
         file_type, permissions, link_count, owner, group, file_size, time_string);
  color_print(file_name, file_type, 0);
  puts("");
}

void update_windows_dimension()
{
  struct winsize ws;
  ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
  // ROWS = ws.ws_row - 1;
  COLS = ws.ws_col;
  // printf("-----%d-------%d-------\n\n", ROWS, COLS);
}

int cmpfunc (const void * a, const void * b) {
  const char **ia = (const char **)a;
  const char **ib = (const char **)b;
  return strcmp(*ia, *ib);
}

void color_print(char* file_name, char file_type, int s)
{
  switch (file_type)
  {
  case 'p':
    printf("%-*s", s, file_name);
    break;
  case 'c':
  case 'b':
    printf("\033[7m%-*s\033[m", s, file_name);
    break;
  case 'd':
    printf("\e[0;34m%-*s\033[0m", s, file_name);
    break;
  case 'l':
    printf("\033[95;6m%-*s\033[0m", s, file_name);
    break;
  case 's':
    printf("%-*s", s, file_name);
    break;
  case '-':
    if (strstr(file_name, ".out") || strstr(file_name, ".out"))
      printf("\e[0;32m%-*s\033[0m", s, file_name);
    else if (strstr(file_name, ".tar"))
      printf("\e[0;31m%-*s\033[0m", s, file_name);
    else
      printf("%-*s", s, file_name);
    break;
  }
}