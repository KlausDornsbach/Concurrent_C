#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>
//        (pai)
//          |
//      +---+---+
//      |       |
//     sed    grep

// ~~~ printfs  ~~~
//        sed (ao iniciar): "sed PID %d iniciado\n"
//       grep (ao iniciar): "grep PID %d iniciado\n"
//          pai (ao iniciar): "Processo pai iniciado\n"
// pai (após filho terminar): "grep retornou com código %d,%s encontrou silver\n"
//                            , onde %s é
//                              - ""    , se filho saiu com código 0
//                              - " não" , caso contrário

// Obs:
// - processo pai deve esperar pelo filho
// - 1º filho deve trocar seu binário para executar "grep silver text"
//   + dica: use execlp(char*, char*...)
//   + dica: em "grep silver text",  argv = {"grep", "silver", "text"}
// - 2º filho, após o término do 1º deve trocar seu binário para executar
//   sed -i /silver/axamantium/g;s/adamantium/silver/g;s/axamantium/adamantium/g text
//   + dica: leia as dicas do grep

int main(int argc, char** argv) {
    printf("Processo pai iniciado\n");

    // ....
    fflush(NULL);
    pid_t pid0 = fork();
    int stat0;
    if(pid0>0){
      wait(&stat0);
      fflush(NULL);
      pid_t pid1 = fork();
      int stat1;
      if(pid1==0){
        //programa grep iniciado
        printf("grep PID %d iniciado\n", getpid());
        const char *a[3];
        a[0] = "grep";
        a[1] = "silver";
        a[2] = "text";
        execlp(a[2], a[0], a[1], NULL);
        exit(stat1);
      }else if(pid1>0){
        printf("alou");
        wait(&stat1);
        printf("alou");
        if(WIFEXITED(stat1)){
          printf("grep retornou com código 0, encontrou adamantium");
        }else{
          printf("grep retornou com código %d, não encontrou adamantium", stat1);
        }
        return 0;
      }
    }else if(pid0==0){
      //rodo programa sed
      printf("sed PID %d iniciado\n", getpid());
      const char *b[5];
      b[0] = "sed";
      b[1] = "-i";
      b[2] = "-e";
      b[3] = "s/silver/axamantium/g;s/adamantium/silver/g;s/axamantium/adamantium/g";
      b[4] = "text";
      execlp(b[4], b[0], b[1], b[2], b[3], NULL);
    }
    return 0;
}
