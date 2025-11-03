#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

#define TAMANHO_PAGINA 1024
#define NUMERO_QUADROS 4
#define NUMERO_PAGINAS 10
#define ENDERECO_MAXIMO (NUMERO_PAGINAS * TAMANHO_PAGINA - 1)

typedef struct {
    int id_quadro_fisico;
    bool presente;
    bool modificado;
    int tempo_carregamento;
} EntradaTabelaPaginas;

typedef struct {
    int id_pagina_virtual;
    char dados[TAMANHO_PAGINA];
} QuadroMemoriaFisica;

typedef struct {
    char dados[TAMANHO_PAGINA];
} PaginaArmazenamento;

EntradaTabelaPaginas tabela_paginas[NUMERO_PAGINAS];
QuadroMemoriaFisica quadros_memoria[NUMERO_QUADROS];
PaginaArmazenamento armazenamento_disco[NUMERO_PAGINAS];
int total_falhas_pagina = 0;
int tempo_global = 0;
int proximo_quadro_fifo = 0;

int obter_id_pagina(int endereco_virtual) {
    return endereco_virtual / TAMANHO_PAGINA;
}

void mostrar_estado_atual();
void escrever_no_disco(int id_pagina, char *dados_a_escrever);
void carregar_do_disco(int id_pagina, char *dados_a_carregar_em);
void tratar_falha_pagina(int id_pagina, char operacao, char valor, int offset);


bool acessar_memoria(int endereco_virtual, char operacao, char valor) {
    if (endereco_virtual < 0 || endereco_virtual > ENDERECO_MAXIMO) {
        printf("ERRO: Endereço virtual %d fora do intervalo.\n", endereco_virtual);
        return false;
    }

    int id_pagina = obter_id_pagina(endereco_virtual);
    int offset = endereco_virtual % TAMANHO_PAGINA;
    EntradaTabelaPaginas *entrada_pte = &tabela_paginas[id_pagina];

    printf("Acesso: Endereço %d (Página %d, Offset %d) | Operação: %c\n",
           endereco_virtual, id_pagina, offset, operacao);

    if (entrada_pte->presente) {
        int id_quadro = entrada_pte->id_quadro_fisico;
        printf("  [HIT] Página %d encontrada no Quadro %d.\n", id_pagina, id_quadro);

        if (operacao == 'W') {
            quadros_memoria[id_quadro].dados[offset] = valor;
            entrada_pte->modificado = true;
            printf("  Dados escritos. Quadro %d marcado como 'Modificado'.\n", id_quadro);
        } else {
            printf("  Dados lidos: '%c'\n", quadros_memoria[id_quadro].dados[offset]);
        }
        return true;

    } else {
        printf("  [FALHA DE PÁGINA] Página %d não está na memória.\n", id_pagina);
        total_falhas_pagina++;
        tratar_falha_pagina(id_pagina, operacao, valor, offset);
        return false;
    }
}

void inicializar_sistema() {
    printf("Inicializando Memória Virtual...\n");
    for (int i = 0; i < NUMERO_PAGINAS; i++) {
        tabela_paginas[i].id_quadro_fisico = -1;
        tabela_paginas[i].presente = false;
        tabela_paginas[i].modificado = false;
        tabela_paginas[i].tempo_carregamento = 0;

        for(int j = 0; j < TAMANHO_PAGINA; j++) {
            armazenamento_disco[i].dados[j] = (char)('A' + (i % 26));
        }
    }

    for (int i = 0; i < NUMERO_QUADROS; i++) {
        quadros_memoria[i].id_pagina_virtual = -1;
        memset(quadros_memoria[i].dados, 0, TAMANHO_PAGINA);
    }
    printf("Sistema Inicializado. Quadros disponíveis: %d\n\n", NUMERO_QUADROS);
}