#include <iostream>
#include <fstream>
#include <map>
#include <vector>
#include <cstdint>
#include <regex>//библиотека дл€ работы с регул€рными выражени€ми
#include <algorithm>
#include <string>

using namespace std;

const regex pattern_binary("([[:alpha:]]):=([[:alpha:]])([[:punct:]]{1,})([[:alpha:]]);");
const regex pattern_not("([[:alpha:]]):=\\x5c([[:alpha:]]);");
const regex pattern_read_base("read[(]([[:alpha:]]),([[:digit:]]{1,})[)];");
const regex pattern_read("read[(]([[:alpha:]])[)];");
const regex pattern_write_base("write[(]([[:alpha:]]),([[:digit:]]{1,})[)];");
const regex pattern_write("write[(]([[:alpha:]])[)];");

enum {
    OR,
    AND,
    IMP,
    REV_IMP,
    EQU,
    XOR,
    COIMP,
    AND_NOT,
    OR_NOT,
    NOT,
    READ,
    WRITE
};

map<string, int> opcode;

enum {
    ERROR_SUCCESS = 0,
    FILE_ERROR = -1,
    COMMENT_ERROR = -2,
    SYNTAX_ERROR = -3,
    BASE_ERROR = -4,
    RUNTIME_ERROR = -5
};

typedef struct {
    string result;
    string operand_1;
    string operand_2;
    unsigned char operation;
    int base;
} token_struct_t;

int get_token_struct(const string code_line, token_struct_t& token_struct);
int execute(token_struct_t& token_struct, map<string, int>& value_array, 
            bool trace_flag, ofstream& output_file, size_t instruction_num);

int other_to_dec(string value, int base);
string dec_to_other(int value, int base);
bool test_base(string value, int base);
int main(int argc, char* argv[])
{
    cout << "Laba 1 Task 6\nDrobotun Yulia M8o-210B-20\n" << endl << endl;
    ofstream output_file;
    bool is_trace = false;
    vector<string> source_text;
    map<string, int> value_array;
    token_struct_t token_struct;

    size_t bracket_count = 0;
    bool is_line_comment = false;
    string code_line;
    char input_byte;
    size_t instruction_num;
    int err_code;
    opcode["+"] = OR;
    opcode["&"] = AND;
    opcode["->"] = IMP;
    opcode["<-"] = REV_IMP;
    opcode["~"] = EQU;
    opcode["<>"] = XOR;
    opcode["+>"] = COIMP;
    opcode["&"] = AND_NOT;
    opcode["!"] = OR_NOT;

    if (string(argv[2]) == "/trace")
    {
        is_trace = true;
        output_file.open(argv[3]);
        if (!output_file)
        {
            cout << "Trace file open error..." << endl;
        }
    }

    ifstream input_file(argv[1], ios::binary);
    if (!input_file)
    {
        cout << "Source file open error..." << endl;
        return FILE_ERROR;
    }
    while (!input_file.eof())
    {
        input_file.get(input_byte);
        if (input_byte == '{')
        {
            bracket_count++;
        }
        if (input_byte == '}' && bracket_count > 0)
        {
            bracket_count--;
            input_file.get(input_byte);
        }
        if (input_byte == '}' && bracket_count == 0)
        {
            cout << "Invalid comment..." << endl;
            return COMMENT_ERROR;
        }
        if (input_byte == '%')
        {
            is_line_comment = true;
        }
        if (input_byte == '\n' && is_line_comment)
        {
            is_line_comment = false;
        }
        if (bracket_count == 0 && input_byte != '}' && !isspace(input_byte) && !is_line_comment)
        {
            code_line.push_back(input_byte);
        }
        if (input_byte == '\n')
        {
            if (!code_line.empty() && code_line[0] != 10 && code_line[0] != 13)//коды перевод строки и конец строки
            {
                source_text.push_back(code_line);
            }
            code_line.clear();
        }
    }
    if (bracket_count != 0)
    {
        cout << "Invalid comment..." << endl;
        return COMMENT_ERROR;
    }
    input_file.close();
    for (size_t i = 0; i < source_text.size(); i++)
    {
        instruction_num = i;
        err_code = get_token_struct(source_text[i], token_struct);
        if (err_code != ERROR_SUCCESS)
        {
            break;
        }
        err_code = execute(token_struct, value_array, is_trace, output_file, instruction_num);
        if (err_code != ERROR_SUCCESS)
        {
            break;
        }
    }
    switch (err_code) {
    case SYNTAX_ERROR:
        cout << "Syntax error in " << instruction_num + 1 << " instruction..." << endl;
        cout << "--> " << source_text[instruction_num] << endl;
        break;
    case BASE_ERROR:
        cout << "Invalid base value in " << instruction_num + 1 << " instruction..." << endl;
        cout << "--> " << source_text[instruction_num] << endl;
        break;
    case RUNTIME_ERROR:
        cout << "Runtime error in " << instruction_num + 1 << " instruction..." << endl;
        cout << "--> " << source_text[instruction_num] << endl;
        break;
    case ERROR_SUCCESS:
        cout << "The running is complete" << endl;
        break;
    default:
        break;
    }
    if (is_trace)
    {
        output_file.close();
    }
    return err_code;
}

int get_token_struct(const string code_line, token_struct_t& token_struct)
{
    smatch token_match;
    string base;
    if (regex_match(code_line, token_match, pattern_binary))
    {
        if (token_match.str(3) != "+" &&
            token_match.str(3) != "&" &&
            token_match.str(3) != "->" &&
            token_match.str(3) != "<-" &&
            token_match.str(3) != "~" &&
            token_match.str(3) != "<>" &&
            token_match.str(3) != "+>" &&
            token_match.str(3) != "?" &&
            token_match.str(3) != "!")
        {
            return SYNTAX_ERROR;
        }
        token_struct.result = token_match.str(1);
        token_struct.operand_1 = token_match.str(2);
        token_struct.operand_2 = token_match.str(4);
        token_struct.operation = opcode[token_match.str(3)];

    }
    else if (regex_match(code_line, token_match, pattern_not))
    {
        token_struct.result = token_match.str(1);
        token_struct.operand_1 = token_match.str(2);
        token_struct.operation = NOT;
    }
    else if (regex_match(code_line, token_match, pattern_read_base))
    {
        token_struct.operand_1 = token_match.str(1);
        token_struct.base = stoi(string(token_match.str(2)));
        token_struct.operation = READ;
        if (token_struct.base < 2 || token_struct.base > 36)
        {
            return BASE_ERROR;
        }
    }
    else if (regex_match(code_line, token_match, pattern_read))
    {
        token_struct.operand_1 = token_match.str(1);
        token_struct.base = 10;
        token_struct.operation = READ;
    }
    else if (regex_match(code_line, token_match, pattern_write_base))
    {
        token_struct.operand_1 = token_match.str(1);
        base = token_match.str(2);
        token_struct.base = stoi(base);
        token_struct.operation = WRITE;
        if (token_struct.base < 2 || token_struct.base > 36)
        {
            return BASE_ERROR;
        }
    }
    else if (regex_match(code_line, token_match, pattern_write))
    {
        token_struct.operand_1 = token_match.str(1);
        token_struct.base = 10;
        token_struct.operation = WRITE;
    }
    else
    {
        return SYNTAX_ERROR;
    }
    return ERROR_SUCCESS;
}

int execute(token_struct_t& token_struct, map<string, int>& value_array, 
            bool trace_flag, ofstream &output_file, size_t instruction_num)
{
    string input_str;
    regex pattern_value("[a-zA-Z0-9]+");//шаблон дл€ проверки корректности ввода символов в операции read
    switch (token_struct.operation) {
    case OR:
        value_array[token_struct.result] = value_array[token_struct.operand_1] | value_array[token_struct.operand_2];
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case AND:
        value_array[token_struct.result] = value_array[token_struct.operand_1] & value_array[token_struct.operand_2];
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case IMP:
        value_array[token_struct.result] = ~value_array[token_struct.operand_1] | value_array[token_struct.operand_2];
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case REV_IMP:
        value_array[token_struct.result] = value_array[token_struct.operand_1] | ~value_array[token_struct.operand_2];
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case EQU:
        value_array[token_struct.result] = (~value_array[token_struct.operand_1] & ~value_array[token_struct.operand_2]) |
            (value_array[token_struct.operand_1] & value_array[token_struct.operand_2]);
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case XOR:
        value_array[token_struct.result] = value_array[token_struct.operand_1] ^ value_array[token_struct.operand_2];
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case COIMP:
        value_array[token_struct.result] = value_array[token_struct.operand_1] & ~value_array[token_struct.operand_2];
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case AND_NOT:
        value_array[token_struct.result] = ~(value_array[token_struct.operand_1] & value_array[token_struct.operand_2]);
        break;
    case OR_NOT:
        value_array[token_struct.result] = ~(value_array[token_struct.operand_1] | value_array[token_struct.operand_2]);
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand 1 = " << token_struct.operand_1 << endl;
            output_file << "Operand 2 = " << token_struct.operand_2 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case NOT:
        value_array[token_struct.result] = ~value_array[token_struct.operand_1];
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand = " << token_struct.operand_1 << endl;
            output_file << "Result = " << value_array[token_struct.result] << endl;
        }
        break;
    case READ:
        cout << "Enter value in " << token_struct.base << " base: ";
        cin >> input_str;

        transform(input_str.begin(), input_str.end(), input_str.begin(), ::toupper);
        if (!regex_match(input_str, pattern_value))
        {
            return RUNTIME_ERROR;
        }
        if (!test_base(input_str, token_struct.base))
        {
            return RUNTIME_ERROR;
        }
        value_array[token_struct.operand_1] = other_to_dec(input_str, token_struct.base);
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand = " << token_struct.operand_1 << endl;
            output_file << "Result = " << value_array[token_struct.operand_1] << endl;
        }
        break;
    case WRITE:
        cout << "The value with base " << token_struct.base << " is: ";
        cout << dec_to_other(value_array[token_struct.operand_1], token_struct.base) << endl;
        if (trace_flag)
        {
            output_file << "Instruction - " << instruction_num << endl;
            output_file << "Operand = " << token_struct.operand_1 << endl;
            output_file << "Result = " << value_array[token_struct.operand_1] << endl;
        }
        break;
    default:
        break;
    }
    return ERROR_SUCCESS;
}

int other_to_dec(string value, int base)
{
    int finished_value = 0;
    for (size_t i = 0; i < value.length(); i++)
    {
        finished_value = finished_value * base + (isdigit(value[i]) ? value[i] - '0' : value[i] - 'A' + 10);
    }
    return finished_value;
}

string dec_to_other(int value, int base)
{
    string finished_value;
    int digit, counter = 0;
    if (value == 0)
    {
        finished_value.push_back('0');
    }
    while (value > 0)
    {
        digit = value % base;
        finished_value.push_back(digit > 9 ? digit - 10 + 'A' : digit + '0');
        value = value / base;
        counter++;
    }
    reverse(finished_value.begin(), finished_value.end());
    return finished_value;
}

bool test_base(string value, int base)
{
    for (size_t i = 0; i < value.length(); i++)
    {
        if (isdigit(value[i]) && value[i] - 47 > base)
        {
            return false;
        }
        if (isalpha(value[i]) && value[i] - 54 > base)
        {
            return false;
        }
    }
    return true;
}
