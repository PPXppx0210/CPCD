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
	Oper Op;//�м���������
	std::string arg1;//������1
	std::string arg2;//������2
	std::string result;//���
};

struct Block {
	string name;
	vector<quadruple> codes;
	int next1;//���˳����һ��
	int next2;//�����ת��һ��
};

class IntermediateLanguage
{
public:
	std::vector<quadruple> code;//�洢�м���������
	quadrupleIndex nextquad;//��һ���м����

	int emit_code(const quadruple& newqr);
	void back_patch(std::vector<quadrupleIndex> qrlist, int pos);
	void output(ofstream& midcode);
	IntermediateLanguage();
};