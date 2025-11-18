# MyShell: Um Shell Básico em C

## 1. Visão Geral

MyShell é um interpretador de linha de comando (shell) para sistemas operacionais do tipo Linux. Desenvolvido em linguagem C, este projeto replica as funcionalidades essenciais e avançadas de shells conhecidos como o bash, servindo como um estudo prático sobre os mecanismos de criação e gerenciamento de processos, comunicação interprocessos (IPC) e manipulação de arquivos no Linux.

---

## 2. Funcionalidades Implementadas

### Módulo 1: Essencial

* *Loop de Prompt Interativo*: O shell opera em um ciclo contínuo, exibindo um prompt customizável (`MyShell>`) em cor amarela.
* *Execução de Programas Externos*: Capaz de executar qualquer programa disponível no $PATH do sistema (como ls, gcc, pwd, etc.), utilizando o padrão fork()-execvp()-wait().
* *Comandos Embutidos (Built-in)*:
    * cd [caminho]: Altera o diretório de trabalho atual. Suporta cd (para o HOME) e cd ...
    * exit: Encerra a sessão do MyShell de forma limpa.

### Módulo 2: Avançado

* Redirecionamento de Saída (>, >>):
    * comando > arquivo: Redireciona a saída padrão (stdout) para arquivo, sobrescrevendo-o.
    * comando >> arquivo: Redireciona a saída padrão (stdout) para arquivo, concatenando no final (append).
* Redirecionamento de Entrada (<):
    * comando < arquivo: Redireciona a entrada padrão (stdin) para ler de arquivo.
* Pipes (|):
    * comando1 | comando2: Encadeia dois comandos, onde a saída padrão de comando1 serve como entrada padrão para comando2.
* Execução em Background (&):
    * comando &: Executa o comando em segundo plano, liberando o prompt do shell imediatamente. O shell gerencia corretamente os processos filhos (zumbis) através de um handler para o sinal SIGCHLD.

---

## 3. Arquitetura e Explicação

O funcionamento do MyShell é baseado em um loop central (main) que orquestra a leitura, análise e execução dos comandos.

### 3.1. Análise de Comandos

1.  **parse_comando**: Realiza a tokenização inicial, separando a linha de comando por espaços.
2.  **analisar_e_executar**: É a central de lógica do Módulo 2. Ela varre os tokens em busca dos operadores especiais:
    * Se um pipe (|) é encontrado, ela divide os argumentos em dois (antes e depois do pipe) e chama executar_pipe.
    * Se não há pipe, ela varre os argumentos em busca de redirecionamento (<, >, >>) e background (&), armazenando seus alvos (ex: nome do arquivo) e limpando o array de argumentos. Em seguida, chama executar_comando_simples.

### 3.2. Execução de Comandos

A lógica de execução é dividida em três categorias:

* Comandos Embutidos (cd, exit): São tratados diretamente no loop main. Isso é necessário pois eles modificam o estado do próprio processo do shell (o diretório atual ou sua terminação).
* Execução Simples (executar_comando_simples): Esta função lida com todos os comandos que não são pipes. Ela cria *um* processo filho.
    * *No Filho*: Antes de chamar execvp, o processo filho verifica se há redirecionamentos. Se houver, ele usa open() para abrir os arquivos e dup2() para "religar" os descritores de arquivo padrão (STDIN, STDOUT) para que apontem para os arquivos abertos.
    * *No Pai*: O pai verifica a flag background. Se for false, ele chama waitpid() e espera o filho terminar. Se for true, ele imprime o PID do filho e continua imediatamente, sem esperar.
* Execução com Pipe (executar_pipe): Esta função implementa o cmd1 | cmd2. Ela cria *dois* processos filhos:
    1.  O Pai primeiro cria um pipe() (um canal de comunicação).
    2.  O Pai cria o *Filho 1* (pid1). O Filho 1 usa dup2() para redirecionar seu STDOUT para a ponta de escrita do pipe e então executa cmd1.
    3.  O Pai cria o *Filho 2* (pid2). O Filho 2 usa dup2() para redirecionar seu STDIN para a ponta de leitura do pipe e então executa cmd2.
    4.  O Pai fecha ambas as pontas do pipe (crucial!) e chama waitpid() duas vezes, esperando por ambos os filhos terminarem.

### 3.3. Gerenciamento de Processos Zumbis

Quando um processo em background (&) termina, ele se torna um "zumbi" até que o pai o "limpe" (usando wait). Para evitar o acúmulo de zumbis, registramos um signal handler (handle_sigchld) para o sinal SIGCHLD. O kernel envia este sinal ao pai sempre que um filho morre, e o handler usa waitpid(..., WNOHANG) para limpar o processo terminado.

---

## 4. Como Compilar e Executar

*Pré-requisitos:*
* Um ambiente Linux (como o Ubuntu rodando no WSL).
* O compilador GCC e ferramentas de build (sudo apt install build-essential).

*Passos:*
1.  Salve o código-fonte em um arquivo chamado myShell.c.
2.  Abra o terminal do Linux e navegue até a pasta onde o arquivo foi salvo.
3.  Compile o programa com o seguinte comando bash:
    ```gcc -o myshell myShell.c -Wall```
    
4.  Execute o seu novo shell no terminal:
    ```./myshell```
    

---

## 5. Cenários de Teste

### a) Testes do Módulo 1 (Funcionalidades Essenciais)

```bash
# Teste de comandos básicos
ls -la
pwd
whoami

# Teste de 'cd'
MyShell> pwd
/home/micaellisilva/myShell
MyShell> cd /tmp
MyShell> pwd
/tmp
MyShell> cd
MyShell> pwd
/home/micaellisilva

# Teste de 'exit'
MyShell> exit
```
### b) Testes do Módulo 2 (Funcionalidades Avançadas)
```bash

# --- Teste de Redirecionamento de Saída (>) ---
# Cria o arquivo 'lista.txt' e escreve a saída do 'ls -l' nele
MyShell> ls -l > lista.txt
# Verifica o conteúdo (deve mostrar a lista de arquivos)
MyShell> cat lista.txt
# Sobrescreve o arquivo 'lista.txt' com a saída do 'whoami'
MyShell> whoami > lista.txt
MyShell> cat lista.txt

# --- Teste de Redirecionamento de Saída (>>) ---
# Adiciona (append) a saída do 'date' ao final do 'lista.txt'
MyShell> date >> lista.txt
MyShell> cat lista.txt
# (O arquivo agora deve conter seu nome de usuário E a data)

# --- Teste de Redirecionamento de Entrada (<) ---
# Cria um arquivo com nomes para ordenar
MyShell> echo "banana" > nomes.txt
MyShell> echo "abacaxi" >> nomes.txt
MyShell> echo "laranja" >> nomes.txt
# Usa 'sort' para ler de 'nomes.txt' e imprimir ordenado
MyShell> sort < nomes.txt
abacaxi
banana
laranja

# --- Teste de Pipe (|) ---
# Encadeia 'ls -l' com 'grep', filtrando apenas o arquivo .c
MyShell> ls -l | grep ".c"
# Encadeia 'cat' com 'sort' para ordenar nosso arquivo de nomes
MyShell> cat nomes.txt | sort

# --- Teste de Execução em Background (&) ---
# Executa um comando longo (sleep 5) em background
MyShell> sleep 5 &
Comando [PID 1234] iniciado em background.
# O prompt deve voltar imediatamente
MyShell> ls
# (Enquanto o 'ls' roda, o 'sleep' continua em background)
# (Após 5s, o processo 'sleep' terminará e será limpo pelo handler)
```
