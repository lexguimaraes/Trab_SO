# Simulador de Escalonamento

Trabalho prático de Sistemas Operacionais: simulador de processos com CPUs,
discos, memória principal, filas de prontos e modos de execução single-thread e
multi-thread.

## Build

```sh
make
```

O build também regenera automaticamente `compile_commands.json` para
`clangd`/LSP. Para atualizar apenas esse arquivo:

```sh
make compile_commands
```

## Execução

```sh
./simulador --modo single examples/processos.txt
```

## Entrada

O parser aceita o formato oficial descrito no enunciado:

```text
[id, cpu1, io, cpu2, memoria_mb]
```

Nesse formato, o processo entra como usuário, chega no tempo 0 e usa 1 disco
durante I/O quando `io > 0`.

Para testes mais completos, também são aceitos:

```text
id prioridade cpu1 io cpu2 memoria_mb
id prioridade chegada cpu1 io cpu2 memoria_mb discos
```

Prioridade `0` representa tempo real; prioridade `1` representa usuário.

## Estado Atual

Implementado:

- modo `single`;
- 4 CPUs simuladas;
- 4 discos simulados;
- processos de tempo real em FCFS sem quantum;
- processos de usuário com feedback de 3 filas e quantum 2;
- memória por blocos com first-fit e junção de blocos livres;
- logs de criação, despacho, bloqueio, preempção e finalização.

O modo `multi` será implementado usando os mesmos modelos do modo `single`,
com threads reais representando componentes do sistema, não processos do
usuário.
