#include <iostream>
#include <fstream>
#include <string>
#include <map>
#include <vector>
#include <cctype>
#include <iomanip> //deixar a tabela de saída alinhada

using namespace std;

//1. DEFINIÇÃO DOS CÓDIGOS DOS TOKENS
//Usamos um enum para facilitar a identificação de cada token no código
enum TokenType {
    TOK_EOF = 0,      
    TOK_ERRO,         

    //Palavras Reservadas da linguagem
    TOK_INICIO, TOK_FIM, TOK_SE, TOK_ENTAO, TOK_SENAO, TOK_FIMSE,
    TOK_PARA, TOK_ATE, TOK_PASSO, TOK_FIMPARA,
    TOK_INTEIRO_TYPE, TOK_LEIA, TOK_IMPRIMA,

    //Tipos de Dados e Identificadores
    TOK_ID,          
    TOK_NUM_INT,     
    TOK_LITERAL,      

    //Operadores Aritméticos e Lógicos
    TOK_MAIS, TOK_MENOS, TOK_MULT, TOK_DIV,
    TOK_E, TOK_OU, TOK_NAO,

    //Operadores Relacionais e Atribuição
    TOK_MAIOR, TOK_MENOR, TOK_MAIOR_IGUAL, TOK_MENOR_IGUAL, 
    TOK_IGUAL_REL, TOK_DIFERENTE,
    TOK_ATRIBUICAO,  

    //Pontuação/Delimitadores
    TOK_ABRE_PAR, TOK_FECHA_PAR, TOK_PONTO_VIRGULA, TOK_DOIS_PONTOS
};

//ESTRUTURA DO TOKEN
struct Token {
    TokenType tipo;     
    string lexema;      
    int linha;          
    int indiceTabela;  
};

//CLASSE DO ANALISADOR LÉXICO
class AnalisadorLexico {
private:
    ifstream arquivo;
    int linhaAtual;
    
    //Mapa para busca rápida de palavras reservadas (Complexidade O(log n))
    map<string, TokenType> palavrasReservadas;
    
    //Tabela de Símbolos: Armazena apenas os nomes das variáveis criadas pelo usuário
    vector<string> tabelaSimbolos; 

    //Configura o mapa com as palavras da linguagem
    void inicializarTabela() {
        palavrasReservadas["inicio"] = TOK_INICIO;
        palavrasReservadas["fim"] = TOK_FIM;
        palavrasReservadas["se"] = TOK_SE;
        // Tratamento para aceitar versões com e sem acento (robustez)
        palavrasReservadas["entao"] = TOK_ENTAO; palavrasReservadas["então"] = TOK_ENTAO;
        palavrasReservadas["senao"] = TOK_SENAO; palavrasReservadas["senão"] = TOK_SENAO;
        palavrasReservadas["fim_se"] = TOK_FIMSE;
        palavrasReservadas["para"] = TOK_PARA;
        palavrasReservadas["ate"] = TOK_ATE; palavrasReservadas["até"] = TOK_ATE;
        palavrasReservadas["passo"] = TOK_PASSO;
        palavrasReservadas["fim_para"] = TOK_FIMPARA;
        palavrasReservadas["inteiro"] = TOK_INTEIRO_TYPE;
        palavrasReservadas["leia"] = TOK_LEIA;
        palavrasReservadas["imprima"] = TOK_IMPRIMA;
        palavrasReservadas["escreva"] = TOK_IMPRIMA; // Sinônimo aceito
        palavrasReservadas["E"] = TOK_E;
        palavrasReservadas["OU"] = TOK_OU;
        palavrasReservadas["NAO"] = TOK_NAO;
    }

    //Adiciona uma nova variável na tabela de símbolos se ela ainda não existir
    int gerenciarTabelaSimbolos(string lexema) {
        for (size_t i = 0; i < tabelaSimbolos.size(); i++) {
            if (tabelaSimbolos[i] == lexema) return i; 
        }
        tabelaSimbolos.push_back(lexema); //Nova variável
        return tabelaSimbolos.size() - 1;
    }

public:
    AnalisadorLexico(string nomeArquivo) {
        arquivo.open(nomeArquivo);
        linhaAtual = 1;
        inicializarTabela();
        
        if (!arquivo.is_open()) {
            cout << "Erro: Nao foi possivel abrir o arquivo '" << nomeArquivo << "'" << endl;
            exit(1);
        }
    }

    ~AnalisadorLexico() {
        if (arquivo.is_open()) arquivo.close();
    }

    //acessa a tabela de símbolos final para impressão na main
    vector<string> getTabelaSimbolos() {
        return tabelaSimbolos;
    }

    //FUNÇÃO PRINCIPAL: O AUTOMATO FINITO
    Token proximoToken() {
        char car;

        //1. Ignorar espaços em branco e contar linhas
        while (arquivo.get(car)) {
            if (car == '\n') linhaAtual++;
            else if (!isspace(car)) break; 
        }

        //Verifica se o arquivo acabou
        if (arquivo.eof()) return {TOK_EOF, "EOF", linhaAtual, -1};

        //2. Autômato para Identificadores e Palavras Reservadas (Começa com Letra)
        if (isalpha(car)) {
            string lexema = "";
            lexema += car;

            //continua lendo enquanto for letra, número ou underline
            while (arquivo.get(car)) {
                if (isalnum(car) || car == '_' || (unsigned char)car > 127) {
                    lexema += car;
                } else {
                    arquivo.unget(); 
                    break;
                }
            }

            //Verifica: É palavra reservada ou variável?
            if (palavrasReservadas.count(lexema)) {
                return {palavrasReservadas[lexema], lexema, linhaAtual, -1};
            } else {
                //Se não é reservada, é um ID. Salva na tabela.
                int idx = gerenciarTabelaSimbolos(lexema);
                return {TOK_ID, lexema, linhaAtual, idx};
            }
        }

        //3. Autômato para Números Inteiros
        if (isdigit(car)) {
            string lexema = "";
            lexema += car;
            while (arquivo.get(car)) {
                if (isdigit(car)) {
                    lexema += car;
                } else {
                    arquivo.unget();
                    break;
                }
            }
            return {TOK_NUM_INT, lexema, linhaAtual, -1};
        }

        //4. Autômato para Literais (Strings entre aspas)
        if (car == '"') {
            string lexema = ""; 
            while (arquivo.get(car)) {
                if (car == '"') break; 
                lexema += car;
            }
            //Retorna com as aspas para indicar que é string
            return {TOK_LITERAL, "\"" + lexema + "\"", linhaAtual, -1}; 
        }

        //5. Autômato para Símbolos e Operadores (Tratamento de Ambiguidade)
        string lexema = string(1, car);
        switch (car) {
            case '+': return {TOK_MAIS, lexema, linhaAtual, -1};
            case '-': return {TOK_MENOS, lexema, linhaAtual, -1};
            case '*': return {TOK_MULT, lexema, linhaAtual, -1};
            case '/': return {TOK_DIV, lexema, linhaAtual, -1};
            case '(': return {TOK_ABRE_PAR, lexema, linhaAtual, -1};
            case ')': return {TOK_FECHA_PAR, lexema, linhaAtual, -1};
            case ';': return {TOK_PONTO_VIRGULA, lexema, linhaAtual, -1};
            case ':': return {TOK_DOIS_PONTOS, lexema, linhaAtual, -1};
            
            case '>': 
                //Lookahead: Espia o próximo para ver se é >=
                if (arquivo.peek() == '=') { 
                    arquivo.get(car); // Consome o '='
                    return {TOK_MAIOR_IGUAL, ">=", linhaAtual, -1}; 
                }
                return {TOK_MAIOR, ">", linhaAtual, -1};

            case '<':
                //Lookahead complexo: Pode ser <=, <- (atribuição) ou <> (diferente)
                if (arquivo.peek() == '=') {
                    arquivo.get(car);
                    return {TOK_MENOR_IGUAL, "<=", linhaAtual, -1};
                } else if (arquivo.peek() == '-') {
                    arquivo.get(car);
                    return {TOK_ATRIBUICAO, "<-", linhaAtual, -1};
                } else if (arquivo.peek() == '>') {
                    arquivo.get(car);
                    return {TOK_DIFERENTE, "<>", linhaAtual, -1};
                }
                return {TOK_MENOR, "<", linhaAtual, -1};

            case '=': return {TOK_IGUAL_REL, "=", linhaAtual, -1};

            default:
                return {TOK_ERRO, lexema, linhaAtual, -1};
        }
    }
};

//Função auxiliar que traduz o enum em texto na hora de imprimir
string nomeToken(TokenType t) {
    switch(t) {
        case TOK_EOF: return "EOF";
        case TOK_ERRO: return "ERRO";
        case TOK_ID: return "ID";
        case TOK_NUM_INT: return "CONST_INT";
        case TOK_LITERAL: return "CONST_LIT";
        case TOK_INICIO: return "KW_INICIO";
        case TOK_FIM: return "KW_FIM";
        case TOK_SE: return "KW_SE";
        case TOK_ENTAO: return "KW_ENTAO";
        case TOK_SENAO: return "KW_SENAO";
        case TOK_FIMSE: return "KW_FIM_SE";
        case TOK_PARA: return "KW_PARA";
        case TOK_ATE: return "KW_ATE";
        case TOK_PASSO: return "KW_PASSO";
        case TOK_FIMPARA: return "KW_FIM_PARA";
        case TOK_INTEIRO_TYPE: return "KW_INTEIRO"; 
        case TOK_LEIA: return "KW_LEIA";
        case TOK_IMPRIMA: return "KW_IMPRIMA";
        
        case TOK_MAIS: return "OP_SOMA";       
        case TOK_MENOS: return "OP_SUB";       
        case TOK_MULT: return "OP_MULT";       
        case TOK_DIV: return "OP_DIV";         
        
        case TOK_MAIOR: return "OP_REL_MAIOR";       
        case TOK_MENOR: return "OP_REL_MENOR";       
        case TOK_MAIOR_IGUAL: return "OP_MAIOR_IGUAL";
        case TOK_MENOR_IGUAL: return "OP_MENOR_IGUAL";
        case TOK_DIFERENTE: return "OP_DIFERENTE";   
        case TOK_IGUAL_REL: return "OP_REL_IGUAL";   
        case TOK_ATRIBUICAO: return "OP_ATRIBUICAO";
        
        case TOK_ABRE_PAR: return "DELIM_ABRE_PAR";  
        case TOK_FECHA_PAR: return "DELIM_FECHA_PAR";
        case TOK_PONTO_VIRGULA: return "DELIM_P_VIRGULA";
        case TOK_DOIS_PONTOS: return "DELIM_DOIS_PONTOS"; 
        
        default: return "TOKEN_OUTRO";
    }
}

//MAIN: (Simulador do Sintático)
int main() {
    //O arquivo teste.por deve estar na mesma pasta do executável
    AnalisadorLexico lexico("teste.por");
    Token token;

    cout << "=== ANALISADOR LEXICO - RESULTADO ===" << endl;
    cout << left << setw(10) << "LINHA" 
         << setw(20) << "TIPO TOKEN" 
         << setw(20) << "LEXEMA" 
         << setw(10) << "IND. TAB" << endl;
    cout << string(60, '-') << endl;

    //Loop principal: chama o próximo token até acabar o arquivo
    do {
        token = lexico.proximoToken();
        
        if (token.tipo != TOK_EOF) {
            cout << left << setw(10) << token.linha 
                 << setw(20) << nomeToken(token.tipo) 
                 << setw(20) << token.lexema;
            
            //Se for ID, mostra o índice na tabela de símbolos
            if (token.indiceTabela != -1) cout << setw(10) << token.indiceTabela;
            else cout << setw(10) << "-";
            
            cout << endl;
        }

    } while (token.tipo != TOK_EOF);

    //Tabela de Símbolos Final
    cout << "\n=== TABELA DE SIMBOLOS (VARIAVEIS) ===" << endl;
    vector<string> tab = lexico.getTabelaSimbolos();
    for (size_t i = 0; i < tab.size(); i++) {
        cout << "Indice " << i << ": " << tab[i] << endl;
    }

    return 0;
}