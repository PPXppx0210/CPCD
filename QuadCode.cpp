#include "QuadCode.h"

IntermediateLanguage::IntermediateLanguage() :nextquad(CODE_START_POS)
{}

//向中间代码vector中添加新的一句并返回其索引
int IntermediateLanguage::emit_code(const quadruple& newqr)
{
	code.push_back(newqr);
	int quadnow = nextquad;
	nextquad++;
	return quadnow;
}

//回填部分中间代码的出口
void IntermediateLanguage::back_patch(std::vector<quadrupleIndex> qrlist, int pos)
{
	for (auto it = qrlist.begin(); it != qrlist.end(); it++)
		code[(*it) - CODE_START_POS].result = std::to_string(pos); //注意减去代码段的初始偏移
	return;
}

//返回string类型的操作符
string Oper2string(Oper op)
{
	switch (op)
	{
		case Oper::J:
			return "J";
			break;
		case Oper::Jeq:
			return "Jeq";
			break;
		case Oper::Jgt:
			return "Jgt";
			break;
		case Oper::Jge:
			return "Jge";
			break;
		case Oper::Jlt:
			return "Jlt";
			break;
		case Oper::Jle:
			return "Jle";
			break;
		case Oper::Jne:
			return "Jne";
			break;
		case Oper::Plus:
			return "Plus";
			break;
		case Oper::Minus:
			return "Minus";
			break;
		case Oper::Multiply:
			return "Multiply";
			break;
		case Oper::Divide:
			return "Divide";
			break;
		case Oper::Assign:
			return "Assign";
			break;
		case Oper::Parm:
			return "Parm";
			break;
		case Oper::Call:
			return "Call";
			break;
		case Oper::Return:
			return "Return";
			break;
		case Oper::AssignArray:
			return "AssignArray";
			break;
		case Oper::ArrayAssign:
			return "ArrayAssign";
			break;
		case Oper::Get:
			return "Get";
			break;
		default:
			return "-1";
	}
}

//中间代码输出到文件中
void IntermediateLanguage::output(ofstream& midcode)
{
	int pos = CODE_START_POS;
	for (auto i : code)
		midcode << pos++ << "," << Oper2string(i.Op) << "," << i.arg1 << "," << i.arg2 << "," << i.result << "," << endl;
}

quadruple::quadruple(Oper Op, std::string arg1, std::string arg2, std::string result)
{
	this->Op = Op;
	this->arg1 = arg1;
	this->arg2 = arg2;
	this->result = result;
}

quadruple::quadruple() : Op(Oper::Assign)
{}