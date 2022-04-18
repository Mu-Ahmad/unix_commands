/*
*  Video Lecture 11
*  morev4.c: handle io rediretion....reverse video featureread and print one page then pause for a few special commands ('q', ' ' , '\n')
 */

#include<stdio.h>
#include <stdlib.h>
#include <time.h>
#include <sys/types.h>
#include<string.h>
#include <sys/wait.h>
#include<termios.h>
#include<unistd.h>
#include <sys/ioctl.h>

int ROWS, COLS;

static struct termios info;

void do_more(FILE *, const char* file_name);
int  get_input(FILE*, int, int, const char*);
int get_total_line_count(FILE*);
void update_windows_dimension();
int search(FILE*, FILE*);
void open_in_vim(const char*, int);

int main(int argc , char *argv[])
{
   int i = 0;
   update_windows_dimension();
   if (argc == 1) {
      // check if pipe or io redicrectionn
      char proc_link[1024];
      char file_name[1024];
      int fd = fileno(stdin);
      sprintf(proc_link, "/proc/self/fd/%d", fd);
      memset(file_name, 0, sizeof(file_name));
      readlink(proc_link, file_name, sizeof(file_name) - 1);

      if (strstr(file_name, "tty") || strstr(file_name, "dev") || strstr(file_name, "pts"))
      {
         puts("mymore: bad usage\nAsk 'Ahmad' for help and more information.");
         return 0;
      }

      do_more(stdin, "");
   }
   FILE * fp;
   while (++i < argc) {
      fp = fopen(argv[i] , "r");
      if (fp == NULL) {
         perror("Can't open file");
         exit (1);
      }
      do_more(fp, argv[i]);
      puts("::::::::::::::::::::::::::::::::::::::::");
      fclose(fp);
   }
   return 0;
}

void do_more(FILE *fp, const char * file_name)
{
   int num_of_lines = 0;
   int total_line_count = (strlen(file_name)) ? get_total_line_count(fp) : 0;
   int line_count = 0;
   int rv;
   char buffer[COLS];
   FILE* fp_tty = fopen("/dev//tty", "r");
   while (fgets(buffer, COLS, fp)) {
      fputs(buffer, stdout);
      num_of_lines++;
      line_count++;
      const char* label = "\e[7m\033[1m --more(%0.f%%)--\e[m";
      if (strlen(file_name) == 0) label = "\e[7m\033[1m --MORE--\e[m";
check_point:
      if (num_of_lines == ROWS) {
         rv = get_input(fp_tty, line_count, total_line_count, label);
         if (rv == 0) { //user pressed q
            printf("\033[2K \033[1G");
            break;//
         }
         else if (rv == 1) { //user pressed space bar
            printf("\033[2K \033[1G");
            num_of_lines = 0;
         }
         else if (rv == 2) { //user pressed return/enter
            printf("\033[1A \033[2K \033[1G");
            num_of_lines = ROWS - 1; //show one more line
         }
         else if (rv == 3) { //invalid character
            printf("\033[1A \033[2K \033[1G");
            break;
         }
         else if (rv == 4) { //user entered '/'
            printf("\033[2K \033[1G/");
            // if (strlen(file_name) == 0) goto check_point;

            if (search(fp, fp_tty))
            {
               num_of_lines = 2;
            }
            else
            {
               // printf("\033[1A \033[2K \033\e[7m\033[1mPattern not found\e[m");
               // label = "\033[1A\033[2K\033\e[7m\033[1mPattern not found\e[m";
               puts("Pattern Not Found");
               exit(0);
            }
         }
         else if (rv == 5) { //vim
            if (strlen(file_name) == 0)
            {
               printf("\033[2K \033[1G");
               goto check_point;
            }
            open_in_vim(file_name, line_count);
            goto check_point;
            break;
         }
      }
   }
}

int get_input(FILE* cmdstream, int lines, int total_line_count, const char* label)
{
   int c;
   printf(label, (float) lines / total_line_count * 100);
   int fd = fileno(cmdstream);
   tcgetattr(fd, &info);
   info.c_lflag &= ~ICANON ;
   tcsetattr(fd, TCSANOW, &info);

   c = getc(cmdstream);

   info.c_lflag |= ICANON;
   tcsetattr(fd, TCSANOW, &info);

   update_windows_dimension();

   if (c == 'q')
      return 0;
   if ( c == ' ' )
      return 1;
   if ( c == '\n' )
      return 2;
   if ( c == '/' )
      return 4;
   if ( c == 'v' )
      return 5;
   return 3;
}

int get_total_line_count(FILE* f)
{
   int total_line_count = 0;
   char buffer[COLS];
   while (fgets(buffer, COLS, f))  total_line_count++;
   rewind(f);
   return total_line_count;
}

void update_windows_dimension()
{
   struct winsize ws;
   ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws);
   ROWS = ws.ws_row - 1;
   COLS = ws.ws_col;
   // printf("-----%d-------%d-------\n\n", ROWS, COLS);
}

int search(FILE* fp, FILE* cmdstream)
{
   char buff[COLS], target[COLS];
   fgets(target, COLS, cmdstream);
   // scanf("%[^\n]%*c", buff);
   target[strlen(target) - 1] = 0;

   if (fp == NULL) {
      perror("Something went wrong");
      exit (1);
   }


   while (fgets(buff, COLS, fp))
   {
      if (strstr(buff, target))
      {
         puts("\033[1A \033[2K \033[1GSkippinn......");
         printf("%s", buff);
         return 1;
      }
   }

   return 0;
}

void open_in_vim(const char* file_name, int line)
{
   printf("\033[2K \033[1G");
   char command[100];
   sprintf(command, "vim +%d %s", line, file_name);
   system(command);
   return;
}