#pragma once
#include <string>
#include <vector>
#include <fstream>
#include <list>
#include <map>
using namespace std;

const int CODE_START_POS = 100;
typedef int quadrupleIndex;

enum class Oper : char
{
	J = 0,
	Jeq,
	Jgt,
	Jge,
	Jlt,
	Jle,
	Jne,
	Plus,
	Minus,
	Multiply,
	Divide,
	Assign,
	AssignArray,
	ArrayAssign,
	Parm,
	Call,
	Get,
	Return
};
string Oper2string(Oper op);

class quadruple
{
public:
	quadruple(Oper Op, std::string arg1, std::string arg2, std::string result);
	quadruple();
	Oper Op;//中间代码操作符
	std::string arg1;//操作数1
	std::string arg2;//操作数2
	std::string result;//结果
};

struct Block {
	string name;
	vector<quadruple> codes;
	int next1;//块的顺序下一块
	int next2;//块的跳转下一块
};

class IntermediateLanguage
{
public:
	std::vector<quadruple> code;//存储中间代码的数组
	quadrupleIndex nextquad;//下一句中间代码

	int emit_code(const quadruple& newqr);
	void back_patch(std::vector<quadrupleIndex> qrlist, int pos);
	void output(ofstream& midcode);
	IntermediateLanguage();
};