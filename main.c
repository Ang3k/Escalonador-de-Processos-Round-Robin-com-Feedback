#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include <stdarg.h>
#include "fila.h"

// Definindo variáveis e constantes importantes
#define max_processos    10 
#define quantum          5
#define TEMPO_DISCO      3
#define TEMPO_FITA       5
#define TEMPO_IMPRESSORA 7
#define MAX_EVENTOS      256      // eventos por ciclo

// Contador para criar os PID´s e outro para ir acumulando o tempo de chegada
int contador_global = 0;
int tempo_chegada_acumulado = 0;

typedef struct {
    int instante;        // instante do evento
    int sequencia;       // ordem de inserção (para desempate)
    char texto[256];     // mensagem pronta a imprimir
} Evento;

// Buffer global para armazenar os eventos de um ciclo
static Evento buffer_eventos[MAX_EVENTOS];
static int contador_eventos = 0;
static int sequencia_eventos = 0;

// Adiciona um evento ao log para impressão ordenada no tempo
static void adicionar_evento(int instante_atual, const char* formato, ...){
    if (contador_eventos >= MAX_EVENTOS) return; // Evita estourar o buffer
    va_list lista_args;
    va_start(lista_args, formato);
    vsnprintf(buffer_eventos[contador_eventos].texto, sizeof(buffer_eventos[contador_eventos].texto), formato, lista_args);
    va_end(lista_args);
    buffer_eventos[contador_eventos].instante = instante_atual;
    buffer_eventos[contador_eventos].sequencia = sequencia_eventos++;
    contador_eventos++;
}

// Função de comparação para ordenar os eventos
static int comparar_evento(const void* evento_a, const void* evento_b){
    const Evento* ptr_evento_x = (const Evento*)evento_a;
    const Evento* ptr_evento_y = (const Evento*)evento_b;
    // Compara primeiro pelo instante do tempo
    if (ptr_evento_x->instante != ptr_evento_y->instante) return ptr_evento_x->instante - ptr_evento_y->instante;
    // Se o tempo for igual, usa a ordem de inserção como desempate
    return ptr_evento_x->sequencia - ptr_evento_y->sequencia;
}

// Vetor para printar informações dos dispositivos
static const char* dispositivos_IO[] = {
    "Nenhum", "Disco", "Fita Magnetica", "Impressora"
};

// Protótipos das Funções
processo criadora_processos(void);
int carregar_processos_csv(const char* nome_arquivo, processo lista_de_processos[], int capacidade_da_lista);
void printa_lista_de_processos(processo lista_de_processos[], int total_processos);
int CPU_executa(fila* fila_alta, fila* fila_baixa, fila* fila_disco, fila* fila_fita, fila* fila_impressora, int* processos_finalizados, int tempo_atual, processo* processo_preemptado);
void avancar_fila_io(fila* fila_io, int duracao_ciclo, int instante_inicio_ciclo, const char* nome_dispositivo, fila* fila_alta, fila* fila_baixa, int destino_eh_fila_alta);
void gerenciar_dispositivos_io(int duracao_ciclo_cpu, int instante_inicio_ciclo, fila* fila_alta, fila* fila_baixa, fila* fila_disco, fila* fila_fita, fila* fila_impressora);

// Função para criação dos processos
processo criadora_processos(void)
{
    processo novo_processo = {0}; // Zera a estrutura para um novo processo
    // Informações básicas da PCB
    novo_processo.pcb_info.PID = contador_global++;
    novo_processo.pcb_info.PPID = 1;
    // Atribui um tempo de chegada crescente
    tempo_chegada_acumulado += (rand() % 4 + 1);
    novo_processo.tempo_chegada = tempo_chegada_acumulado;
    // Define um número aleatório de operações de I/O
    novo_processo.num_total_IO = rand() % 3;
    int acumulador_tempo_io = 0;
    // Define quando e qual I/O vai acontecer
    for (int i = 0; i < novo_processo.num_total_IO; ++i) {
        acumulador_tempo_io += (rand() % 6) + 2;
        novo_processo.gatilhos_IO[i] = acumulador_tempo_io;
        novo_processo.tipo_IO[i] = (rand() % 3) + 1;
    }
    // Garante tempo de serviço suficiente para rodar e fazer todos os I/O's
    novo_processo.tempo_servico   = acumulador_tempo_io + (rand() % 10) + 5;
    novo_processo.tempo_inicio_IO = -1; // Processo ainda não iniciou I/O
    return novo_processo;
}

// Função carregadora de arquivos csv
int carregar_processos_csv(const char* nome_arquivo, processo lista_de_processos[], int capacidade_da_lista)
{
    FILE* arquivo_ponteiro = fopen(nome_arquivo, "r");
    if (!arquivo_ponteiro) return -1; // Retorna erro se não conseguir abrir o arquivo

    char linha[512]; int total_processos = 0;
    fgets(linha, sizeof linha, arquivo_ponteiro); // Pula o cabeçalho do CSV
    // CORREÇÃO: Usando o novo nome do parâmetro na condição do loop
    while (fgets(linha, sizeof linha, arquivo_ponteiro) && total_processos < capacidade_da_lista) {
        processo processo_atual = {0}; // Zera a estrutura antes de preencher
        // Separa a linha em partes (tokens) usando a vírgula
        char* token = strtok(linha, ","); if (!token) continue;
        processo_atual.pcb_info.PID = atoi(token); contador_global = processo_atual.pcb_info.PID + 1;
        token = strtok(NULL, ","); if (!token) continue; processo_atual.pcb_info.PPID = atoi(token);
        token = strtok(NULL, ","); if (!token) continue; processo_atual.tempo_chegada = atoi(token);
        token = strtok(NULL, ","); if (!token) continue; processo_atual.tempo_servico = atoi(token);
        token = strtok(NULL, ","); if (!token) continue; processo_atual.num_total_IO   = atoi(token);
        // Lê os gatilhos e tipos de I/O
        for (int i = 0; i < processo_atual.num_total_IO && i < 3; ++i) {
            token = strtok(NULL, ","); if (!token) break;
            processo_atual.gatilhos_IO[i] = atoi(token);
            token = strtok(NULL, ","); if (!token) break;
            processo_atual.tipo_IO[i]     = atoi(token);
        }
        processo_atual.tempo_inicio_IO = -1; // Inicializa outros campos importantes
        lista_de_processos[total_processos++] = processo_atual; // Adiciona o processo lido na lista
    }
    fclose(arquivo_ponteiro);
    return total_processos;
}

// Função para printar as informações principais dos processos
void printa_lista_de_processos(processo lista_de_processos[], int total_processos)
{
    puts("\nPID | Chegada | Servico | IOs | Dispositivos");
    puts("---------------------------------------------");
    for (int i = 0; i < total_processos; ++i) {
        printf("%2d  | %4d    | %4d    | %2d  | ", lista_de_processos[i].pcb_info.PID, lista_de_processos[i].tempo_chegada, lista_de_processos[i].tempo_servico, lista_de_processos[i].num_total_IO);
        // Imprime os detalhes de cada operação de I/O
        for (int j = 0; j < lista_de_processos[i].num_total_IO; ++j) {
            printf("%s: %d", dispositivos_IO[lista_de_processos[i].tipo_IO[j]], lista_de_processos[i].gatilhos_IO[j]);
            if (j < lista_de_processos[i].num_total_IO - 1) printf(", ");
        }
        puts(lista_de_processos[i].num_total_IO ? "" : "Nenhuma");
    }
    puts("---------------------------------------------\n");
}

// Função que simula as decisões da CPU usando as filas
int CPU_executa(fila* fila_alta, fila* fila_baixa, fila* fila_disco, fila* fila_fita, fila* fila_impressora, int* processos_finalizados, int tempo_atual, processo* processo_preemptado){
    // Inicia o processo preemptado como inválido para este ciclo
    processo_preemptado->pcb_info.PID = -1;

    // Se não há processos prontos, a CPU fica ociosa
    if (!fila_alta->tamanho && !fila_baixa->tamanho) {
        adicionar_evento(tempo_atual, "CPU Ociosa...");
        return 1;
    }

    // Decide qual processo executar com base na prioridade (primeiro a fila alta)
    processo processo_em_execucao = fila_alta->tamanho ? desenfileirar(fila_alta): desenfileirar(fila_baixa);
    adicionar_evento(tempo_atual, "CPU <- PID %d (Servico restante: %d)", processo_em_execucao.pcb_info.PID, processo_em_execucao.tempo_servico);

    int tempo_executado = 0;
    // Executa o processo, limitado pelo quantum ou pelo seu tempo de serviço restante
    while (tempo_executado < quantum && processo_em_execucao.tempo_servico > 0) {
        // Decrementa o tempo de serviço e incrementa o tempo de CPU usado
        processo_em_execucao.tempo_servico--; processo_em_execucao.tempo_CPU_usado++; tempo_executado++;

        // Verifica se uma operação de I/O deve ser iniciada
        if (processo_em_execucao.proximo_IO < processo_em_execucao.num_total_IO &&
            processo_em_execucao.tempo_CPU_usado == processo_em_execucao.gatilhos_IO[processo_em_execucao.proximo_IO]) {

            int tipo_io = processo_em_execucao.tipo_IO[processo_em_execucao.proximo_IO++];
            int instante_evento = tempo_atual + tempo_executado;
            adicionar_evento(instante_evento, "PID %d solicitou I/O de %s.", processo_em_execucao.pcb_info.PID, dispositivos_IO[tipo_io]);

            // Determina a fila de I/O e a duração da operação
            fila* fila_io_destino = (tipo_io == 1) ? fila_disco : (tipo_io == 2 ? fila_fita : fila_impressora);
            int duracao_io = (tipo_io == 1) ? TEMPO_DISCO : (tipo_io == 2) ? TEMPO_FITA  : TEMPO_IMPRESSORA;

            // Prepara o processo para a fila de I/O
            processo_em_execucao.tempo_restante_IO = duracao_io;
            processo_em_execucao.tempo_inicio_IO = instante_evento;
            enfileirar(fila_io_destino, processo_em_execucao);
            // Se o dispositivo estava ocioso, marca o início do I/O para agora
            if (fila_io_destino->tamanho == 1)
                fila_io_destino->fila_processos[fila_io_destino->inicio].tempo_inicio_IO = instante_evento;

            return tempo_executado; // Processo sai da CPU para aguardar I/O
        }
    }

    // Verifica o estado do processo ao sair da CPU
    if (processo_em_execucao.tempo_servico == 0) {
        // Processo terminou sua execução
        adicionar_evento(tempo_atual + tempo_executado, "PID %d TERMINOU.", processo_em_execucao.pcb_info.PID);
        (*processos_finalizados)++;

    } else {
        // Processo sofreu preempção (fim do quantum)
        adicionar_evento(tempo_atual + tempo_executado, "PID %d sofreu PREEMPCAO. Falta %d de servico.", processo_em_execucao.pcb_info.PID, processo_em_execucao.tempo_servico);
        *processo_preemptado = processo_em_execucao; // Retorna o processo preemptado pela variável de ponteiro
    }
    // Retorna o tempo que a CPU ficou ocupada, ou 1 para o tempo da simulação avançar
    return tempo_executado ? tempo_executado : 1;
}

// Processa uma única fila de I/O
void avancar_fila_io(fila* fila_io, int duracao_ciclo, int instante_inicio_ciclo, const char* nome_dispositivo, fila* fila_alta, fila* fila_baixa, int destino_eh_fila_alta)
{
    int tempo_restante_ciclo = duracao_ciclo, tempo_atual_na_fila = instante_inicio_ciclo;
    // Processa a fila enquanto houver tempo disponível no ciclo e processos na fila
    while (tempo_restante_ciclo > 0 && fila_io->tamanho) {
        processo* processo_na_fila = &fila_io->fila_processos[fila_io->inicio];

        // Se o I/O do processo atual só pode começar mais tarde, avança o tempo
        if (processo_na_fila->tempo_inicio_IO > tempo_atual_na_fila) {
            int tempo_de_espera = processo_na_fila->tempo_inicio_IO - tempo_atual_na_fila;
            if (tempo_de_espera >= tempo_restante_ciclo) break; // Não há tempo suficiente neste ciclo
            tempo_atual_na_fila += tempo_de_espera;
            tempo_restante_ciclo -= tempo_de_espera;
        }

        // Calcula quanto tempo de I/O pode ser processado neste ciclo
        int tempo_processado_io = (processo_na_fila->tempo_restante_IO < tempo_restante_ciclo) ? processo_na_fila->tempo_restante_IO : tempo_restante_ciclo;

        // Atualiza os tempos
        processo_na_fila->tempo_restante_IO -= tempo_processado_io;
        tempo_atual_na_fila += tempo_processado_io;
        tempo_restante_ciclo -= tempo_processado_io;

        // Se a operação de I/O do processo terminou
        if (processo_na_fila->tempo_restante_IO == 0) {
            processo processo_concluido = desenfileirar(fila_io);
            // Se havia outro processo esperando, ele pode começar agora
            if (fila_io->tamanho) fila_io->fila_processos[fila_io->inicio].tempo_inicio_IO = tempo_atual_na_fila;

            // Devolve o processo para a fila de prontos apropriada (alta ou baixa)
            if (destino_eh_fila_alta) enfileirar(fila_alta, processo_concluido);
            else enfileirar(fila_baixa, processo_concluido);

            adicionar_evento(tempo_atual_na_fila, "PID %d terminou I/O de %s. Voltando para fila %s.", processo_concluido.pcb_info.PID, nome_dispositivo, destino_eh_fila_alta ? "ALTA" : "BAIXA");
        }
    }
}

// Atualiza o estado de todas as filas de I/O
void gerenciar_dispositivos_io(int duracao_ciclo_cpu, int instante_inicio_ciclo, fila* fila_alta, fila* fila_baixa, fila* fila_disco, fila* fila_fita, fila* fila_impressora){
    // Processa a fila de disco, processos retornam para a fila de baixa prioridade
    avancar_fila_io(fila_disco, duracao_ciclo_cpu, instante_inicio_ciclo, "Disco", fila_alta, fila_baixa, 0);
    // Processa a fila de fita, processos retornam para a fila de alta prioridade
    avancar_fila_io(fila_fita, duracao_ciclo_cpu, instante_inicio_ciclo, "Fita Magnetica", fila_alta, fila_baixa, 1);
    // Processa a fila de impressora, processos retornam para a fila de alta prioridade
    avancar_fila_io(fila_impressora, duracao_ciclo_cpu, instante_inicio_ciclo, "Impressora", fila_alta, fila_baixa, 1);
}

int main(int argc, char* argv[])
{
    // Prepara o gerador de números aleatórios
    srand(time(NULL));

    // Inicializa todas as filas do sistema
    fila fila_prioridade_alta, fila_prioridade_baixa, fila_disco, fila_fita, fila_impressora;
    inicializar_fila(&fila_prioridade_alta);
    inicializar_fila(&fila_prioridade_baixa);
    inicializar_fila(&fila_disco);
    inicializar_fila(&fila_fita);
    inicializar_fila(&fila_impressora);

    // Cria a lista para armazenar todos os processos
    processo lista_todos_processos[max_processos];
    int num_total_processos;

    // Tenta carregar processos de um arquivo CSV se fornecido
    if (argc > 1) {
        num_total_processos = carregar_processos_csv(argv[1], lista_todos_processos, max_processos);
        // Se o carregamento falhar, gera processos aleatórios
        if (num_total_processos <= 0) {
            printf("Erro ao carregar CSV. Gerando %d processos aleatórios.\n", max_processos);
            num_total_processos = max_processos;
            for (int i = 0; i < num_total_processos; ++i) lista_todos_processos[i] = criadora_processos();
        }
    } else {
        // Se nenhum arquivo foi passado, gera processos aleatórios
        num_total_processos = max_processos;
        for (int i = 0; i < num_total_processos; ++i) lista_todos_processos[i] = criadora_processos();
    }

    // Mostra a lista de processos que será simulada
    printa_lista_de_processos(lista_todos_processos, num_total_processos);

    // Variáveis para o controle do tempo na simulação
    int tempo_simulacao = 0, processos_finalizados = 0;
    printf("--- INICIO (Quantum = %d) ---\n", quantum);

    // Loop principal da simulação, continua até todos os processos terminarem
    while (processos_finalizados < num_total_processos) {
        // Zera o buffer de eventos para o novo ciclo
        contador_eventos = sequencia_eventos = 0;

        // Verifica a chegada de novos processos no instante atual
        for (int i = 0; i < num_total_processos; ++i)
            if (lista_todos_processos[i].pcb_info.PID != -1 && lista_todos_processos[i].tempo_chegada <= tempo_simulacao) {
                adicionar_evento(lista_todos_processos[i].tempo_chegada, "Chegou o PID %d.", lista_todos_processos[i].pcb_info.PID);
                enfileirar(&fila_prioridade_alta, lista_todos_processos[i]);
                lista_todos_processos[i].pcb_info.PID = -1; // Marca o processo para não ser adicionado de novo
            }

        // Executa um processo da CPU e obtém o tempo gasto
        processo processo_retornado_preemptado;
        int tempo_gasto_cpu = CPU_executa(&fila_prioridade_alta, &fila_prioridade_baixa, &fila_disco, &fila_fita, &fila_impressora, &processos_finalizados, tempo_simulacao, &processo_retornado_preemptado);

        // Gerencia as filas de I/O com base no tempo que a CPU gastou
        gerenciar_dispositivos_io(tempo_gasto_cpu, tempo_simulacao, &fila_prioridade_alta, &fila_prioridade_baixa, &fila_disco, &fila_fita, &fila_impressora);

        // Reenfileira o processo que sofreu preempção, se houver
        if (processo_retornado_preemptado.pcb_info.PID != -1) enfileirar(&fila_prioridade_baixa, processo_retornado_preemptado);

        // Calcula o instante final deste ciclo de execução
        int instante_fim_ciclo = tempo_simulacao + tempo_gasto_cpu;

        // Verifica chegadas que ocorreram durante a execução da CPU
        for (int i = 0; i < num_total_processos; ++i)
            if (lista_todos_processos[i].pcb_info.PID != -1 &&
                lista_todos_processos[i].tempo_chegada > tempo_simulacao &&
                lista_todos_processos[i].tempo_chegada <= instante_fim_ciclo) {
                adicionar_evento(lista_todos_processos[i].tempo_chegada, "Chegou o PID %d.", lista_todos_processos[i].pcb_info.PID);
                enfileirar(&fila_prioridade_alta, lista_todos_processos[i]);
                lista_todos_processos[i].pcb_info.PID = -1;
            }

        // Ordena e imprime todos os eventos que ocorreram no ciclo
        qsort(buffer_eventos, contador_eventos, sizeof(Evento), comparar_evento);
        for (int i = 0; i < contador_eventos; ++i)
            printf("Tempo %d: %s\n", buffer_eventos[i].instante, buffer_eventos[i].texto);

        // Imprime o resumo das filas no final do ciclo
        printf("Estado no fim do ciclo (Tempo %d):\n", instante_fim_ciclo);
        printf("\tFila Alta: ");       printa_fila(&fila_prioridade_alta);      puts("");
        printf("\tFila Baixa: ");      printa_fila(&fila_prioridade_baixa);     puts("");
        printf("\tFila Disco: ");      printa_fila(&fila_disco);     puts("");
        printf("\tFila Fita: ");       printa_fila(&fila_fita);      puts("");
        printf("\tFila Impressora: "); printa_fila(&fila_impressora); puts("");
        puts("---------------------------------------------");

        // Atualiza o tempo da simulação para o próximo ciclo
        tempo_simulacao = instante_fim_ciclo;
    }

    // Mensagem de fim da simulação
    printf("--- FIM DA SIMULACAO (%d unidades de tempo) ---\n", tempo_simulacao);
    return 0;
}