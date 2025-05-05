#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_ROWS 100
#define MAX_COLS 100
#define MAX_CELL_LEN 256

char headers[MAX_COLS][MAX_CELL_LEN];
char raw[MAX_ROWS][MAX_COLS][MAX_CELL_LEN];
int values[MAX_ROWS][MAX_COLS];
int calculated[MAX_ROWS][MAX_COLS];
int row_indices[MAX_ROWS];
int num_cols = 0, num_rows = 0;

int eval_cell(int row, int col);

int get_col_index(const char *name) 
{
	int i;
    for (i = 0; i < num_cols; i++) 
	{
        if (strcmp(headers[i], name) == 0) 
			return i;
    }
    return -1;
}

int get_row_index(int id) 
{
	int i;
    for (i = 0; i < num_rows; i++) 
	{
        if (row_indices[i] == id) 
			return i;
    }
    return -1;
}

int parse_operand(const char *s, int *result) 
{
    char clean[MAX_CELL_LEN];
    
    int i, j = 0;

    for (i = 0; s[i] != '\0'; i++) 
	{
        if (!isspace((unsigned char)s[i]) && s[i] != '\r' && s[i] != '\n') 
		{
            clean[j++] = s[i];
        }
    }
    clean[j] = '\0';

    if (isdigit(clean[0]) || (clean[0] == '-' && isdigit(clean[1]))) 
	{
        *result = atoi(clean);
        return 0;
    }
    
    else if (isalpha(clean[0])) 
	{
        char col_name[MAX_CELL_LEN];
        int row_id;
        int k = 0;

        while (isalpha(clean[k])) 
		{
            col_name[k] = clean[k];
            k++;
        }
        col_name[k] = '\0';

        row_id = atoi(&clean[k]);

        int col_idx = get_col_index(col_name);
        int row_idx = get_row_index(row_id);

        if (col_idx == -1) 
		{
            fprintf(stderr, "Column '%s' not found\n", col_name);
            return -1;
        }
        
        if (row_idx == -1) 
		{
            fprintf(stderr, "String '%d' not found\n", row_id);
            return -1;
        }

        *result = eval_cell(row_idx, col_idx);
        return 0;
    } 
	else 
	{
        fprintf(stderr, "Invalid operand: '%s'\n", clean);
        return -1;
    }
}


int eval_cell(int row, int col) 
{
    if (calculated[row][col]) return values[row][col];

    const char *cell = raw[row][col];

    if (cell[0] != '=') 
	{
        values[row][col] = atoi(cell);
        calculated[row][col] = 1;
        return values[row][col];
    }

    char arg1[MAX_CELL_LEN], arg2[MAX_CELL_LEN], *op_ptr;
	const char *expr = cell + 1; 

	op_ptr = strpbrk(expr, "+-*/");
	if (!op_ptr) 
	{
    	fprintf(stderr, "Invalid expression: %s\n", cell);
    	exit(1);
	}	

	char op = *op_ptr;
	size_t len1 = op_ptr - expr;
	strncpy(arg1, expr, len1);
	arg1[len1] = '\0';
	strcpy(arg2, op_ptr + 1);

    int val1, val2;
    if (parse_operand(arg1, &val1) || parse_operand(arg2, &val2)) 
	{
        fprintf(stderr, "Invalid operand in: %s\n", cell);
        exit(1);
    }

    int result = 0;
    switch (op) 
	{
        case '+': result = val1 + val2; break;
        case '-': result = val1 - val2; break;
        case '*': result = val1 * val2; break;
        case '/':
        	
            if (val2 == 0) 
			{
                fprintf(stderr, "Division by zero in: %s\n", cell);
                exit(1);
            }
            
			result = val1 / val2;
            break;
        
		default:
            fprintf(stderr, "Unknown operator in: %s\n", cell);
            exit(1);
    }

    values[row][col] = result;
    calculated[row][col] = 1;
    return result;
}

void read_csv(const char *filename) 
{
    FILE *f = fopen(filename, "r");
    if (!f) 
	{
        perror("fopen");
        exit(1);
    }

    char line[4096];
    if (!fgets(line, sizeof(line), f)) 
	{
        fprintf(stderr, "Empty file\n");
        exit(1);
    }

    if (line[0] == '\xEF' && line[1] == '\xBB' && line[2] == '\xBF') 
	{
        memmove(line, line + 3, strlen(line) - 2);
    }

    char *token = strtok(line, ",\n");
    while (token != NULL) 
	{
        if (token[0] != '\0') 
		{
            token[strcspn(token, "\r\n")] = '\0';
            strcpy(headers[num_cols], token);
            num_cols++;
        }
        
        token = strtok(NULL, ",\n");
    }

    while (fgets(line, sizeof(line), f)) 
	{
        token = strtok(line, ",\n");
        if (token == NULL) continue;

        row_indices[num_rows] = atoi(token);
        int col = 0;

        while ((token = strtok(NULL, ",\n")) != NULL && col < num_cols) 
		{
            token[strcspn(token, "\r\n")] = '\0';
            strcpy(raw[num_rows][col], token);
            col++;
        }
        
        num_rows++;
    }

    fclose(f);
}

void evaluate_all() 
{
	int r;
	int c;
    for (r = 0; r < num_rows; r++)
        for (c = 0; c < num_cols; c++)
            eval_cell(r, c);
}

void print_table() 
{

    printf(",");
    int i, r, c;

    for (i = 0; i < num_cols; i++) 
	{
        printf("%s", headers[i]);
        if (i < num_cols - 1) 
		{
            printf(",");
        }
    }
    printf("\n");

    for (r = 0; r < num_rows; r++) 
	{
        printf("%d", row_indices[r]);
        
		for (c = 0; c < num_cols; c++) 
		{
            printf(",%d", values[r][c]);
        }
        
		printf("\n");
    }
}

int main(int argc, char *argv[]) 
{
    if (argc != 2) 
	{
        fprintf(stderr, "Usage: %s file.csv\n", argv[0]);
        return 1;
    }

    read_csv(argv[1]);
    evaluate_all();
    print_table();

    return 0;
}

