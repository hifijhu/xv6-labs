#include "kernel/types.h"
#include "kernel/stat.h"
#include "user/user.h"
#include "kernel/fs.h"

char*
fmtname(char *path)
{
  
  char *p;

  // Find first character after last slash.
  for(p=path+strlen(path); p >= path && *p != '/'; p--)
    ;
  p++;
  return p;
  // Return blank-padded name.
  /*if(strlen(p) >= DIRSIZ)
    return p;
  memmove(buf, p, strlen(p));
  memset(buf+strlen(p), ' ', DIRSIZ-strlen(p));
  return buf;*/
}



void find(char *path, char *cond){
  char buf[512], *p;
  int fd;
  struct dirent de;
  struct stat st;
  
  if((fd = open(path, 0)) < 0){
    fprintf(2, "ls: cannot open %s\n", path);
    return;
  }

  if(fstat(fd, &st) < 0){
    
    fprintf(2, "ls: cannot stat %s\n", path);
    close(fd);
    return;
  }

  switch(st.type){
  case T_FILE:
    if(strcmp(fmtname(path), cond) == 0){
      printf("%s\n", path);
    }
    return;
    break;

  case T_DIR:
    if(strlen(path) + 1 + DIRSIZ + 1 > sizeof buf){
      printf("ls: path too long\n");
      break;
    }
    strcpy(buf, path);
    p = buf+strlen(buf);
    *p++ = '/';
    while(read(fd, &de, sizeof(de)) == sizeof(de)){
      if(de.inum == 0)
        continue;
      memmove(p, de.name, DIRSIZ);
      p[DIRSIZ] = 0;
      if(stat(buf, &st) < 0){
        printf("1 ");
        printf("ls: cannot stat %s\n", buf);
        continue;
      }
      
      if((strcmp(fmtname(buf), ".") == 0) || (strcmp(fmtname(buf), "..") == 0)){
        
        continue;
      }
      switch (st.type)
      {
      case T_FILE:
        if(strcmp(fmtname(buf), cond) == 0){
          printf("%s\n", buf);
        }
        break;
      
      case T_DIR:
        find(buf, cond);
        break;
      }
      
      
      
    }
    break;
  }
  close(fd);

}

int
main(int argc, char *argv[])
{
  int i;

  if(argc < 3){
    fprintf(2, "usage: find file..");
    exit(0);
  }
  char *path = argv[1];
  char *cond = argv[2];
  find(path, cond);
  exit(0);
}
