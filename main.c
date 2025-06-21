#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "fila.h"

// Definindo variáveis e constantes importantes
#define max_processos 10
#define quantum 3
#define TEMPO_DISCO 7
#define TEMPO_FITA 10
#define TEMPO_IMPRESSORA 15

int contador_global = 0;
int tempo_chegada_acumulado = 0;

// Protótipos das funções
processo criadora_processos();
void printa_lista_de_processos(processo lista[], int num_processos);
int rodar_processo(processo* p, int tempo_atual, int* status_final, int* tipo_io_ocorrido);
int CPU_executa(fila* alta, fila* baixa, fila* io_disco, fila* io_fita, fila* io_impressora, int* finalizados, int tempo_atual);
void gerenciar_e_avancar_IOs(int tempo_passado, int tempo_atual_ciclo, fila* alta, fila* baixa, fila* io_disco, fila* io_fita, fila* io_impressora);
static void processa_uma_fila_io(int tempo_passado, int tempo_atual_ciclo, fila* fila_io, fila* alta, fila* baixa, int tipo_io_id);


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
    novo_processo.tempo_restante_IO = 0;
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


// Vetor para printar informações na função à seguir
static const char* dispositivos_IO[] = {
    "Nenhum",
    "Disco",
    "Fita Magnetica",
    "Impressora"
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
int rodar_processo(processo* p, int tempo_atual, int* status_final, int* tipo_io_ocorrido) {
    printf("Tempo %d: CPU <- PID %d (Servico restante: %d)\n", tempo_atual, p->pcb_info.PID, p->tempo_servico);
    p->pcb_info.status = 1; // rodando
    int tempo_executado = 0;
    while (tempo_executado < quantum && p->tempo_servico > 0) { // Código só termina quando o quantum acaba ou o tempo de serviço na CPU se esgota
        p->tempo_servico--;
        p->tempo_CPU_usado++;
        tempo_executado++;
        
        if (p->proximo_IO < p->num_total_IO){
            int gatilho_atual = p->gatilhos_IO[p->proximo_IO];
            if (p->tempo_CPU_usado == gatilho_atual){
                printf("Tempo %d: PID %d solicitou I/O de %s.\n", tempo_atual + tempo_executado, p->pcb_info.PID, dispositivos_IO[p->tipo_IO[p->proximo_IO]]); 
                p->pcb_info.status = 2; // indica fazendo IO
                *status_final = 2;
                *tipo_io_ocorrido = p->tipo_IO[p->proximo_IO];
                p->proximo_IO++;
                return tempo_executado;
            }
        }
    }
    if (p->tempo_servico <= 0) { // Informa se terminamos o tempo de serviço, atualizando o status na PCB
        p->pcb_info.status = 3;
        printf("Tempo %d: PID %d TERMINOU.\n", tempo_atual + tempo_executado, p->pcb_info.PID);
        *status_final = 1;
    } else { // Preempção do processo pelo fim do quantum
        p->pcb_info.status = 0;
        printf("Tempo %d: PID %d sofreu PREEMPCAO. Falta %d de servico.\n", tempo_atual + tempo_executado, p->pcb_info.PID, p->tempo_servico);
        *status_final = 0;
    }
    return tempo_executado;
}


int CPU_executa(fila* alta, fila* baixa, fila* io_disco, fila* io_fita, fila* io_impressora, int* finalizados, int tempo_atual) {
    processo p_executando;
    int tempo_gasto = 0;
    int status_saida = 0;
    int tipo_io;

    if (alta->tamanho > 0) { // Busca primeiro na fila de alta prioridade, removendo o processo de lá e rodando ele
        p_executando = desenfileirar(alta);
        tempo_gasto = rodar_processo(&p_executando, tempo_atual, &status_saida, &tipo_io);

        switch(status_saida){
            case 0: // Preempção
                enfileirar(baixa, p_executando);
                break;
            case 1: // Terminou
                (*finalizados)++;
                break;
            case 2: // Pedido de I/O
                switch (tipo_io) {
                    case 1:
                        p_executando.tempo_restante_IO = TEMPO_DISCO;
                        enfileirar(io_disco, p_executando); break;
                    case 2:
                        p_executando.tempo_restante_IO = TEMPO_FITA;
                        enfileirar(io_fita, p_executando); break;
                    case 3:
                        p_executando.tempo_restante_IO = TEMPO_IMPRESSORA;
                        enfileirar(io_impressora, p_executando); break;
                }
                break;
            }

        }

    else if (baixa->tamanho > 0) { // Busca se não achar na fila de alta prioridade
        p_executando = desenfileirar(baixa);
        tempo_gasto = rodar_processo(&p_executando, tempo_atual, &status_saida, &tipo_io);

        switch (status_saida) {
            case 0: // Preempção
                enfileirar(baixa, p_executando); // Processo de baixa prioridade volta para a de baixa
                break;
            case 1: // Terminou
                (*finalizados)++;
                break;
            case 2: // Pedido de I/O
                switch (tipo_io) {
                    case 1:
                        p_executando.tempo_restante_IO = TEMPO_DISCO;
                        enfileirar(io_disco, p_executando); break;
                    case 2:
                        p_executando.tempo_restante_IO = TEMPO_FITA;
                        enfileirar(io_fita, p_executando); break;
                    case 3:
                        p_executando.tempo_restante_IO = TEMPO_IMPRESSORA;
                        enfileirar(io_impressora, p_executando); break;
                }
                break;
        }
    }   else {
        printf("Tempo %d: CPU Ociosa...\n", tempo_atual);
        return 1;
    }
    return tempo_gasto > 0 ? tempo_gasto : 1;
}

static void processa_uma_fila_io(int tempo_passado, int tempo_atual_ciclo, fila* fila_io, fila* alta, fila* baixa, int tipo_io_id) {
    if (fila_io->tamanho == 0) return;

    int tempo_restante_no_ciclo = tempo_passado;

    while (tempo_restante_no_ciclo > 0 && fila_io->tamanho > 0) {
        processo* p_frente = &fila_io->fila_processos[fila_io->inicio];
        int tempo_para_finalizar = p_frente->tempo_restante_IO;

        if (tempo_para_finalizar <= tempo_restante_no_ciclo) {
            tempo_restante_no_ciclo -= tempo_para_finalizar;
            processo p = desenfileirar(fila_io);
            p.pcb_info.status = 0;

            const char* nome_fila_destino = "";
            if (tipo_io_id == 1) {
                enfileirar(baixa, p);
                nome_fila_destino = "BAIXA";
            } else {
                enfileirar(alta, p);
                nome_fila_destino = "ALTA";
            }
            int tempo_conclusao = tempo_atual_ciclo + (tempo_passado - tempo_restante_no_ciclo);
            printf("Tempo %d: PID %d terminou I/O de %s. Voltando para a fila de %s prioridade.\n",
                   tempo_conclusao, p.pcb_info.PID, dispositivos_IO[tipo_io_id], nome_fila_destino);

        } else {
            p_frente->tempo_restante_IO -= tempo_restante_no_ciclo;
            tempo_restante_no_ciclo = 0;
        }
    }
}

void gerenciar_e_avancar_IOs(int tempo_passado, int tempo_atual_ciclo, fila* alta, fila* baixa, fila* io_disco, fila* io_fita, fila* io_impressora) {
    processa_uma_fila_io(tempo_passado, tempo_atual_ciclo, io_disco, alta, baixa, 1);
    processa_uma_fila_io(tempo_passado, tempo_atual_ciclo, io_fita, alta, baixa, 2);
    processa_uma_fila_io(tempo_passado, tempo_atual_ciclo, io_impressora, alta, baixa, 3);
}


// Lógica Principal
int main() {
    fila prioridade_alta, prioridade_baixa, io_disco, io_fita, io_impressora;
    inicializar_fila(&prioridade_alta);
    inicializar_fila(&prioridade_baixa);
    inicializar_fila(&io_disco);
    inicializar_fila(&io_fita);
    inicializar_fila(&io_impressora);

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
            if (todos_os_processos[i].pcb_info.PID != -1 && todos_os_processos[i].tempo_chegada <= tempo_simulacao) {
                printf("Tempo %d: Chegou o PID %d.\n", todos_os_processos[i].tempo_chegada, todos_os_processos[i].pcb_info.PID);
                enfileirar(&prioridade_alta, todos_os_processos[i]);
                todos_os_processos[i].pcb_info.PID = -1;
            }
        }

        int tempo_de_execucao = CPU_executa(&prioridade_alta, &prioridade_baixa, &io_disco, &io_fita, &io_impressora, &processos_finalizados, tempo_simulacao);

        gerenciar_e_avancar_IOs(tempo_de_execucao, tempo_simulacao, &prioridade_alta, &prioridade_baixa, &io_disco, &io_fita, &io_impressora);

        printf("Estado no fim do ciclo que comecou em %d: \n", tempo_simulacao);
        printf("\tFila Alta: "); printa_fila(&prioridade_alta); printf("\n");
        printf("\tFila Baixa: "); printa_fila(&prioridade_baixa); printf("\n");
        printf("\tFila Disco: "); printa_fila(&io_disco); printf("\n");
        printf("\tFila Fita: "); printa_fila(&io_fita); printf("\n");
        printf("\tFila Impressora: "); printa_fila(&io_impressora); printf("\n");
        printf("---------------------------------------------\n");

        // Atualiza o tempo anterior antes de pular
        tempo_simulacao += tempo_de_execucao;
    }

    printf("\n--- FIM DA SIMULACAO ---\n");
    printf("Simulacao finalizada em %d unidades de tempo.\n", tempo_simulacao);
    printf("Total de processos finalizados: %d\n", processos_finalizados);

    return 0;
}