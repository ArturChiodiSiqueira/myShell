# MyShell: Um Shell Básico em C

## 1. Visão Geral

MyShell é um interpretador de linha de comando (shell) para sistemas operacionais do tipo Unix (como o Linux). Desenvolvido em linguagem C, este projeto tem como objetivo replicar as funcionalidades essenciais de shells conhecidos como o `bash`, servindo como um estudo prático sobre os mecanismos de criação e gerenciamento de processos, chamadas de sistema e manipulação de arquivos no Linux.

## 2. Funcionalidades Implementadas

* **Loop de Prompt Interativo**: O shell opera em um ciclo contínuo, exibindo um prompt customizável, aguardando o comando do usuário, executando-o e repetindo o processo.
* **Prompt Colorido**: O prompt `MyShell>` é exibido na cor amarela para melhor diferenciação visual.
* **Execução de Programas Externos**: Capaz de executar qualquer programa disponível no `$PATH` do sistema (como `ls`, `gcc`, `pwd`, etc.), utilizando o padrão `fork()`-`execvp()`-`wait()`.
* **Comandos Embutidos (Built-in)**: Implementação de comandos que modificam o estado do próprio shell:
    * `cd [caminho]`: Altera o diretório de trabalho atual. Suporta mudança para um caminho específico, para o diretório `HOME` do usuário (com `cd` sozinho) e para o diretório pai (`cd ..`).
    * `exit`: Encerra a sessão do MyShell de forma limpa.

---

## 3. Arquitetura e Explicação do Código

O funcionamento do MyShell é baseado em um loop central que orquestra a leitura, análise e execução dos comandos.

### 3.1. O Loop Principal (`while(1)`)

A função `main` contém um loop infinito que representa o ciclo de vida do shell. A cada iteração, ele executa os seguintes passos:
1.  **Exibe o Prompt**: Usa `printf()` com códigos de escape ANSI para mostrar o prompt `MyShell> ` em amarelo. A função `fflush(stdout)` é chamada para garantir que o prompt apareça na tela imediatamente.
2.  **Lê o Comando**: Utiliza `fgets()` para ler a linha de comando inserida pelo usuário de forma segura, evitando estouros de buffer.
3.  **Analisa o Comando**: A linha lida é passada para a função `parse_comando`.
4.  **Executa o Comando**: Com base no comando analisado, decide se ele é um comando embutido ou um programa externo.

### 3.2. Análise de Comandos (`parse_comando`)

Esta função é responsável por "traduzir" a entrada do usuário em um formato que o sistema possa entender.
* **Entrada**: Uma string única, como `"ls -l /home"`.
* **Processo**:
    1.  Primeiro, o caractere de nova linha (`\n`), capturado pelo `fgets`, é removido do final da string.
    2.  Em seguida, a função `strtok` é usada para quebrar a string em "tokens" (pedaços), usando o espaço como delimitador.
    3.  Cada token é armazenado como um ponteiro em um array de strings (`char *args[]`).
* **Saída**: Um array de strings terminado por `NULL`, como `["ls", "-l", "/home", NULL]`. O `NULL` final é crucial, pois atua como um sinalizador para a função `execvp`, indicando o fim da lista de argumentos.

### 3.3. Execução de Comandos

A lógica de execução é dividida em duas categorias:

#### a) Comandos Embutidos (`cd` e `exit`)
Estes comandos são especiais porque precisam alterar o estado do **próprio processo do shell**. Se tentássemos executar `cd` em um processo filho, o diretório mudaria apenas para o filho, que morreria em seguida, e o shell pai permaneceria no mesmo lugar.
* `exit`: Simplesmente usa a instrução `break` para sair do loop `while(1)`, encerrando o programa.
* `cd`: Usa a chamada de sistema `chdir()`. Se nenhum argumento for fornecido, a variável de ambiente `HOME` é recuperada com `getenv("HOME")` para determinar o destino. Caso contrário, o argumento fornecido (`args[1]`) é usado como o caminho.

#### b) Programas Externos (O Padrão Fork-Exec-Wait)
Para todos os outros comandos, o MyShell utiliza o modelo clássico de gerenciamento de processos do Unix:
1.  **`fork()` - A Clonagem**: O processo principal do shell (o Pai) cria uma cópia exata de si mesmo, gerando um novo processo (o Filho).
2.  **O Trabalho do Filho (`pid == 0`)**: O processo filho executa a chamada de sistema `execvp(args[0], args)`. Esta função **substitui** o código do processo filho pelo código do programa que se deseja executar (ex: `ls`). Se `execvp` for bem-sucedido, o código do MyShell não é mais executado pelo filho; se falhar (ex: comando não encontrado), ele retorna `-1`, um erro é impresso, e o filho encerra com `exit(EXIT_FAILURE)`.
3.  **O Trabalho do Pai (`pid > 0`)**: O processo pai, enquanto isso, executa a chamada `wait(&status)`. Isso faz com que o pai **pause e espere** até que o processo filho tenha terminado completamente sua execução.
4.  **Repetição**: Uma vez que o filho termina e o pai é "acordado" pelo `wait`, o loop `while(1)` continua, e o prompt é exibido novamente, pronto para o próximo comando.

---

## 4. Como Compilar e Executar

**Pré-requisitos:**
* Um ambiente Linux (como o Ubuntu rodando no WSL).
* O compilador GCC e ferramentas de build (`sudo apt install build-essential`).

**Passos:**
1.  Salve o código-fonte em um arquivo chamado `myShell.c`.
2.  Abra o terminal do Linux e navegue até a pasta onde o arquivo foi salvo.
3.  Compile o programa com o seguinte comando:
    ```bash
    gcc -o myshell myShell.c -Wall
    ```
4.  Execute o seu novo shell:
    ```bash
    ./myshell
    ```

---

## 5. Comandos de Teste

Após executar `./myshell`, você pode usar os seguintes comandos para testar as funcionalidades:

### Testes Básicos de Execução
```bash
ls
pwd
whoami
date
---------------------------------------------------------------
ls -la
echo "Meu shell funciona!"
touch arquivo_teste.txt  # Cria um arquivo
ls                       # Verifica se o arquivo foi criado
rm arquivo_teste.txt     # Remove o arquivo
---------------------------------------------------------------
pwd                      # Mostra o diretório inicial
mkdir pasta_teste
cd pasta_teste
pwd                      # Verifica se o diretório mudou
cd ..
pwd                      # Verifica se voltou ao diretório pai
cd                       # Volta para o diretório HOME
pwd
exit                     # Encerra o MyShell