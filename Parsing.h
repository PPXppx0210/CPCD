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

//项目
class DFA_item
{
public:
	symbolTableIndex lhs;
	vector<symbolTableIndex> rhs;
	int pos;
	symbolTableIndex forecast;
};

//文法
class syntaxTableItem
{
public:
	symbolTableIndex lhs;  //左部类型
	vector<symbolTableIndex> rhs;//产生式索引数组
};

//语法树节点
class syntaxTreeNode
{
public:
	syntaxTreeNodeIndex index;
	syntaxTreeNodeIndex parent;
	vector<syntaxTreeNodeIndex> children;
	syntaxTableIndex productions; //产生式
	symbolTableIndex type;        //token类型
	string val;                   //token具体值
	int line;                     //行  
	int position;                 //列

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

//语法分析器
class Parsing
{
	vector<symbolItem> symbolTable;     //符号表
	symbolTableIndex terminalSymbolMax; //最后一个终结符的索引
	symbolTableIndex startIndex;		//开始符索引
	symbolTableIndex emptyIndex;		//空字符索引
	map<symbolItem, int> symbol2Index;  //symbol映射
	map<int, symbolItem> Index2symbol;  //索引映射
	vector<syntaxTableItem> syntaxTable;//文法表
	vector<set<syntaxTableIndex>> searchSyntaxByLhs; //通过文法的左侧找产生式
	vector<firstTableItem> firstTable;//first表
	vector<DFA_status> DFA;//项目集
	vector<vector<analyseTableItem>> analyseTable;//分析表
	//分析过程
	vector<syntaxTreeNode> syntaxTree;			   //语法树节点
	stack<DFA_statusIndex> statusStack;			   //分析状态栈
	stack<syntaxTreeNodeIndex> analyseSymbolStack; //分析符号栈
	stack<syntaxTreeNodeIndex> inputSymbolvector;  //输入符号栈
	syntaxTreeNodeIndex topNode;
	//中间代码生成部分
	proc_symbolTable* p_symbolTable;//符号表
	IntermediateLanguage mid_code;//中间代码

	void initSymbolTable(ifstream&);//初始化符号表
	void initFirstTable();//求FIRST集
	void initAnalyseTable();//初始化LR分析表
	void initTerminalSymbol();//初始化终结符集
	bool generate_midcode(syntaxTableIndex SyntaxIndex, syntaxTreeNode& lhs, vector<syntaxTreeNodeIndex>& rhs);//生成中间代码
	symbolTableIndex insertSymbol(symbolItem);//向符号表中插入新的项
	set<symbolTableIndex> firstForPhrase(vector<symbolTableIndex> p);//求短语的FIRST集
	pair<int, bool> createClosure(DFA_status& sta);//求项目集闭包
	void outputDot(ofstream&, syntaxTreeNodeIndex);//输出语法树的DOT文件
	vector<quadrupleIndex> mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2);//合并真假链
	vector<quadrupleIndex> mergelist(vector<quadrupleIndex>& list1, vector<quadrupleIndex>& list2, vector<quadrupleIndex>& list3);

public:
	Parsing() = default;
	void clear();
	void initSyntax(ifstream&);//语法分析初始化
	bool analyze(const vector<TokenInfo>&);//语法分析
	void outputSTree(ofstream& graph);//输出语法分析树
	void outputMidcode(ofstream& midcode);//输出中间代码
	void outputDFA(ofstream&);//输出DFA项目集
	void outputAnalyseTable(ofstream&);//输出LR分析表
	proc_symbolTable* get_proc_symbolTable()const;//获取符号表
	friend class ObjectCode;
};
