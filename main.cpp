#include <iostream>
#include <fstream>
#include <conio.h>
#include "Lexer.h"
#include "Parsing.h"
#include "ObjectCode.h"
using namespace std;

int main()
{
	cout << "程序输入部分：" << endl;
	cout << "test.txt为类C语言测试源代码，支持函数调用，可根据要求放入不同源代码进行测试" << endl;
	cout << "parse.txt为语法规则，原则上无需改动" << endl << endl;
	cout << "程序主要输出结果：" << endl;
	cout << "Token.txt为词法分析结果" << endl;
	cout << "SyntaxTree.png为语法树" << endl;
	cout << "MidCode.txt为中间代码生成结果" << endl;
	cout << "BaseBlock.txt为基本块划分结果" << endl;
	cout << "ObjCode.asm为目标代码生成结果" << endl << endl;
	cout << "确认test.txt中为目标源程序后，按任意键编译器开始运行..." << endl;
	while (1) {
		char ch = _getch();
		break;
	}
	cout << "编译器开始运行..." << endl;

	ifstream TestCode, Grammar;
	ofstream Token, DFAset, AnalyseTable, STreeGraph, MidCode, BaseBlock, InformationBlock, ObjCode;

	TestCode.open("./test.txt");
	Grammar.open("./parse.txt");
	Token.open("./Token.txt");
	DFAset.open("./DFA.txt");
	AnalyseTable.open("./LRAnalyser.txt");
	STreeGraph.open("./SyntaxTree.dot");
	MidCode.open("./MidCode.txt");
	BaseBlock.open("./BaseBlock.txt");
	InformationBlock.open("./InformationBlocks.txt");
	ObjCode.open("./ObjCode.asm");

	Lexer lexer;
	Parsing parser;
	ObjectCode objectcode;
	bool pd = true;

	pd = lexer.analyze(TestCode);//词法分析
	if (!pd)
	{
		system("pause");
		return 0;
	}
	lexer.outputToken(Token);//输出所有token

	parser.initSyntax(Grammar);//读入文法表 构造DFA和LR分析表
	pd = parser.analyze(lexer.output());//分析词法分析得到的token表
	if (!pd)
	{
		system("pause");
		return 0;
	}
	parser.outputDFA(DFAset);//输出DFA项目集
	parser.outputAnalyseTable(AnalyseTable);//输出LR分析表
	parser.outputSTree(STreeGraph);//输出语法树
	parser.outputMidcode(MidCode);//输出中间代码

	objectcode.divideBlocks(parser, parser.get_proc_symbolTable());//划分基本块
	objectcode.outputBlocks(BaseBlock);//输出基本块
	objectcode.init_INOUTL();//初始化出入口活跃变量集
	objectcode.analyseBlock();//分析基本块的变量信息
	objectcode.outputIBlocks(InformationBlock);//输出基本块信息 带有变量信息
	objectcode.generateCode(parser.get_proc_symbolTable());//生成目标代码
	objectcode.outputObjectCode(ObjCode);//输出目标代码

	TestCode.close();
	Grammar.close();
	Token.close();
	DFAset.close();
	AnalyseTable.close();
	STreeGraph.close();
	MidCode.close();
	BaseBlock.close();
	InformationBlock.close();
	ObjCode.close();
	cout << "编译成功，程序结束，可前往生成的文件查看结果" << endl;
	system("pause");
	return 0;
}
