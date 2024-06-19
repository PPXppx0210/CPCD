#include <iostream>
#include <fstream>
#include <conio.h>
#include "Lexer.h"
#include "Parsing.h"
#include "ObjectCode.h"
using namespace std;

int main()
{
	cout << "�������벿�֣�" << endl;
	cout << "test.txtΪ��C���Բ���Դ���룬֧�ֺ������ã��ɸ���Ҫ����벻ͬԴ������в���" << endl;
	cout << "parse.txtΪ�﷨����ԭ��������Ķ�" << endl << endl;
	cout << "������Ҫ��������" << endl;
	cout << "Token.txtΪ�ʷ��������" << endl;
	cout << "SyntaxTree.pngΪ�﷨��" << endl;
	cout << "MidCode.txtΪ�м�������ɽ��" << endl;
	cout << "BaseBlock.txtΪ�����黮�ֽ��" << endl;
	cout << "ObjCode.asmΪĿ��������ɽ��" << endl << endl;
	cout << "ȷ��test.txt��ΪĿ��Դ����󣬰��������������ʼ����..." << endl;
	while (1) {
		char ch = _getch();
		break;
	}
	cout << "��������ʼ����..." << endl;

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

	pd = lexer.analyze(TestCode);//�ʷ�����
	if (!pd)
	{
		system("pause");
		return 0;
	}
	lexer.outputToken(Token);//�������token

	parser.initSyntax(Grammar);//�����ķ��� ����DFA��LR������
	pd = parser.analyze(lexer.output());//�����ʷ������õ���token��
	if (!pd)
	{
		system("pause");
		return 0;
	}
	parser.outputDFA(DFAset);//���DFA��Ŀ��
	parser.outputAnalyseTable(AnalyseTable);//���LR������
	parser.outputSTree(STreeGraph);//����﷨��
	parser.outputMidcode(MidCode);//����м����

	objectcode.divideBlocks(parser, parser.get_proc_symbolTable());//���ֻ�����
	objectcode.outputBlocks(BaseBlock);//���������
	objectcode.init_INOUTL();//��ʼ������ڻ�Ծ������
	objectcode.analyseBlock();//����������ı�����Ϣ
	objectcode.outputIBlocks(InformationBlock);//�����������Ϣ ���б�����Ϣ
	objectcode.generateCode(parser.get_proc_symbolTable());//����Ŀ�����
	objectcode.outputObjectCode(ObjCode);//���Ŀ�����

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
	cout << "����ɹ��������������ǰ�����ɵ��ļ��鿴���" << endl;
	system("pause");
	return 0;
}
