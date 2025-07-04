#include "fila.h"
#include <stdio.h>

void inicializar_fila(fila* f)
{
    f->inicio = 0;
    f->fim = 0;
    f->tamanho = 0;
}

void enfileirar(fila* f, processo p)
{
    if (f->tamanho >= max_processos) return;
    f->fila_processos[f->fim] = p;
    f->fim = (f->fim + 1) % max_processos;
    f->tamanho++;
}

processo desenfileirar(fila* f)
{
    if (f->tamanho == 0) {
        processo vazio = {0};
        vazio.pcb_info.PID = -1;
        return vazio;
    }
    processo p = f->fila_processos[f->inicio];
    f->inicio = (f->inicio + 1) % max_processos;
    f->tamanho--;
    return p;
}

void printa_fila(fila* f)
{
    if (f->tamanho == 0) {
        printf("[vazia]");
        return;
    }
    int i = f->inicio, c = 0;
    while (c < f->tamanho) {
        printf("[PID:%d] ", f->fila_processos[i].pcb_info.PID);
        i = (i + 1) % max_processos;
        c++;
    }
}