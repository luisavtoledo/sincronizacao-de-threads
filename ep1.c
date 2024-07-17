#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include "passa_tempo.h"

#define MAX_N 20

pthread_mutex_t mut;
pthread_cond_t grid_movement;

// estrutura de dados para guardar informações de cada posição
typedef struct Position_info {
    int x;
    int y;
    int time;
} Position_info; 

// estrutura da dados para guardar informações da thread
typedef struct Thread_info {
    int id;
    int group;
    int n_positions;
    Position_info positions[2 * MAX_N];
} Thread_info; 

// estrutura de dados para armazenar quais threads estão em determinada posição
typedef struct Grid_position {
    int group_ids[2];
    int num_threads;
} Grid_position;

// matriz para representar a grade
Grid_position grid[MAX_N][MAX_N];

// inicializa a grade, com todas as posições vazias
void initialize_grid(int N) {
    for (int i = 0; i < N; i++) {
        for (int j = 0; j < N; j++) {
            grid[i][j].group_ids[0] = -1;
            grid[i][j].group_ids[1] = -1;
            grid[i][j].num_threads = 0;
        }
    }
}

/* As funções a seguir (can_move, go_in, get_out) são usadas para a implementação da sincronização. Antes de uma thread entrar em uma nova posição (função go_in), ela adquire o mutex e trava
o acesso de outras threads à seção crítica, que é o trecho de código onde uma thread altera sua posição (se não houvesse essa proteção, mais de uma thread poderiam tentar entrar na mesma posição 
ao mesmo tempo, o que resultaria em erro). Em seguida, ela verifica se ela pode entrar na posição (função can_move), ou seja, se esse movimento seria válida de acordo com a regra estabelecida. 
Caso ela possa entrar, ela entra, atualiza a posição para mostrar que está ali e libera o mutex (permitindo que outra thread agora entre na seção crítica). Caso contrário, ela libera a trava e 
outras threads podem se movimentar, sendo que assim a thread original será avisada assim que houver um movimento de saída de alguma posição. Para uma thread sair de uma posição, ela trava o mutex
e entra na seção crítica, onde atualiza os atributos da posição, se removendo. Em seguida, ela avisa que houve uma saída de posição e libera a trava mutex, para que outras possam mudar de posição.*/

// função que verifica se thread pode entrar na posição
int can_move(int x, int y, int group) {

    // se não houver threads na posição, pode entrar
    if(grid[x][y].num_threads == 0) {
        return 0;
    }
    // se houver apenas uma thread na posição e a thread for de um grupo diferente, pode entrar
    else if(grid[x][y].num_threads == 1 ) {
        
        // se houver uma thread do mesmo grupo na posição, não pode entrar
        if(grid[x][y].group_ids[0] == group || grid[x][y].group_ids[1] == group) {
            return 1;
        }
        return 0;
    }
    // se houver 2 ou mais threads na posição, não pode entrar
    else {
        return 1;
    }

}

// função para a thread entrar na posição
void go_in(int x, int y, int group) {

    // trava mutex para entrar na seção crítica
    pthread_mutex_lock(&mut);

    // enquanto não puder entrar, libera mutex e espera um movimento na grade
    while (can_move(x, y, group) != 0) {
        pthread_cond_wait(&grid_movement, &mut);
    }

    // entra na seção crítica
    // atualiza o grid (colocando suas informações na posição na qual entrou)
    grid[x][y].num_threads++;
    if(grid[x][y].group_ids[0] == -1) {
        grid[x][y].group_ids[0] = group;
    }
    else {
        grid[x][y].group_ids[1] = group;
    }

    // libera trava da seção crítica
    pthread_mutex_unlock(&mut);

}

// função para a thread sair da posição
void get_out(int x, int y, int group) {

    // trava mutex para entrar na seção crítica
    pthread_mutex_lock(&mut);

    // entrou na seção crítica
    // atualiza o grid (tirando suas informações da posição)
    grid[x][y].num_threads--;
    if(grid[x][y].group_ids[0] == group) {
        grid[x][y].group_ids[0] = -1;
    }
    else {
        grid[x][y].group_ids[1] = -1;
    }

    //sinaliza que saiu de uma posição
    pthread_cond_signal(&grid_movement);

    // libera trava da seção crítica
    pthread_mutex_unlock(&mut);

}


// função de execução da thread
void *thread_function(void *arg) {

    Thread_info *thread_info = (Thread_info *)arg;

    // informações da primeira posição que a thread vai visitar
    int current_position = 0;
    Position_info current_position_info = thread_info->positions[current_position];
    int current_x = current_position_info.x;
    int current_y = current_position_info.y;
    int current_time = current_position_info.time;

    int group = thread_info->group;

    // thread entra na primeira posição
    go_in(current_x, current_y, group);

    passa_tempo(thread_info->id, current_x, current_y, current_time);

    // percorre todas as próximas posições da thread
    for (int i = 1; i < thread_info->n_positions; i++) {

        // informações da próxima posição que a thread vai entrar
        Position_info next_position_info = thread_info->positions[i];
        int next_x = next_position_info.x;
        int next_y = next_position_info.y;
        int next_time = next_position_info.time;

        // thread entra na próxima posição
        go_in(next_x, next_y, thread_info->group);

        //thread sai da posição anterior
        get_out(current_x, current_y, thread_info->group);

        passa_tempo(thread_info->id, next_x, next_y, next_time);

        // atualização das informações
        current_x = next_x;
        current_y = next_y;
        current_time = next_time;

    }

    // thread sai da última posição
    get_out(current_x, current_y, thread_info->group);

    return NULL;

}

int main() {

    int N, n_threads;

    // lê a dimensão do tabuleiro e o número de threads
    scanf("%d %d", &N, &n_threads);

    initialize_grid(N);

    // cria um array que guarda as informações das threads
    Thread_info threads[n_threads];

    for(int i = 0; i < n_threads; i++) {

        // lê o id, o grupo e o número de posições da thread
        int id, group, n_positions;
        scanf("%d %d %d", &id, &group, &n_positions);

        threads[i].id = id;
        threads[i].group = group;
        threads[i].n_positions = n_positions;

        for(int j = 0; j < n_positions; j++) {

            // lê as coordenadas e o tempo referentes à posição
            int x, y, time;
            scanf("%d %d %d", &x, &y, &time);

            threads[i].positions[j].x = x;
            threads[i].positions[j].y = y;
            threads[i].positions[j].time = time;
        }
    }

    pthread_mutex_init(&mut, NULL);
    pthread_cond_init(&grid_movement, NULL);

    // lista das threads
    pthread_t thread_list[n_threads];

    // cria e inicializa cada uma das threads
    for (int i = 0; i < n_threads; i++) {
        pthread_create(&thread_list[i], NULL, thread_function, (void *)&threads[i]);
    }

    // aguarda a conclusão de todas as threads antes de terminar o programa
    for (int i = 0; i < n_threads; i++) {
        pthread_join(thread_list[i], NULL);
    }

    pthread_mutex_destroy(&mut);
    pthread_cond_destroy(&grid_movement);

    return 0;
}