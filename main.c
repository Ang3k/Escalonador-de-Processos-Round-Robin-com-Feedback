#include <stdio.h>
#include <stdlib.h>
#include <time.h>

// Definindo o quantum e o máximo de processos no escalonador
#define max_processos 10
#define quantum 3
int contador_global = 0;
int tempo_chegada_acumulado = 0;

// Estruturas
typedef struct {
    int PID;
    int PPID;
    int status;
} pcb;

typedef struct {
    pcb pcb_info; 
    int tempo_servico;
    int tempo_chegada;
    int tempo_CPU_usado;
    
    int num_total_IO;            
    int proximo_IO;               
    int tipo_IO[3]; // Os IO´S são representados por valores inteiros, "1" representa Disco por exemplo
    int gatilhos_IO[3];  
} processo;

typedef struct {
    int inicio;
    int fim;
    int tamanho;
    processo fila_processos[max_processos];
} fila;

// Protótipos das funções
processo criadora_processos();
void inicializar_fila(fila* f);
void enfileirar(fila* f, processo p);
processo desenfileirar(fila* f);
void printa_fila(fila* f);
void printa_lista_de_processos(processo lista[], int num_processos);
int rodar_processo(processo* p, int tempo_atual, int* terminou);
int CPU_executa(fila* alta, fila* baixa, int* finalizados, int tempo_atual);


// Implementações das funções

processo criadora_processos(){
    processo novo_processo;

    // Informações da PCB
    novo_processo.pcb_info.PID = contador_global++;
    novo_processo.pcb_info.PPID = 1;
    novo_processo.pcb_info.status = 0;

    // Contextos gerais
    novo_processo.tempo_CPU_usado = 0;
    tempo_chegada_acumulado += (rand() % 4 + 1); 
    novo_processo.tempo_chegada = tempo_chegada_acumulado; 

    // Informações de IO
    int temp = 0;
    novo_processo.num_total_IO = (rand() % 4);
    novo_processo.proximo_IO = 0;
    for (int i = 0 ; i < novo_processo.num_total_IO ; i++){
        novo_processo.gatilhos_IO[i] = temp + (rand() % 6) + 2;
        novo_processo.tipo_IO[i] = (rand() % 3) + 1;
        temp = novo_processo.gatilhos_IO[i];
    }
    novo_processo.tempo_servico = temp + (rand() % 10) + 1; // utilizando temp para ter tempo o suficiente para rodar o programa e fazer os IO´s 
    return novo_processo;
}


// Lógicas para as filas

void inicializar_fila(fila* f){
    f->inicio = 0;
    f->fim = 0;
    f->tamanho = 0;
}

void enfileirar(fila* f, processo p){
    if (f->tamanho >= max_processos){ return; } 
    f->fila_processos[f->fim] = p;
    f->fim = (f->fim + 1) % max_processos;
    f->tamanho++;
}

processo desenfileirar(fila* f){
    if (f->tamanho == 0){
        processo processo_vazio = {0}; 
        processo_vazio.pcb_info.PID = -1;
        return processo_vazio;
    }
    processo p = f->fila_processos[f->inicio]; 
    f->inicio = (f->inicio + 1) % max_processos; 
    f->tamanho--;
    return p;
}

void printa_fila(fila* f){
    if (f->tamanho == 0) { printf("[vazia]"); return; }
    int i = f->inicio;
    int count = 0;
    while(count < f->tamanho) {
        printf("[PID:%d] ", f->fila_processos[i].pcb_info.PID);
        i = (i + 1) % max_processos;
        count++;
    }
}

// Vetor para printar informações na função à seguir
static const char* dispositivos_IO[] = {
    "Nenhum",           // 0
    "Disco",            // 1
    "Fita Magnetica",   // 2
    "Impressora"        // 3
};

// Função apenas para printara as informações principais do processo
void printa_lista_de_processos(processo lista[], int num_processos) {
    printf("\n--------------------------------------------------------------------------\n");
    printf("PID | Chegada | Servico | I/Os | Dispositivos\n");
    printf("----|---------|---------|------|------------------------------------------\n");
    
    for (int i = 0; i < num_processos; i++) {
        printf("%3d |   %3d   |   %3d   |  %2d  | ", 
               lista[i].pcb_info.PID,
               lista[i].tempo_chegada, 
               lista[i].tempo_servico,
               lista[i].num_total_IO);
        
        // Imprime todas as operações I/O em uma linha
        if (lista[i].num_total_IO > 0) {
            for (int j = 0; j < lista[i].num_total_IO; j++) {
                printf("%s(%d)", dispositivos_IO[lista[i].tipo_IO[j]], lista[i].gatilhos_IO[j]);
                if (j < lista[i].num_total_IO - 1) printf(", ");
            }
        } else {
            printf("Nenhuma");
        }
        printf("\n");
    }
    printf("--------------------------------------------------------------------------\n\n");
}

// Função Principal para Rodar os Processos
int rodar_processo(processo* p, int tempo_atual, int* terminou) {
    printf("Tempo %d: CPU <- PID %d (Servico restante: %d)\n", tempo_atual, p->pcb_info.PID, p->tempo_servico);
    p->pcb_info.status = 1;
    int tempo_executado = 0;
    while (tempo_executado < quantum && p->tempo_servico > 0) { // Código só termina quando o quantum acaba ou o tempo de serviço na CPU se esgota
        p->tempo_servico--;
        p->tempo_CPU_usado++;
        tempo_executado++;
    }
    if (p->tempo_servico <= 0) { // Informa se terminamos o tempo de serviço, atualizando o status na PCB
        p->pcb_info.status = 3;
        printf("Tempo %d: PID %d TERMINOU.\n", tempo_atual + tempo_executado, p->pcb_info.PID);
        *terminou = 1;
    } else { // Preempção do processo pelo fim do quantum
        p->pcb_info.status = 0;
        printf("Tempo %d: PID %d sofreu PREEMPCAO. Falta %d de servico.\n", tempo_atual + tempo_executado, p->pcb_info.PID, p->tempo_servico);
        *terminou = 0;
    }
    return tempo_executado;
}


int CPU_executa(fila* alta, fila* baixa, int* finalizados, int tempo_atual) {
    processo p_executando;
    int processo_terminou = 0;
    int tempo_gasto = 0;

    if (alta->tamanho > 0) { // Busca primeiro na fila de alta prioridade, removendo o processo de lá e rodando ele
        p_executando = desenfileirar(alta);
        tempo_gasto = rodar_processo(&p_executando, tempo_atual, &processo_terminou);

        if (processo_terminou == 1) (*finalizados)++;
        else enfileirar(baixa, p_executando); // Se ele não terminar, vai pra fila de baixa prioridade
        
    } else if (baixa->tamanho > 0) { // Busca se não achar na fila de alta prioridade
        p_executando = desenfileirar(baixa);
        tempo_gasto = rodar_processo(&p_executando, tempo_atual, &processo_terminou);

        if (processo_terminou == 1) (*finalizados)++;
        else enfileirar(baixa, p_executando); // Se ele não terminar, se mantém na fila de baixa prioridade
        
    } else {
        printf("Tempo %d: CPU Ociosa...\n", tempo_atual);
        return 1; 
    }
    return tempo_gasto;
}


// Lógica Principal
int main() {
    fila prioridade_alta, prioridade_baixa;
    inicializar_fila(&prioridade_alta);
    inicializar_fila(&prioridade_baixa);
    
    srand(time(NULL));

    // Simplesmente criando todos os processos que vão ser simulados
    processo todos_os_processos[max_processos]; 
    for (int i = 0; i < max_processos; i++) todos_os_processos[i] = criadora_processos();
    printa_lista_de_processos(todos_os_processos, max_processos);


    // Variáveis para o controle do tempo na simulação
    int tempo_simulacao = 0;
    int processos_finalizados = 0;
    int tempo_anterior = -1; 

    printf("\n--- INICIO DA SIMULACAO (Quantum = %d) ---\n", quantum);
    while (processos_finalizados < max_processos) {
        // Verifica chegadas no INTERVALO de tempo
        for (int i = 0; i < max_processos; i++) {
            if (todos_os_processos[i].tempo_chegada > tempo_anterior && todos_os_processos[i].tempo_chegada <= tempo_simulacao) {
                printf("Tempo %d: Chegou o PID %d.\n", todos_os_processos[i].tempo_chegada, todos_os_processos[i].pcb_info.PID);
                enfileirar(&prioridade_alta, todos_os_processos[i]);
            }
        }

        int tempo_de_execucao = CPU_executa(&prioridade_alta, &prioridade_baixa, &processos_finalizados, tempo_simulacao);
        
        printf("Estado no fim do ciclo que comecou em %d: Fila Alta: ", tempo_simulacao);
        printa_fila(&prioridade_alta);
        printf(" | Fila Baixa: ");
        printa_fila(&prioridade_baixa);
        printf("\n---------------------------------------------\n");
        
        // Atualiza o tempo anterior antes de pular
        tempo_anterior = tempo_simulacao;
        tempo_simulacao += tempo_de_execucao;
    }

    printf("\n--- FIM DA SIMULACAO ---\n");
    printf("Simulacao finalizada em %d unidades de tempo.\n", tempo_simulacao);
    printf("Total de processos finalizados: %d\n", processos_finalizados);

    return 0;
}