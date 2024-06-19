#pragma once
#include <vector>
#include <map>
#include <set>
#include <utility>
#include <stack>
#include <string>
#include <fstream>
#include "Lexer.h"
#include "SymbolTable.h"
#include "QuadCode.h"
using namespace std;

class DFA_item;
typedef int symbolTableIndex;
typedef int syntaxTableIndex;
typedef int syntaxTreeNodeIndex;
typedef int DFA_statusIndex;
typedef int quadrupleIndex;
typedef std::set<symbolTableIndex> firstTableItem;
typedef std::pair<char, int> analyseTableItem;
typedef std::string symbolItem;
typedef std::set<DFA_item> DFA_status;

//��Ŀ
class DFA_item
{
public:
	symbolTableIndex lhs;
	vector<symbolTableIndex> rhs;
	int pos;
	symbolTableIndex forecast;
};

//�ķ�
class syntaxTableItem
{
public:
	symbolTableIndex lhs;  //������
	vector<symbolTableIndex> rhs;//����ʽ��������
};

//�﷨���ڵ�
class syntaxTreeNode
{
public:
	syntaxTreeNodeIndex index;
	syntaxTreeNodeIndex parent;
	vector<syntaxTreeNodeIndex> children;
	syntaxTableIndex productions; //����ʽ
	symbolTableIndex type;        //token����
	string val;                   //token����ֵ
	int line;                     //��  
	int position;                 //��

	quadrupleIndex quad;
	symbolType stype;
	string place;
	vector<quadrupleIndex> truelist;
	vector<quadrupleIndex> falselist;
	vector<quadrupleIndex> nextlist;
	std::vector<symbolTableItem> plist;
	vector<string> array_vec;
	string array_place;
	bool inTree;

	syntaxTreeNode();
	syntaxTreeNode(const TokenInfo&);
};

class ObjectCode;

//�﷨������
class Parsing
{
	vector<symbolItem> symbolTable;     //���ű�
	symbolTableIndex terminalSymbolMax; //���һ���ս��������
	symbolTableIndex startIndex;		//��ʼ������
	symbolTableIndex emptyIndex;		//���ַ�����
	map<symbolItem, int> symbol2Index;  //symbolӳ��
	map<int, symbolItem> Index2symbol;  //����ӳ��
	vector<syntaxTableItem> syntaxTable;//�ķ���
	vector<set<syntaxTableIndex>> searchSyntaxByLhs; //ͨ���ķ�������Ҳ���ʽ
	vector<firstTableItem> firstTable;//first��
	vector<DFA_status> DFA;//��Ŀ��
	vector<vector<analyseTableItem>> analyseTable;//������
	//��������
	vector<syntaxTreeNode> syntaxTree;			   //�﷨���ڵ�
	stack<DFA_statusIndex> statusStack;			   //����״̬ջ
	stack<syntaxTreeNodeIndex> analyseSymbolStack; //��������ջ
	stack<syntaxTreeNodeIndex> inputSymbolvector;  //�������ջ
	syntaxTreeNodeIndex topNode;
	//�м�������ɲ���
	proc_symbolTable* p_symbolTable;//���ű�
	IntermediateLanguage mid_code;//�м����

	void initSymbolTable(ifstream&);//��ʼ�����ű�
	void initFirstTable();//��FIRST��
	void initAnalyseTable();//��ʼ��LR������
	void initTerminalSymbol();//��ʼ���ս����
	bool generate_midcode(syntaxTableIndex SyntaxIndex, syntaxTreeNode& lhs, vector<syntaxTreeNodeIndex>& rhs);//�����м����
	symbolTableIndex insertSymbol(symbolItem);//����ű��в����µ���
	set<symbolTableIndex> firstForPhrase(vector<symbolTableIndex> p);//������FIRST��
	pair<int, bool> createClosure(DFA_status& sta);//����Ŀ���հ�
	void outputDot(ofstream&, syntaxTreeNodeIndex);//����﷨����DOT�ļ�
	vector<quadrupleIndex> mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2);//�ϲ������
	vector<quadrupleIndex> mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2, vector<quadrupleIndex>& list3);

public:
	Parsing() = default;
	void clear();
	void initSyntax(ifstream&);//�﷨������ʼ��
	bool analyze(const vector<TokenInfo>&);//�﷨����
	void outputSTree(ofstream& graph);//����﷨������
	void outputMidcode(ofstream& midcode);//����м����
	void outputDFA(ofstream&);//���DFA��Ŀ��
	void outputAnalyseTable(ofstream&);//���LR������
	proc_symbolTable* get_proc_symbolTable()const;//��ȡ���ű�
	friend class ObjectCode;
};
