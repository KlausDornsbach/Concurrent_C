#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <stdio.h>
#include <pthread.h>

// Lê o conteúdo do arquivo filename e retorna um vetor E o tamanho dele
// Se filename for da forma "gen:%d", gera um vetor aleatório com %d elementos
//
// +-------> retorno da função, ponteiro para vetor malloc()ado e preenchido
// |
// |         tamanho do vetor (usado <-----+
// |         como 2o retorno)              |
// v                                       v
double* load_vector(const char* filename, int* out_size);


// Avalia se o prod_escalar é o produto escalar dos vetores a e b. Assume-se
// que ambos a e b sejam vetores de tamanho size.
void avaliar(double* a, double* b, int size, double prod_escalar);

int main(int argc, char* argv[]) {
    srand(time(NULL));

    //Temos argumentos suficientes?
    if(argc < 4) {
        printf("Uso: %s n_threads a_file b_file\n"
               "    n_threads    número de threads a serem usadas na computação\n"
               "    *_file       caminho de arquivo ou uma expressão com a forma gen:N,\n"
               "                 representando um vetor aleatório de tamanho N\n",
               argv[0]);
        return 1;
    }

    //Quantas threads?
    int n_threads = atoi(argv[1]);
    if (!n_threads) {
        printf("Número de threads deve ser > 0\n");
        return 1;
    }
    //Lê números de arquivos para vetores alocados com malloc
    int a_size = 0, b_size = 0;
    double* a = load_vector(argv[2], &a_size);
    if (!a) {
        //load_vector não conseguiu abrir o arquivo
        printf("Erro ao ler arquivo %s\n", argv[2]);
        return 1;
    }
    double* b = load_vector(argv[3], &b_size);
    if (!b) {
        printf("Erro ao ler arquivo %s\n", argv[3]);
        return 1;
    }

    //Garante que entradas são compatíveis
    if (a_size != b_size) {
        printf("Vetores a e b tem tamanhos diferentes! (%d != %d)\n", a_size, b_size);
        return 1;
    }
    //Cria vetor do resultado
    double* c = malloc(a_size*sizeof(double));

    //Calcula produto escalar. Paralelize essa parte
    double result = 0;//vai adicionar todos os resultados

    int vet[n_threads];//vetor auxiliar para ciação de threads
    for(int i = 0; i<n_threads; i++){
      vet[i] = i;
    }

    pthread_t thread[n_threads];//ponteiros threads

    void *func_thread(void *arg){
      int index = *((int*)arg);
      if(index<a_size){
        c[index] = a[index] * b[index];
      }
      if(n_threads<a_size){
        int cont = 1;
        int k = index + cont*n_threads;
        while(k < a_size){
          c[k] = a[k]*b[k];
          cont++;
          k = index + cont*n_threads;
        }
      }
      pthread_exit(NULL);
    }


    for(int i = 0; i < n_threads && i<a_size; i++){//crio threads
      pthread_create(&thread[i], NULL, func_thread, &vet[i]);
    }

    for(int i = 0; i<n_threads && i<a_size; i++){
      pthread_join(thread[i], NULL);
    }

    for(int i = 0; i<a_size; i++ ){
      result += c[i];
    }


    //    +---------------------------------+
    // ** | IMPORTANTE: avalia o resultado! | **
    //    +---------------------------------+
    avaliar(a, b, a_size, result);

    //Libera memória
    free(a);
    free(b);
    free(c);

    return 0;
}
