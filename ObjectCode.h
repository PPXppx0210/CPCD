#pragma once
#include "QuadCode.h"
#include "Parsing.h"

class NewLabeler {
private:
	int index;
	int tmp;
public:
	NewLabeler();
	string newLabel();
	string newTmp();
};

//变量信息
class VarInfomation {
public:
	int next;//待用信息
	bool active;//活跃信息

	VarInfomation(int next, bool active);
	VarInfomation(const VarInfomation& other);
	VarInfomation();
	void output(ostream& out);
};

//带有变量信息的四元式
class QuaternaryWithInfo {
public:
	quadruple q;//四元式
	VarInfomation info1;//变量信息
	VarInfomation info2;
	VarInfomation info3;

	QuaternaryWithInfo(quadruple q, VarInfomation info1, VarInfomation info2, VarInfomation info3);
	void output(ostream& out);
};

//带有变量信息的基本块
struct IBlock {
	string name;
	vector<QuaternaryWithInfo> codes;//带有变量信息的四元式数组
	int next1;
	int next2;
};

class ObjectCode {
private:
	IntermediateLanguage code;//中间代码
	map<string, vector<Block> >funcBlocks; //各函数的基本块
	map<string, vector<set<string> > >funcOUTL; //各函数块中基本块的出口活跃变量集
	map<string, vector<set<string> > >funcINL; //各函数块中基本块的入口活跃变量集
	NewLabeler label;
	map<string, vector<IBlock> >funcIBlocks;//一个函数映射到多个基本块
	map<string, set<string> >Avalue;//存储函数中的活跃变量的集合
	map<string, set<string> >Rvalue;//存储函数中的可用寄存器的集合
	map<string, int>varOffset;//各变量的存储位置
	int top;//当前栈顶
	list<string>freeReg;//空闲的寄存器编号
	string nowFunc;//当前分析的函数
	vector<IBlock>::iterator nowIBlock;//当前分析的基本块
	vector<QuaternaryWithInfo>::iterator nowQuatenary;//当前分析的四元式
	vector<string>objectCodes;//目标代码

	void storeVar(string reg, string var);//存储变量和寄存器之间的映射关系
	void storeOutLiveVar(set<string>& outl);//存储出口活跃变量
	void releaseVar(string var);//释放变量的寄存器
	string getReg();//获取一个可用的寄存器
	string allocateReg();//分配一个寄存器
	string allocateReg(string var);//为特定变量分配一个寄存器

	void generateArrayData(const proc_symbolTable*);//生成数组数据
	void generateCodeForFuncBlocks(map<string, vector<IBlock> >::iterator& fiter);//为函数的各个基本块生成目标代码
	void generateCodeForBaseBlocks(int nowBaseBlockIndex);//为基本块生成目标代码
	void generateCodeForQuatenary(int nowBaseBlockIndex, int& arg_num, int& par_num, list<pair<string, bool> >& par_list);//为四元式生成目标代码
public:
	ObjectCode();
	void outputBlocks(ostream& out);  //输出基本块
	void divideBlocks(const Parsing&, const proc_symbolTable*);  //根据语法树和符号表划分基本块
	void init_INOUTL(); //初始化出入口活跃变量集
	void generateCode(const proc_symbolTable*);//生成目标代码
	void analyseBlock();//分析基本块得到变量信息
	void outputIBlocks(ostream& out);//输出各个基本块 带变量信息
	void outputObjectCode(ostream& out);//输出目标代码
};