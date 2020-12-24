#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

//       (pai)
//         |
//    +----+----+
//    |         |
// filho_1   filho_2


// ~~~ printfs  ~~~
// pai (ao criar filho): "Processo pai criou %d\n"
//    pai (ao terminar): "Processo pai finalizado!\n"
//  filhos (ao iniciar): "Processo filho %d criado\n"

// Obs:
// - pai deve esperar pelos filhos antes de terminar!


int main(int argc, char** argv) {
    pid_t pid0 = fork();
    pid_t pid1;
    if(pid0>0){
      printf("Processo pai criou %d\n", pid0);
      pid1 = fork();
      if(pid1>0){
        printf("Processo pai criou %d\n", pid1);
        int status0;
        wait(&status0);
        wait(&status0);
        printf("Processo pai finalizado!\n");
      }else if(pid1==0){
        printf("Processo filho %d criado\n", getpid());
      }
    }else if(pid0==0){          //processo filho 0
      printf("Processo filho %d criado\n", getpid());
    }

    //pai criou 0, pai criou 1, filho 0 criado, filho 1 criado
    // ....
    return 0;
}
