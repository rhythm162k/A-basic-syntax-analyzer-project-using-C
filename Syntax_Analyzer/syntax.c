#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define MAX_HEIGHT 100
#define MAX_WIDTH  80
#define MAX_TOKENS 100

char treePrint[MAX_HEIGHT][MAX_WIDTH];

// Structure to store tokens
typedef struct {
    char type[20];  // Token type
    int line;       // Line number
    int position;   // Position in line
} Token;

Token tokens[MAX_TOKENS];
int tokenIndex = 0;
int totalTokens = 0;

// AST Node Structure   //Tree using Linked-List Data Structure
typedef struct ASTNode {
    char value[20];
    struct ASTNode *left, *right;
} ASTNode;

// Function Prototypes
void tokenizeAndStore(const char *inputFile, const char *tokenFile);
void readTokensFromFile(const char *filename);
void match(char *expected);
ASTNode* parseExpression();
ASTNode* parseTerm();
ASTNode* parseFactor();
ASTNode* parsePower();
ASTNode* parseAssignment();
ASTNode* createASTNode(char *value, ASTNode *left, ASTNode *right);
void printAST(ASTNode *node, int depth);
void fillPrintTree(ASTNode *node, int level, int start, int end);
int getHeight(ASTNode *node);
void writeOutputToFile(ASTNode *root, const char *filename);
void freeAST(ASTNode *node);

// Function to tokenize input expressions and store tokens in a file
void tokenizeAndStore(const char *inputFile, const char *tokenFile) {
    FILE *input = fopen(inputFile, "r");
    FILE *output = fopen(tokenFile, "w");

    if (!input || !output) {
        printf("Error opening file.\n");
        exit(1);
    }

    char ch;
    int line = 1, position = 1;

    while ((ch = fgetc(input)) != EOF) {
        if (ch == '\n') {
            fprintf(output, "; %d %d\n", line, position); // Add separator token
            line++;
            position = 1;
        } else if (strchr("+-*/=()^", ch)) {
            fprintf(output, "%c %d %d\n", ch, line, position);
            position++;
        } else if (ch >= 'a' && ch <= 'z') {
            fprintf(output, "id %d %d\n", line, position);
            position++;
        } else if (ch >= '0' && ch <= '9') {
            char number[20];
            int numPos = 0;
            int startPos = position;

            while (ch >= '0' && ch <= '9') {
                number[numPos++] = ch;
                ch = fgetc(input);
                position++;
            }

            number[numPos] = '\0';
            fprintf(output, "num %d %d\n", line, startPos);

            if (ch != EOF) ungetc(ch, input);
        } else if (ch != ' ' && ch != '\t') {
            printf("Lexical Error: Unknown character '%c' at line %d, position %d\n", ch, line, position);
            exit(1);
        } else {
            position++;
        }
    }

    // Add final separator if file doesn't end with newline
    if (position > 1) {
        fprintf(output, "; %d %d\n", line, position);
    }

    fclose(input);
    fclose(output);
}

// Function to read tokens from a file
void readTokensFromFile(const char *filename) {
    FILE *file = fopen(filename, "r");
    if (!file) {
        printf("Error opening file.\n");
        exit(1);
    }

    char type[20];
    int line, position;

    while (fscanf(file, "%s %d %d", type, &line, &position) == 3) {
        strcpy(tokens[totalTokens].type, type);
        tokens[totalTokens].line = line;
        tokens[totalTokens].position = position;
        totalTokens++;
    }

    fclose(file);

    if (totalTokens == 0) {
        printf("Error: The tokens file is empty. Please provide a valid input.\n");
        exit(1);
    }
}

// Function to create an AST node
ASTNode* createASTNode(char *value, ASTNode *left, ASTNode *right) {
    ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));
    if (!node) {
        printf("Error: Memory allocation failed.\n");
        exit(1);
    }
    strcpy(node->value, value);
    node->left = left;
    node->right = right;
    return node;
}

// Function to free an AST
void freeAST(ASTNode *node) {
    if (!node) return;
    freeAST(node->left);
    freeAST(node->right);
    free(node);
}

// Function to check if the current token matches the expected token
void match(char *expected) {
    if (tokenIndex < totalTokens && strcmp(tokens[tokenIndex].type, expected) == 0) {
        tokenIndex++;
    } else {
        printf("Syntax Error: Expected %s, found %s at line %d, position %d\n",
               expected, tokenIndex < totalTokens ? tokens[tokenIndex].type : "EOF",
               tokenIndex < totalTokens ? tokens[tokenIndex].line : -1,
               tokenIndex < totalTokens ? tokens[tokenIndex].position : -1);
        exit(1);
    }
}   

// Parses exponentiation (^)
ASTNode* parsePower() {
    ASTNode* node = parseFactor();

    while (tokenIndex < totalTokens && strcmp(tokens[tokenIndex].type, "^") == 0) {
        char op[2];
        strcpy(op, tokens[tokenIndex].type);
        match("^");
        node = createASTNode(op, node, parsePower());
    }

    return node;
}

// Parses multiplication and division (*, /)
ASTNode* parseTerm() {
    ASTNode *node = parsePower();

    while (tokenIndex < totalTokens && (strcmp(tokens[tokenIndex].type, "*") == 0 || strcmp(tokens[tokenIndex].type, "/") == 0)) {
        char op[2];
        strcpy(op, tokens[tokenIndex].type);
        match(op);
        node = createASTNode(op, node, parsePower());
    }

    return node;
}

// Parses addition and subtraction (+, -)
ASTNode* parseExpression() {
    ASTNode *node = parseTerm();

    while (tokenIndex < totalTokens && (strcmp(tokens[tokenIndex].type, "+") == 0 || strcmp(tokens[tokenIndex].type, "-") == 0)) {
        char op[2];
        strcpy(op, tokens[tokenIndex].type);
        match(op);
        node = createASTNode(op, node, parseTerm());
    }

    return node;
}

// Parses assignment (id = expression)
ASTNode* parseAssignment() {
    if (tokenIndex < totalTokens && strcmp(tokens[tokenIndex].type, "id") == 0) {
        ASTNode *left = createASTNode(tokens[tokenIndex].type, NULL, NULL);
        match("id");

        if (tokenIndex < totalTokens && strcmp(tokens[tokenIndex].type, "=") == 0) {
            match("=");
            ASTNode *right = parseExpression();
            return createASTNode("=", left, right);
        } else {
            return left;
        }
    }

    return parseExpression();
}

// Parses factors (id, num, or (expression))
ASTNode* parseFactor() {
    if (tokenIndex >= totalTokens) {
        printf("Syntax Error: Unexpected end of input.\n");
        exit(1);
    }

    if (strcmp(tokens[tokenIndex].type, "id") == 0 || strcmp(tokens[tokenIndex].type, "num") == 0) {
        ASTNode *node = createASTNode(tokens[tokenIndex].type, NULL, NULL);
        char *currentType = tokens[tokenIndex].type;
        match(currentType);

        // Check for consecutive id or num tokens
        if (tokenIndex < totalTokens &&
            (strcmp(tokens[tokenIndex].type, "id") == 0 || strcmp(tokens[tokenIndex].type, "num") == 0)) {
            printf("Syntax Error: Missing operator before '%s' at line %d, position %d\n",
                   tokens[tokenIndex].type, tokens[tokenIndex].line, tokens[tokenIndex].position);
            exit(1);
        }

        return node;
    } else if (strcmp(tokens[tokenIndex].type, "(") == 0) {
        match("(");
        ASTNode *node = parseExpression();
        if (tokenIndex >= totalTokens || strcmp(tokens[tokenIndex].type, ")") != 0) {
            printf("Syntax Error: Missing closing ')' at line %d, position %d\n",
                   tokenIndex < totalTokens ? tokens[tokenIndex].line : -1,
                   tokenIndex < totalTokens ? tokens[tokenIndex].position : -1);
            exit(1);
        }
        match(")");
        return node;
    } else {
        printf("Syntax Error: Unexpected token '%s' at line %d, position %d\n",
               tokens[tokenIndex].type, tokens[tokenIndex].line, tokens[tokenIndex].position);
        exit(1);
    }
}

// Function to print AST (for debugging)
void printAST(ASTNode *node, int depth) {
    if (node == NULL) return;

    for (int i = 0; i < depth; i++) printf("  ");
    printf("%s\n", node->value);

    printAST(node->left, depth + 1);
    printAST(node->right, depth + 1);
}

// Recursive helper to fill the AST into a visual tree format
void fillPrintTree(ASTNode *node, int level, int start, int end) {
    if (!node || level >= MAX_HEIGHT || start > end) return;

    int mid = (start + end) / 2;
    int len = strlen(node->value);
    int node_start = mid - (len - 1) / 2;

    if (node_start < 0) node_start = 0;
    if (node_start + len > MAX_WIDTH) node_start = MAX_WIDTH - len;

    for (int i = 0; i < len && node_start + i < MAX_WIDTH; i++) {
        if (node_start + i >= 0)
            treePrint[level][node_start + i] = node->value[i];
    }

    if (node->left) {
        int left_end = node_start - 1;
        int left_start = start;
        int left_mid = (left_start + left_end) / 2;
        if (level + 1 < MAX_HEIGHT && mid > left_mid)
            treePrint[level + 1][left_mid] = '/';
        fillPrintTree(node->left, level + 2, left_start, left_end);
    }

    if (node->right) {
        int right_start = node_start + len + 1;
        int right_end = end;
        int right_mid = (right_start + right_end) / 2;
        if (level + 1 < MAX_HEIGHT && mid < right_mid)
            treePrint[level + 1][right_mid] = '\\';
        fillPrintTree(node->right, level + 2, right_start, right_end);
    }
}

// Helper function to get the width of the subtree 
int getSubtreeWidth(ASTNode *node) {
    if (!node) return 0;
    return strlen(node->value) + getSubtreeWidth(node->left) + getSubtreeWidth(node->right);
}

// Write AST to file
void writeOutputToFile(ASTNode *root, const char *filename) {
    FILE *file = fopen(filename, "a");
    if (!file) {
        printf("Error opening file.\n");
        exit(1);
    }

    for (int i = 0; i < MAX_HEIGHT; i++)
        for (int j = 0; j < MAX_WIDTH; j++)
            treePrint[i][j] = ' ';

    fillPrintTree(root, 0, 0, MAX_WIDTH - 1);

    fprintf(file, "AST:\n");
    for (int i = 0; i < MAX_HEIGHT; i++) {
        int j;
        for (j = MAX_WIDTH - 1; j >= 0 && treePrint[i][j] == ' '; j--);
        treePrint[i][j + 1] = '\0';
        if (j >= 0) fprintf(file, "%s\n", treePrint[i]);
    }
    fprintf(file, "\n");

    fclose(file);
}

// Helper function to get the height of the AST 
int getHeight(ASTNode *node) {
    if (!node) return -1;
    return 1 + fmax(getHeight(node->left), getHeight(node->right));
}

// Main function
int main() {
    printf("Parsing started...\n");

    FILE *clearFile = fopen("output.txt", "w");
    if (clearFile) fclose(clearFile);

    tokenizeAndStore("expressions.txt", "tokens.txt");
    readTokensFromFile("tokens.txt");

    int expressionCount = 0;

    while (tokenIndex < totalTokens) {
        expressionCount++;
        printf("Parsed Expression: (%d)\n", expressionCount);
        ASTNode *root = parseAssignment();
        printAST(root, 0);
        writeOutputToFile(root, "output.txt");
        freeAST(root);

        // Expect a semicolon to separate expressions or end of tokens
        if (tokenIndex < totalTokens && strcmp(tokens[tokenIndex].type, ";") != 0) {
            printf("Syntax Error: Unexpected token '%s' at line %d, position %d\n",
                   tokens[tokenIndex].type, tokens[tokenIndex].line, tokens[tokenIndex].position);
            exit(1);
        }

        // Consume semicolon if present
        if (tokenIndex < totalTokens && strcmp(tokens[tokenIndex].type, ";") == 0) {
            tokenIndex++;
        }
    }

    printf("Parsing Successful. No syntax errors detected.\n");
    return 0;
}
