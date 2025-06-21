Claro, aqui está uma sugestão de arquivo `README.md` resumido.

-----

# Simulador de Escalonador de Processos

[cite\_start]Este projeto é uma implementação em C de um simulador de escalonador de processos, desenvolvido para a disciplina de Arquitetura de Computadores e Sistemas Operacionais[cite: 1]. [cite\_start]O objetivo é simular a alocação de CPU e o gerenciamento de filas de processos e de I/O[cite: 2].

## Algoritmo Implementado

[cite\_start]O simulador implementa o algoritmo **Round Robin com Feedback**, utilizando múltiplas filas de prioridade para gerenciar os processos[cite: 3]. As regras principais são:

  * [cite\_start]**Filas de Prontos**: O sistema utiliza uma fila de **alta prioridade** e uma de **baixa prioridade**[cite: 19].
  * [cite\_start]**Chegada de Processos**: Processos novos entram na fila de alta prioridade[cite: 22].
  * [cite\_start]**Preempção**: Um processo em execução é preemptado se seu tempo de uso da CPU excede o *quantum* definido, sendo então movido para a fila de baixa prioridade[cite: 24].
  * [cite\_start]**Operações de I/O**: Os processos podem solicitar operações de I/O para três tipos de dispositivos: Disco, Fita Magnética e Impressora[cite: 17]. Ao fazer isso, são movidos para a fila do respectivo dispositivo.
  * **Retorno de I/O**: O retorno para as filas de prontos depende do dispositivo.
      * [cite\_start]**Disco**: Retorna para a fila de baixa prioridade[cite: 20].
      * [cite\_start]**Fita Magnética e Impressora**: Retornam para a fila de alta prioridade[cite: 21].

## Estrutura dos Arquivos

  * **`main.c`**: Contém a lógica principal da simulação, o controle do tempo e o gerenciamento dos ciclos de execução.
  * **`fila.c`**: Implementa as funções para o gerenciamento das filas (enfileirar, desenfileirar, etc.).
  * **`fila.h`**: Arquivo de cabeçalho que define as estruturas de dados (`processo`, `fila`) e os protótipos das funções utilizadas no projeto.

## Como Compilar e Executar

1.  Para compilar o projeto, utilize o seguinte comando no terminal:

    ```bash
    gcc main.c fila.c -o main
    ```

2.  Para executar o simulador após a compilação:

    ```bash
    ./main
    ```
