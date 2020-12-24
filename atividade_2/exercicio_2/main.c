#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdio.h>
#include <string.h>

//                          (principal)
//                               |
//              +----------------+--------------+
//              |                               |
//           filho_1                         filho_2
//              |                               |
//    +---------+-----------+          +--------+--------+
//    |         |           |          |        |        |
// neto_1_1  neto_1_2  neto_1_3     neto_2_1 neto_2_2 neto_2_3

// ~~~ printfs  ~~~
//      principal (ao finalizar): "Processo principal %d finalizado\n"
// filhos e netos (ao finalizar): "Processo %d finalizado\n"
//    filhos e netos (ao inciar): "Processo %d, filho de %d\n"

// Obs:
// - netos devem esperar 5 segundos antes de imprmir a mensagem de finalizado (e terminar)
// - pais devem esperar pelos seu descendentes diretos antes de terminar

int main(int argc, char** argv) {
    pid_t pid0 = fork();

    if(pid0>0){//primeira entrada do pai, cria filho 2
      fflush(NULL);
      pid_t pid1 = fork();
      if(pid1>0){//segunda entrada do pai,
        int status;
        wait(&status);
        wait(&status);
        printf("Processo principal %d finalizado\n", getpid());
      }else if(pid1==0){//entrada do filho 2, cria neto 1
        printf("Processo %d, filho de %d\n", getpid(), getppid());
        fflush(NULL);
        pid_t pid10 = fork();
        if(pid10>0){//cria neto 2
          fflush(NULL);
          pid_t pid11 = fork();
          if(pid11>0){//cria neto 3
            fflush(NULL);
            pid_t pid12 = fork();
            if(pid12>0){//entro novamente no filho 2 e dou wait
              int status1;
              wait(&status1);
              wait(&status1);
              wait(&status1);
              printf("Processo %d finalizado\n", getpid());
            }else if(pid12==0){
              printf("Processo %d, filho de %d\n", getpid(), getppid());
              sleep(5);
              printf("Processo %d finalizado\n", getpid());
              fflush(NULL);
            }
          }else if(pid11==0){//entra no neto 2
            printf("Processo %d, filho de %d\n", getpid(), getppid());

            sleep(5);
            printf("Processo %d finalizado\n", getpid());

          }
        }else if(pid10==0){
          printf("Processo %d, filho de %d\n", getpid(), getppid());

          sleep(5);
          printf("Processo %d finalizado\n", getpid());

        }
      }
    }else if(pid0==0){//estou no filho 1, cria neto 1
      printf("Processo %d, filho de %d\n", getpid(), getppid());
      fflush(NULL);
      pid_t pid00 = fork();
      if(pid00>0){//cria neto 2
        fflush(NULL);
        pid_t pid01 = fork();
        if(pid01>0){//cria neto 3
          fflush(NULL);
          pid_t pid02 = fork();
          if(pid02>0){//entro no pai novamente
            int status0;
            wait(&status0);
            wait(&status0);
            wait(&status0);
            printf("Processo %d finalizado\n", getpid());
          }else if(pid02==0){//entro no neto 3
            printf("Processo %d, filho de %d\n", getpid(), getppid());
            sleep(5);
            printf("Processo %d finalizado\n", getpid());
          }
        }else if(pid01==0){//entro no neto 2
          printf("Processo %d, filho de %d\n", getpid(), getppid());
          sleep(5);
          printf("Processo %d finalizado\n", getpid());
        }
      }else if(pid00==0){//entro no neto 1
        printf("Processo %d, filho de %d\n", getpid(), getppid());
        sleep(5);
        printf("Processo %d finalizado\n", getpid());
      }
    }
    // ....

    return 0;
}
